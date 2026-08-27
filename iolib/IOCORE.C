/*
** IOCORE.C -- startup, heap and shared state for split IOLIB99
*/
#include "stdio.h"

/*
** Small-C/Plus 1.06a split-build note:
** keep compile-time constants in this source file.  Do not include IOINT.H
** here: IOCORE owns the shared storage and must not also emit EXT symbols.
*/
#define NBUFS   5
#define MFREE   11387
#define MREAD   22489
#define MWRITE  17325
#define CMDBUF      0xA0
#define CMDSIZ      0xA2
#define COMM_BUF    0xA4
#define FREEMEM     0xA6
#define COMM_FCB    0xAC
#define MEMLIMIT    0xB0
#define FCBSIZ      36

int _eof_marker;
int _argc;
char **_argv;
unsigned int _heaptop;
unsigned int _memlimit;
int *fmptr;
int *limptr;
int _current;

int _ffcb[NBUFS];
int _fnext[NBUFS];
int _ffirst[NBUFS];
int _flast[NBUFS];
char *_dfltbuf;
int _fmode[NBUFS];
int _ferr[NBUFS];

char *_commbuf;
char *_commfcb;

_stage_fcb(index) int index; {
  char *src, *dst;
  int i;
  src = _ffcb[index];
  dst = _commfcb;
  i = FCBSIZ;
  while(i--) *dst++ = *src++;
  }

_unstage_fcb(index) int index; {
  char *src, *dst;
  int i;
  src = _commfcb;
  dst = _ffcb[index];
  i = FCBSIZ;
  while(i--) *dst++ = *src++;
  }

_cpm_fcb(func, index) int func, index; {
  int result;
  _stage_fcb(index);
  result = _cpm(func, _commfcb);
  _unstage_fcb(index);
  return result;
  }

_main() {
  int *p;
  int i;

  fmptr = FREEMEM;
  limptr = MEMLIMIT;

  p = COMM_FCB;
  _commfcb = *p;

  p = COMM_BUF;
  _commbuf = *p;
  _dfltbuf = _commbuf;

  _heaptop = *fmptr;
  _memlimit = *limptr;

  _eof_marker = 0x1B;
  _eof_marker--;

  i = NBUFS;
  while(i--) {
    _fmode[i] = MFREE;
    _ferr[i] = 0;
    }

  _setargs();
  main(_argc, _argv);
  exit(0);
  }

_setargs() {
  char *inname, *outname;
  int count;
  char *lastc, *mode, *next, *ptr;
  int *vptr;

  vptr = CMDBUF;
  ptr = *vptr;
  vptr = CMDSIZ;
  count = *vptr;

  lastc = ptr + count - 1;
  *lastc = SPACE;
  _argv = alloc(30);
  _argv[0] = next = alloc(count + 2);
  *next++ = NULL;
  _argc = 0;
  inname = outname = NULL;

  while(++ptr < lastc) {
    if(*ptr == SPACE) continue;
    if(*ptr == '<') {
      while(*++ptr == SPACE);
      inname = next;
      }
    else if(*ptr == '>') {
      if(ptr[1] == '>') { ++ptr; mode = "a"; }
      else mode = "w";
      while(*++ptr == SPACE);
      outname = next;
      }
    else _argv[_argc++] = next;

    while(*ptr != SPACE) *next++ = *ptr++;
    *next++ = NULL;
    }

  _argv[_argc] = 0;
  _redirect(inname, "r", stdin);
  _redirect(outname, mode, stdout);
  }

_redirect(filename, mode, std)
  char *filename; char *mode; int *std; {
  if(filename) {
    if((*std = fopen(filename, mode)) == 0) {
      err("CAN'T REDIRECT");
      exit(-1);
      }
    }
  }


/* ------------------------------------------------------------------ */
/*  Memory management                                                  */
/* ------------------------------------------------------------------ */


/* alloc2 is the zero-filled allocator used by Hendrix MAC. */
alloc2(n, size) unsigned int n, size; {
  unsigned int bytes;
  int addr;
  char *p;

  bytes = n * size;
  if(size && (bytes / size) != n) return 0;

  addr = alloc(bytes);
  if(addr == -1) return 0;

  p = addr;
  while(bytes) {
    *p++ = 0;
    --bytes;
    }
  return addr;
  }


alloc(b)   unsigned int b;
{
    unsigned int base;

    if (b & 1) b++;

    base = _heaptop;

    if (base > _memlimit)
        return -1;

    if (b > (_memlimit - base))
        return -1;

    _heaptop = base + b;

    return base;
}

free(addr)   unsigned int addr;
{
    _heaptop = addr;
}

avail()
{
   /* if (_heaptop > _memlimit)
        return 0;
        */

    return (_memlimit - _heaptop);
}

heaptop()
{
    return _heaptop;
}

memlimit()
{
    return _memlimit;
}


_fchk(index) int index; {
  int i;
  if((index >= 0) & (index < NBUFS)) {
    i = _fmode[index];
    if((i == MREAD) | (i == MWRITE)) return i;
    }
  err("INVALID UNIT NUMBER");
  exit(-1);
  }

ferror(unit) int unit; {
  int index;
  if(unit == stdin || unit == stdout || unit == stderr) return 0;
  index = unit - 5;
  if(index < 0 || index >= NBUFS) return 1;
  return _ferr[index];
  }

err(s) char *s; {
  int str;
  puts("\nERROR: ");
  puts(s);
  str = _current;
  while(str) {
    puts("\ncalled by ");
    puts(*(str + 1));
    str = *str;
    }
  }

exit(value) int value; {
  int index;
  index = NBUFS;
  while(index--) {
    if(_fmode[index] == MWRITE) fclose(index + 5);
    }
  _shell(value);
  }
