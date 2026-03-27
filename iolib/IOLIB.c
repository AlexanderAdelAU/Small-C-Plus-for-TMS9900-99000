/*  name...
 *      iolib
 *
 *  purpose...
 *      Standard C I/O interface for programs running on this system.
 *
 *  notes...
 *      Compile using -M option.
 *
 *  architecture...
 *      BDOS lives in segment 0.  Application code may live in any
 *      segment.  All BDOS FCB and DMA operations are routed through
 *      two staging areas in COMMON memory so BDOS always receives
 *      pointers it can reach:
 *
 *        _commfcb  (physical addr stored at COMM_FCB = 0xAC)
 *            One shared FCB.  _stage_fcb() copies the per-slot FCB
 *            into common before each BDOS call; _unstage_fcb() copies
 *            it back so BDOS field updates are preserved in the slot.
 *
 *        _commbuf  (physical addr stored at COMM_BUF = 0xA4)
 *            One shared 512-byte DMA window.  Reads: BDOS fills
 *            _commbuf, _getbuf copies into the per-slot buffer.
 *            Writes: fflush copies from per-slot buffer into _commbuf,
 *            then calls WRSEQ.
 *
 *  FCB layout (this BDOS, TMS9900 big-endian words)...
 *      Bytes 0-7   filename
 *      Bytes 8-10  extension
 *      Byte  11    FTY  - file type
 *      Bytes 12-13 FSB  - file starting block
 *      Bytes 14-15 FSZ  - file size in sectors
 *      Bytes 16-17 FLA  - file load address
 *      Bytes 18-19 FSZBH- file size in bytes (high word)
 *      Bytes 20-21 LRBL - last record byte length
 *      Bytes 22-23 spare
 *      Bytes 24-25 CBN  - current block number
 *      Bytes 26-27 CRN  - current record number (data begins at CRN=1)
 *      Bytes 28-29 RELB - relative block number
 *      Bytes 30-31 RELR - relative record number
 *      Byte  32    CMLTI- current module load index
 *      Byte  33    NMSECT-module size in sectors
 *      Byte  34    MPAGE- memory page / drive
 *      Byte  35    LBUFCNT - bytes in last buffer (binary EOF support)
 *
 *      No drive byte in bytes 0-31.  Drive selected via SELDSK(14).
 *      All word fields: high byte at even offset, low byte at odd offset.
 *      FCB_xxx defines below point to the low (active) byte of each word.
 *
 *  append mode...
 *      CP/M has no native append.  fopen("a") opens for reading,
 *      reads to EOF so BDOS CRN is positioned at the append point,
 *      decrements CRN by 1 so WRSEQ rewrites the last sector (which
 *      contains existing data up to ^Z plus new appended data), then
 *      switches the slot to MWRITE.  BDOS sequential state is preserved
 *      by staying in the same open - never FCLOSE/FOPEN.
 *
 *  EOF handling...
 *      fclose() pads the last sector with ^Z (0x1A) for text
 *      compatibility and sets LBUFCNT (byte 35) to the exact byte
 *      count of valid data in the last sector.  getc() checks for ^Z
 *      as a fallback for files written without LBUFCNT support.
 *      When BDOS implements LBUFCNT, _getbuf() will use it to limit
 *      the valid byte count without needing ^Z scanning.
 *
 *  known compiler quirk...
 *      _upper() uses two separate comparisons rather than a single
 *      >= because the compiler fails to convert 'a' (the boundary
 *      value) when using c >= 'a'.  Using c < 'a' as the first test
 *      avoids the boundary condition entirely.
 *
 *  known transfer note...
 *      _eof_marker (0x1A) is computed at runtime to avoid a literal
 *      0x1A byte in the COM image, which causes TeraTerm 4 (Windows
 *      text mode XMODEM send) to truncate the transfer at that offset.
 *      TeraTerm 5 or xsend.py open the file in binary mode and do not
 *      have this problem.
 */

#include "stdio.h"

#define NBUFS   5           /* max files open simultaneously            */
#define LGH     512         /* per-slot data buffer size                */
#define BUFLGH  (LGH + 36) /* buffer + FCB                             */
#define SECTOR  512         /* BDOS DMA transfer size                   */

#define MFREE   11387       /* slot free                                */
#define MREAD   22489       /* slot open for reading                    */
#define MWRITE  17325       /* slot open for writing                    */

#define CMDBUF      0xA0    /* pointer to command line buffer           */
#define CMDSIZ      0xA2    /* command line size                        */
#define COMM_BUF    0xA4    /* pointer to common DMA staging buffer     */
#define COMM_FCB    0xAC    /* pointer to common FCB staging area       */
#define FREEMEM     0xA6    /* pointer to top of heap                   */
#define MEMLIMIT    0xB0    /* pointer to memory limit                  */

#define NAMSIZ      11      /* FCB name + extension field width         */
#define EXTSIZ       3      /* extension width                          */
#define FCBSIZ      36      /* total FCB size                           */

/* FCB field offsets - low byte of each big-endian word                 */
#define FCB_FSB    13   /* file starting block                          */
#define FCB_FSZ    15   /* file size in sectors                         */
#define FCB_FLA    17   /* file load address                            */
#define FCB_LRBL   21   /* last record byte length                      */
#define FCB_CBN    25   /* current block number                         */
#define FCB_CRN    27   /* current record number                        */
#define FCB_RELB   29   /* relative block number                        */
#define FCB_RELR   31   /* relative record number                       */
#define FCB_LBUFCNT 35  /* bytes in last buffer                         */

/* ------------------------------------------------------------------ */
/*  Globals                                                            */
/* ------------------------------------------------------------------ */

int _eof_marker;    /* = 26 = 0x1A, computed at runtime - see notes    */

int    _argc;
char **_argv;
unsigned int _heaptop;
unsigned int _memlimit;
int   *fmptr;
int   *limptr;
int    _current;

int  _ffcb[NBUFS];      /* per-slot: pointer to local FCB              */
int  _fnext[NBUFS];     /* per-slot: next byte pointer                 */
int  _ffirst[NBUFS];    /* per-slot: start of data buffer              */
int  _flast[NBUFS];     /* per-slot: end of valid data in buffer       */
char *_dfltbuf;         /* default DMA reset target                    */
int  _fmode[NBUFS];     /* per-slot mode: MFREE/MREAD/MWRITE           */

char *_commbuf;         /* physical address of common DMA buffer       */
char *_commfcb;         /* physical address of common FCB buffer       */

int _argcval;           /* argument count for _setargc in call.a99     */

/* ------------------------------------------------------------------ */
/*  Common-memory staging                                              */
/* ------------------------------------------------------------------ */

_stage_fcb(index)   int index;
{
    char *src, *dst;  int i;
    src = _ffcb[index];  dst = _commfcb;  i = FCBSIZ;
    while (i--) *dst++ = *src++;
}

_unstage_fcb(index)   int index;
{
    char *src, *dst;  int i;
    src = _commfcb;  dst = _ffcb[index];  i = FCBSIZ;
    while (i--) *dst++ = *src++;
}

_cpm_fcb(func, index)   int func, index;
{
    int result;
    _stage_fcb(index);
    result = _cpm(func, _commfcb);
    _unstage_fcb(index);
    return result;
}

/* ------------------------------------------------------------------ */
/*  Startup                                                            */
/* ------------------------------------------------------------------ */

_main()
{
    int *p;
    int  i;

    fmptr  = FREEMEM;
    limptr = MEMLIMIT;

    p = COMM_FCB;  _commfcb = *p;
    p = COMM_BUF;  _commbuf = *p;
    _dfltbuf = COMM_BUF;

    _heaptop  = *fmptr;
    _memlimit = *limptr;

    /* 0x1A computed at runtime - no literal in binary image            */
    _eof_marker = 0x1B;
    _eof_marker--;

    i = NBUFS;
    while (i--) _fmode[i] = MFREE;

    _setargs();
    main(_argc, _argv);
    exit(0);
}

_setargs()
{
    char *inname, *outname;
    int   count;
    char *lastc, *mode, *next, *ptr;
    int  *vptr;

    vptr = CMDBUF;  ptr   = *vptr;
    vptr = CMDSIZ;  count = *vptr;

    lastc  = ptr + count - 1;
    *lastc = SPACE;
    _argv    = alloc(30);
    _argv[0] = next = alloc(count + 2);
    *next++  = NULL;
    _argc    = 0;
    inname   = outname = NULL;

    while (++ptr < lastc) {
        if (*ptr == SPACE) continue;
        if (*ptr == '<') {
            while (*++ptr == SPACE);
            inname = next;
        } else if (*ptr == '>') {
            if (ptr[1] == '>') { ++ptr; mode = "a"; }
            else                {        mode = "w"; }
            while (*++ptr == SPACE);
            outname = next;
        } else {
            _argv[_argc++] = next;
        }
        while (*ptr != SPACE) *next++ = *ptr++;
        *next++ = NULL;
    }
    _argv[_argc] = 0;
    _redirect(inname,  "r", stdin);
    _redirect(outname, mode, stdout);
}

_redirect(filename, mode, std)
    char *filename; char *mode; int *std;
{
    if (filename) {
        if ((*std = fopen(filename, mode)) == 0) {
            err("CAN'T REDIRECT");
            exit(-1);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Memory management                                                  */
/* ------------------------------------------------------------------ */

alloc(b)   int b;
{
    if (b & 1) b++;
    _heaptop += b;
    if (_heaptop > _memlimit) return -1;
    return (_heaptop - b);
}

free(addr)   int addr;  { _heaptop = addr; }

avail()  { return (_memlimit - _heaptop); }

/* ------------------------------------------------------------------ */
/*  Error                                                              */
/* ------------------------------------------------------------------ */

err(s)   char *s;
{
    int str;
    puts("\nERROR: ");
    puts(s);
    str = _current;
    while (str) {
        puts("\ncalled by ");
        puts(*(str + 1));
        str = *str;
    }
}

/* ------------------------------------------------------------------ */
/*  Console I/O                                                        */
/* ------------------------------------------------------------------ */

getchar()               { return getc(stdin); }
putchar(c)  char c;     { return (putc(c, stdout)); }

puts(buf)   char *buf;
{
    char c;
    while (c = *buf++) putchar(c);
}

/* ------------------------------------------------------------------ */
/*  FCB initialisation                                                 */
/* ------------------------------------------------------------------ */

_newfcb(name, fcb)   char *name, *fcb;
{
    char c;
    int  i;

    /* zero entire FCB, then space-fill name+extension fields           */
    i = FCBSIZ;
    while (i) fcb[i--] = 0;
    for (i = 0; i < NAMSIZ; i++) fcb[i] = ' ';

    if (*name == 0) return 0;

    /* skip optional drive prefix e.g. "A:"                             */
    if (name[1] == ':') name += 2;
    if (*name == 0) return 0;

    /* copy name into bytes 0-7                                         */
    i = 0;
    while ((c = _upper(*name++)) && c != '.') {
        if (i < NAMSIZ - EXTSIZ) fcb[i++] = c;
    }

    /* copy extension into bytes 8-10                                   */
    if (c == '.') {
        i = NAMSIZ - EXTSIZ;
        while ((c = _upper(*name++)) && i < NAMSIZ)
            fcb[i++] = c;
    }

    return 1;
}

/* ------------------------------------------------------------------ */
/*  File open / close                                                  */
/* ------------------------------------------------------------------ */

fopen(name, mode)   char *name, *mode;
{
    char  c;
    char *fcb;
    int   index, unit, r15;

    index = NBUFS;
    while (index--) { if (_fmode[index] == MFREE) break; }
    if (index == -1) { err("NO BUFFERS"); exit(-1); }
    unit = index + 5;

    if (_ffcb[index] == 0) _ffcb[index] = alloc(BUFLGH);
    _ffirst[index] = _ffcb[index] + FCBSIZ;
    _flast[index]  = _ffirst[index] + LGH;
    fcb            = _ffcb[index];

    if (_newfcb(name, fcb) == 0) return 0;

    c = _upper(*mode);

    if (c == 'R' || c == 'A') {
        r15 = _cpm_fcb(15, index);
        if (r15 < 0 || r15 == 0xFF) return 0;

        _fmode[index] = MREAD;
        _fnext[index] = _flast[index];  /* force immediate buffer fill  */

        if (c == 'A') {
            while (getc(unit) != -1);   /* read to EOF, BDOS CRN ready  */

            /* CRN (big-endian word, low byte at FCB_CRN=27) points one  */
            /* past the last sector after reading to EOF.  Decrement by  */
            /* 1 so WRSEQ rewrites the last sector in place, preserving  */
            /* existing data up to the ^Z and appending after it.        */
            if (fcb[FCB_CRN] == 0) {
                fcb[FCB_CRN]     = 0xFF;
                fcb[FCB_CRN - 1] = fcb[FCB_CRN - 1] - 1;
            } else {
                fcb[FCB_CRN] = fcb[FCB_CRN] - 1;
            }

            /* back _fnext onto the ^Z so putb overwrites it             */
            if (_fnext[index] > _ffirst[index])
                _fnext[index] = _fnext[index] - 1;

            _fmode[index] = MWRITE;
        }
        return unit;

    } else if (c == 'W') {
        _cpm_fcb(19, index);            /* erase existing file          */
        if (_cpm_fcb(22, index) < 0) return 0;
        _fmode[index] = MWRITE;
        _fnext[index] = _ffirst[index];
        return unit;
    }

    return 0;
}

fclose(unit)   int unit;
{
    int   index, werror, nbytes;
    char *end, *ptr, *fcb;

    index = unit - 5;
    if (_fchk(index) == MREAD) { _fmode[index] = MFREE; return 1; }

    /* bytes of valid data in the last (possibly partial) sector        */
    nbytes = _fnext[index] - _ffirst[index];
    if (nbytes == 0) nbytes = SECTOR;

    /* pad remainder with ^Z for text-mode compatibility                */
    ptr = _fnext[index];
    end = _flast[index];
    while (ptr < end) *ptr++ = _eof_marker;

    werror = fflush(unit);

    /* set LBUFCNT so BDOS knows exact byte count in last sector        */
    fcb = _ffcb[index];
    fcb[FCB_LBUFCNT] = nbytes;

    _fmode[index] = MFREE;
    if ((_cpm_fcb(16, index) < 0) || werror) return 0;
    return 1;
}

_fchk(index)   int index;
{
    int i;
    if ((index >= 0) & (index < NBUFS)) {
        i = _fmode[index];
        if ((i == MREAD) | (i == MWRITE)) return i;
    }
    err("INVALID UNIT NUMBER");
    exit(-1);
}

/* ------------------------------------------------------------------ */
/*  Buffered read                                                      */
/* ------------------------------------------------------------------ */

_getbuf(index)   int index;
{
    char *fcb, *dst, *src;
    int   sectors_read, i, result, lbufcnt;

    fcb          = _ffcb[index];
    dst          = _ffirst[index];
    sectors_read = 0;

    while (sectors_read < (LGH / SECTOR)) {
        _stage_fcb(index);
        _cpm(26, _commbuf);
        result = _cpm(20, _commfcb);
        _unstage_fcb(index);
        if (result) break;              /* non-zero = EOF               */

        /* LBUFCNT: if set and < SECTOR, this is the last partial sector */
        lbufcnt = fcb[FCB_LBUFCNT] & 0xFF;
        if (lbufcnt == 0 || lbufcnt >= SECTOR) lbufcnt = SECTOR;

        src = _commbuf;  i = lbufcnt;
        while (i--) *dst++ = *src++;
        sectors_read++;

        if (lbufcnt < SECTOR) break;    /* partial = last sector        */
    }

    _cpm(26, *_dfltbuf);
    if (sectors_read == 0) return -1;
    _flast[index] = dst;                /* marks end of valid data      */
    return _ffirst[index];
}

/* ------------------------------------------------------------------ */
/*  Buffered write                                                     */
/* ------------------------------------------------------------------ */

fflush(unit)   int unit;
{
    int   index, i;
    char *next, *going, *src, *dst;

    index = unit - 5;
    if (_fchk(index) != MWRITE) { err("CAN'T FLUSH"); exit(-1); }

    next  = _fnext[index];
    going = _fnext[index] = _ffirst[index];

    while (going < next) {
        src = going;  dst = _commbuf;  i = SECTOR;
        while (i--) *dst++ = *src++;

        _cpm(26, _commbuf);
        _stage_fcb(index);
        if (_cpm(21, _commfcb)) {
            _unstage_fcb(index);
            _cpm(26, *_dfltbuf);
            return -1;
        }
        _unstage_fcb(index);
        going += SECTOR;
    }

    _cpm(26, *_dfltbuf);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Byte-level I/O                                                     */
/* ------------------------------------------------------------------ */

getc(unit)   int unit;
{
    int c;
    while ((c = getb(unit)) == LF) {}
    if (c == _eof_marker) return -1;    /* ^Z fallback for old files    */
    return c;
}

getb(unit)   int unit;
{
    int   c, index;
    char *next;

    if (unit == 0) {
        c = _cpm(1, 0);
        if (c == '\n') _cpm(2, LF);
        return c;
    }

    index = unit - 5;
    if (_fchk(index) != MREAD) { err("CAN'T READ OUTFILE"); exit(-1); }

    next = _fnext[index];
    if (next == _flast[index]) {
        next = _getbuf(index);
        if (next == -1) return -1;
    }

    c = (*next++) & 0xff;
    _fnext[index] = next;
    return c;
}

putc(c, unit)   char c; int unit;
{
    int ret;
    ret = putb(c, unit);
    if (ret == '\n') ret = putb(LF, unit);
    return ret;
}

putb(c, unit)   char c; int unit;
{
    int   werror, index;
    char *next;

    if (unit == 1) { _cpm(2, c); return c; }

    index = unit - 5;
    if (_fchk(index) != MWRITE) { err("CAN'T WRITE TO INFILE"); exit(-1); }

    werror = (_fnext[index] == _flast[index]) ? fflush(unit) : 0;
    next   = _fnext[index];
    *next++ = c;
    _fnext[index] = next;
    return werror ? werror : c;
}

/* ------------------------------------------------------------------ */
/*  Utilities                                                          */
/* ------------------------------------------------------------------ */

_upper(c)   char c;
{
    if (c < 'a') return c;
    if (c > 'z') return c;
    return c - 32;
}

exit(value)   int value;
{
    int index;
    index = NBUFS;
    while (index--) {
        if (_fmode[index] == MWRITE) fclose(index + 5);
    }
    _shell(value);
}
