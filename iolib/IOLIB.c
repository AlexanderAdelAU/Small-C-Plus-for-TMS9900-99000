/*  name...
 *      iolib
 *
 *  purpose...
 *      to provide a "standard" interface between C
 *      programs and the CP/M I/O system.
 *
 *  notes...
 *      Compile using -M option.
 *
 *  refactor notes...
 *      All BDOS FCB and DMA operations are now routed through two
 *      staging areas in COMMON memory:
 *
 *        _commfcb  (physical addr from *COMM_FCB @ 0xAC)
 *            - One shared FCB copy, valid only for the duration of
 *              a single BDOS call.  _stage_fcb() copies the per-slot
 *              FCB into common before the call; _unstage_fcb() copies
 *              it back afterward so CP/M's updates (extent, record
 *              counter, etc.) are preserved in the slot.
 *
 *        _commbuf  (physical addr from *COMM_BUF @ 0xA4)
 *            - One shared 128-byte DMA window.  Reads: BDOS fills
 *              _commbuf, then _getbuf copies it into the per-slot
 *              local buffer 128 bytes at a time.  Writes: fflush
 *              copies 128 bytes from the local buffer into _commbuf,
 *              then asks BDOS to write.
 *
 *      Per-slot data buffers (_ffirst / _flast / _fnext) and the
 *      per-slot FCB copies (_ffcb[]) stay in local segment memory and
 *      are never handed directly to BDOS.  This lets iolib live in
 *      any segment while BDOS remains in segment 0.
 */

#include "stdio.h"

/* #define INT_OFF */

#define NBUFS   5
#define LGH     512
#define BUFLGH  (LGH + 36)

#define SECTOR  128     /* CP/M physical sector size (DMA window) */

#define MFREE   11387
#define MREAD   22489
#define MWRITE  17325
#define DIR

#define CMDBUF      0xA0
#define CMDSIZ      0xA2
#define COMM_BUF    0xA4    /* pointer to common DMA staging buffer  */
#define COMM_FCB    0xAC    /* pointer to common FCB staging area    */
#define FREEMEM     0xA6
#define MEMLIMIT    0xB0
#define STACKLIMIT  0x0100

#define NAMSIZ  11
#define FCBSIZ  36

/* ------------------------------------------------------------------ */
/*  Globals                                                            */
/* ------------------------------------------------------------------ */

int    _argc;
char **_argv;
unsigned int _heaptop;
unsigned int _memlimit;
int   *fmptr;
int   *limptr;
int    _current;
int    _dfltdsk;

int  _ffcb[NBUFS];      /* per-slot: pointer to local FCB copy       */
int  _fnext[NBUFS];     /* per-slot: next byte pointer               */
int  _ffirst[NBUFS];    /* per-slot: start of local data buffer      */
int  _flast[NBUFS];     /* per-slot: end of local data buffer        */
char *_dfltbuf;         /* default DMA address (reset target)        */

int  _fmode[NBUFS] = { MFREE, MFREE, MFREE, MFREE, MFREE };

/*
 * Pointers to the two COMMON staging areas.
 * Resolved once in _main() from the addresses stored at COMM_BUF / COMM_FCB.
 */
char *_commbuf;   /* physical address of common DMA buffer  */
char *_commfcb;   /* physical address of common FCB buffer  */

int _ex, _cr;
//int _argcval;

/* ------------------------------------------------------------------ */
/*  Common-memory staging helpers                                      */
/* ------------------------------------------------------------------ */

/*
 * _stage_fcb - copy per-slot FCB into common FCB before a BDOS call.
 *              BDOS (in seg 0) can only see _commfcb; it must never
 *              receive a pointer into local segment memory.
 */
_stage_fcb(index)
    int index;
{
    char *src, *dst;
    int   i;

    src = _ffcb[index];
    dst = _commfcb;
    i   = FCBSIZ;
    while (i--)
        *dst++ = *src++;
}

/*
 * _unstage_fcb - copy common FCB back to the per-slot copy after a
 *                BDOS call so that CP/M's field updates (extent,
 *                current record, etc.) are not lost.
 */
_unstage_fcb(index)
    int index;
{
    char *src, *dst;
    int   i;

    src = _commfcb;
    dst = _ffcb[index];
    i   = FCBSIZ;
    while (i--)
        *dst++ = *src++;
}

/*
 * _cpm_fcb - stage the FCB, call BDOS function 'func', then unstage.
 *            Use this for every BDOS call that takes an FCB argument.
 *            Returns the BDOS result.
 */
_cpm_fcb(func, index)
    int func, index;
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
    fmptr    = FREEMEM;
    limptr   = MEMLIMIT;

    /* Resolve the physical addresses of the two common staging areas.    */
    /* Compiler cannot dereference cast literals; use int* intermediates  */
    /* exactly as fmptr/limptr are used above - assign address, then read.*/
    {
        int *p;
        p        = COMM_FCB;  _commfcb = *p;  /* char* <- *(int*)0xAC   */
        p        = COMM_BUF;  _commbuf = *p;  /* char* <- *(int*)0xA4   */
    }
    _dfltbuf  = COMM_BUF;           /* kept for DMA-reset idiom           */

    _heaptop  = *fmptr;
    _memlimit = *limptr;
    _dfltdsk  = _cpm(25, 0);
    _setargs();
    main(_argc, _argv);
    exit(0);
}

_setargs()
{
    char *inname, *outname;
    int   count;
    char *lastc;
    char *mode;
    char *next;
    char *ptr;
    int  *vptr;

    vptr  = CMDBUF;
    ptr   = *vptr;
    vptr  = CMDSIZ;
    count = *vptr;

    lastc  = ptr + count - 1;
    *lastc = SPACE;
    _argv    = alloc(30);
    _argv[0] = next = alloc(count + 2);
    *next++  = NULL;
    _argc    = 0;
    inname   = outname = NULL;

    while (++ptr < lastc) {
        if (*ptr == SPACE)
            continue;
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
        while (*ptr != SPACE)
            *next++ = *ptr++;
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

alloc(b)
    int b;
{
    if (b & 1) b++;
    _heaptop += b;
    if (_heaptop > _memlimit)
        return -1;
    return (_heaptop - b);
}

free(addr)
    int addr;
{
    _heaptop = addr;
}

avail()
{
    return (_memlimit - _heaptop);
}

/* ------------------------------------------------------------------ */
/*  Error / diagnostics                                                */
/* ------------------------------------------------------------------ */

err(s)
    char *s;
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

/* debug: dump 'size' bytes from 'fcb' as hex to stdout */
print_hex(fcb, size)
    char *fcb; int size;
{
    int i;
    for (i = 0; i < size; i++)
        printf("%02x ", fcb[i]);
    printf("\n");
}

/* ------------------------------------------------------------------ */
/*  Console I/O                                                        */
/* ------------------------------------------------------------------ */

getchar()
{
    return getc(stdin);
}

putchar(c)
    char c;
{
    return (putc(c, stdout));
}

puts(buf)
    char *buf;
{
    char c;
    while (c = *buf++)
        putchar(c);
}

/* ------------------------------------------------------------------ */
/*  FCB helpers                                                        */
/* ------------------------------------------------------------------ */

/*
 * _newfcb - initialise a local per-slot FCB from a filename string.
 *           The FCB pointed to by 'fcb' lives in local segment memory;
 *           it is only staged to common memory immediately before a
 *           BDOS call.
 */
_newfcb(name, fcb)
    char *name, *fcb;
{
    char c;
    int  i;

    i = NAMSIZ;
    while (i)
        fcb[i--] = ' ';

    i = NAMSIZ + 1;
    while (i < FCBSIZ)
        fcb[i++] = 0;

    if (name[1] == ':') {
        fcb[33] = *name & 0xf;
        name += 2;
    } else {
        fcb[33] = _dfltdsk;
    }

    if (*name == 0)
        return 0;

    i = 0;
    while (c = _upper(*name++)) {
        if (c == '.') break;
        fcb[i++] = c;
    }

    if (c == '.') {
        i = 9;
        while ((c = _upper(*name++)) && i < NAMSIZ + 1)
            fcb[i++] = c;
    }

    return (c == 0) ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/*  File open / close                                                  */
/* ------------------------------------------------------------------ */

fopen(name, mode)
    char *name, *mode;
{
    char  c;
    char *fcb;
    int   index, unit;

    index = NBUFS;
    while (index--) {
        if (_fmode[index] == MFREE)
            break;
    }
    if (index == -1) {
        err("NO BUFFERS");
        exit(-1);
    }
    unit = index + 5;

    if (_ffcb[index] == 0)
        _ffcb[index] = alloc(BUFLGH);

    _ffirst[index] = _ffcb[index] + FCBSIZ;
    _flast[index]  = _ffirst[index] + LGH;
    fcb            = _ffcb[index];

    if (_newfcb(name, fcb) == 0)
        return 0;

    c = _upper(*mode);

    if (c == 'R' || c == 'A') {
        /* Open for reading via common FCB */
        if (_cpm_fcb(15, index) < 0)
            return 0;

        _fmode[index] = MREAD;
        _fnext[index] = _flast[index];  /* force immediate buffer fill */

        if (c == 'A') {
            while (getc(unit) != -1);   /* read to EOF */
            fcb[12] = _ex;
            _cpm_fcb(15, index);        /* re-open at that extent      */
            fcb[32] = _cr;
            _fmode[index] = MWRITE;
        }
        return unit;

    } else if (c == 'W') {
        _cpm_fcb(19, index);            /* delete existing file        */
        if (_cpm_fcb(22, index) < 0)    /* create new file             */
            return 0;
        _fmode[index] = MWRITE;
        _fnext[index] = _ffirst[index];
        return unit;
    }

    return 0;
}

fclose(unit)
    int unit;
{
    int   index, werror;
    char *end, *ptr;

    index = unit - 5;
    if (_fchk(index) == MREAD) {
        _fmode[index] = MFREE;
        return 1;
    }

    putb(26, unit);

    ptr = _fnext[index];
    end = _flast[index];
    while (ptr < end)
        *ptr++ = 26;

    werror = fflush(unit);
    _fmode[index] = MFREE;

    if ((_cpm_fcb(16, index) < 0) || werror)
        return 0;
    return 1;
}

_fchk(index)
    int index;
{
    int i;
    if ((index >= 0) & (index < NBUFS)) {
        i = _fmode[index];
        if ((i == MREAD) | (i == MWRITE))
            return i;
    }
    err("INVALID UNIT NUMBER");
    exit(-1);
}

/* ------------------------------------------------------------------ */
/*  Buffered read path                                                 */
/*                                                                     */
/*  _getbuf reads SECTOR (128) bytes at a time from BDOS into the     */
/*  common DMA buffer, then copies each sector into the per-slot      */
/*  local buffer.  BDOS never sees a local-segment pointer.           */
/* ------------------------------------------------------------------ */

_getbuf(index)
    int index;
{
    char *fcb, *dst, *src;
    int   sectors_read, i;

    fcb = _ffcb[index];
    _ex = fcb[12];
    _cr = fcb[32];

    dst          = _ffirst[index];
    sectors_read = 0;

    while (sectors_read < (LGH / SECTOR)) {
        /* Point BDOS DMA at the common staging buffer */
        _cpm(26, _commbuf);

        /* Read one sector via staged FCB */
        _stage_fcb(index);
        if (_cpm(20, _commfcb)) {   /* non-zero = EOF or error */
            _unstage_fcb(index);
            break;
        }
        _unstage_fcb(index);

        /* Copy sector from common buffer to local per-slot buffer */
        src = _commbuf;
        i   = SECTOR;
        while (i--)
            *dst++ = *src++;

        sectors_read++;
    }

    /* Restore default DMA pointer */
    _cpm(26, *_dfltbuf);

    if (sectors_read == 0)
        return -1;

    _flast[index] = _ffirst[index] + (sectors_read * SECTOR);
    return _ffirst[index];
}

/* ------------------------------------------------------------------ */
/*  Buffered write path                                                */
/*                                                                     */
/*  fflush writes the local buffer to disk SECTOR bytes at a time,    */
/*  staging each chunk through the common DMA buffer so BDOS always   */
/*  receives a pointer into common (segment-0-visible) memory.        */
/* ------------------------------------------------------------------ */

fflush(unit)
    int unit;
{
    int   index, i;
    char *next, *going, *src, *dst;

    index = unit - 5;
    if (_fchk(index) != MWRITE) {
        err("CAN\'T FLUSH");
        exit(-1);
    }

    next  = _fnext[index];
    going = _fnext[index] = _ffirst[index];

    while (going < next) {
        /* Copy one sector from local buffer into common staging buffer */
        src = going;
        dst = _commbuf;
        i   = SECTOR;
        while (i--)
            *dst++ = *src++;

        /* Set DMA to common buffer and write via staged FCB */
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
/*  Byte-level read / write                                            */
/* ------------------------------------------------------------------ */

getc(unit)
    int unit;
{
    int c;
    while ((c = getb(unit)) == LF) {}
    if (c == 26) {
        if (unit >= 5)
            _fnext[unit - 5];
        return -1;
    }
    return c;
}

getb(unit)
    int unit;
{
    int   c, index;
    char *next;

    if (unit == 0) {
        c = _cpm(1, 0);
        if (c == '\n')
            _cpm(2, LF);
        return c;
    }

    index = unit - 5;
    if (_fchk(index) != MREAD) {
        err("CAN\'T READ OUTFILE");
        exit(-1);
    }

    next = _fnext[index];
    if (next == _flast[index]) {
        next = _getbuf(index);
        if (next == -1)
            return -1;
    }

    c = (*next++) & 0xff;
    _fnext[index] = next;
    return c;
}

putc(c, unit)
    char c; int unit;
{
    int ret;
    ret = putb(c, unit);
    if (ret == '\n')
        ret = putb(LF, unit);
    return ret;
}

putb(c, unit)
    char c; int unit;
{
    int   werror, index;
    char *next;

    if (unit == 1) {
        _cpm(2, c);
        return c;
    }

    index = unit - 5;
    if (_fchk(index) != MWRITE) {
        err("CAN\'T WRITE TO INFILE");
        exit(-1);
    }

    werror = (_fnext[index] == _flast[index]) ? fflush(unit) : 0;

    next = _fnext[index];
    *next++ = c;
    _fnext[index] = next;

    return werror ? werror : c;
}

/* ------------------------------------------------------------------ */
/*  Utilities                                                          */
/* ------------------------------------------------------------------ */

_upper(c)
    char c;
{
    return (c >= 'a') ? c - 32 : c;
}

exit(value)
    int value;
{
    int index;
    index = NBUFS;
    while (index--) {
        if (_fmode[index] == MWRITE)
            fclose(index + 5);
    }
    _shell(value);
}
