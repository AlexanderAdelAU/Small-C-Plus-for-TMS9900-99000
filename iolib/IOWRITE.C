/*
** IOWRITE.C -- buffered and raw output for split IOLIB99
*/
#include "stdio.h"

/* Constants are local deliberately: Small-C/Plus 1.06a has limited include handling. */
#define NBUFS   5
#define SECTOR  512
#define MWRITE  17325

extern int _fnext[NBUFS];
extern int _ffirst[NBUFS];
extern int _flast[NBUFS];
extern int _ferr[NBUFS];
extern char *_dfltbuf;
extern char *_commbuf;
extern char *_commfcb;

fflush(unit) int unit; {
  int index, i;
  char *next, *going, *src, *dst;

  index = unit - 5;
  if(_fchk(index) != MWRITE) {
    err("CAN'T FLUSH");
    exit(-1);
    }

  next = _fnext[index];
  going = _fnext[index] = _ffirst[index];

  while(going < next) {
    src = going;
    dst = _commbuf;
    i = SECTOR;
    while(i--) *dst++ = *src++;

    _cpm(26, _commbuf);
    _stage_fcb(index);
    if(_cpm(21, _commfcb)) {
      _unstage_fcb(index);
      _cpm(26, *_dfltbuf);
      _ferr[index] = 1;
      return -1;
      }
    _unstage_fcb(index);
    going += SECTOR;
    }

  _cpm(26, *_dfltbuf);
  return 0;
  }

putc(c, unit) char c; int unit; {
  int ret;
  ret = putb(c, unit);
  if(ret == '\n') ret = putb(LF, unit);
  return ret;
  }

putb(c, unit) char c; int unit; {
  int werror, index;
  char *next;

  if(unit == stdout || unit == stderr) {
    _cpm(2, c);
    return c;
    }

  index = unit - 5;
  if(_fchk(index) != MWRITE) {
    err("CAN'T WRITE TO INFILE");
    exit(-1);
    }

  werror = (_fnext[index] == _flast[index]) ? fflush(unit) : 0;
  if(werror) {
    _ferr[index] = 1;
    return -1;
    }

  next = _fnext[index];
  *next++ = c;
  _fnext[index] = next;
  return c;
  }

putchar(c) char c; {
  return putc(c, stdout);
  }

puts(buf) char *buf; {
  char c;
  while(c = *buf++) putchar(c);
  }

fputc(c, unit) char c; int unit; {
  return putc(c, unit);
  }

fputs(buf, unit) char *buf; int unit; {
  char c;
  while(c = *buf++) putc(c, unit);
  }

/* Raw binary block write: never perform CR/LF translation. */
write(unit, buf, n) int unit, n; char *buf; {
  int count;
  count = n;
  while(n--) {
    if(putb(*buf++, unit) == -1) return -1;
    }
  return count;
  }
