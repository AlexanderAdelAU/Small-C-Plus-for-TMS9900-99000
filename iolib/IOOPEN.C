/*
** IOOPEN.C -- FCB construction, open, close and delete for split IOLIB99
*/
#include "stdio.h"

/* Constants are local deliberately: Small-C/Plus 1.06a has limited include handling. */
#define NBUFS   5
#define LGH     512
#define BUFLGH  (LGH + 36)
#define SECTOR  512
#define MFREE   11387
#define MREAD   22489
#define MWRITE  17325
#define COMM_FCB    0xAC
#define NAMSIZ      11
#define EXTSIZ       3
#define FCBSIZ      36
#define FCB_CRN      27
#define FCB_LBUFCNT  35

extern int _eof_marker;
extern int _ffcb[NBUFS];
extern int _fnext[NBUFS];
extern int _ffirst[NBUFS];
extern int _flast[NBUFS];
extern int _fmode[NBUFS];
extern int _ferr[NBUFS];
extern char *_commfcb;

_upper(c) char c; {
  if(c < 'a') return c;
  if(c > 'z') return c;
  return c - 32;
  }

_newfcb(name, fcb) char *name, *fcb; {
  char c;
  int i;

  i = FCBSIZ;
  while(i--) fcb[i] = 0;
  for(i = 0; i < NAMSIZ; i++) fcb[i] = ' ';

  if(*name == 0) return 0;
  if(name[1] == ':') name += 2;
  if(*name == 0) return 0;

  i = 0;
  while((c = _upper(*name++)) && c != '.') {
    if(i < NAMSIZ - EXTSIZ) fcb[i++] = c;
    }

  if(c == '.') {
    i = NAMSIZ - EXTSIZ;
    while((c = _upper(*name++)) && i < NAMSIZ)
      fcb[i++] = c;
    }
  return 1;
  }

fopen(name, mode) char *name, *mode; {
  char c;
  char *fcb;
  int index, unit, r15;

  index = NBUFS;
  while(index--) if(_fmode[index] == MFREE) break;
  if(index == -1) {
    err("NO BUFFERS");
    exit(-1);
    }

  unit = index + 5;
  _ferr[index] = 0;

  if(_ffcb[index] == 0) {
    _ffcb[index] = alloc(BUFLGH);
    if(_ffcb[index] == -1) return 0;
    }

  _ffirst[index] = _ffcb[index] + FCBSIZ;
  _flast[index] = _ffirst[index] + LGH;
  fcb = _ffcb[index];

  if(_newfcb(name, fcb) == 0) return 0;
  c = _upper(*mode);

  if(c == 'R' || c == 'A') {
    r15 = _cpm_fcb(15, index);
    if(r15 < 0 || r15 == 0xFF) return 0;

    _fmode[index] = MREAD;
    _fnext[index] = _flast[index];

    if(c == 'A') {
      while(getc(unit) != -1);

      if(fcb[FCB_CRN] == 0) {
        fcb[FCB_CRN] = 0xFF;
        fcb[FCB_CRN - 1] = fcb[FCB_CRN - 1] - 1;
        }
      else fcb[FCB_CRN] = fcb[FCB_CRN] - 1;

      if(_fnext[index] > _ffirst[index])
        _fnext[index] = _fnext[index] - 1;
      _fmode[index] = MWRITE;
      }
    return unit;
    }

  if(c == 'W') {
    _cpm_fcb(19, index);
    if(_cpm_fcb(22, index) < 0) return 0;
    _fmode[index] = MWRITE;
    _fnext[index] = _ffirst[index];
    return unit;
    }

  return 0;
  }

/* Delete uses the same common-memory FCB staging address as fopen. */
delete(name) char *name; {
  char fcb[FCBSIZ];
  char *src, *dst, *commfcb;
  int *p;
  int i, result;

  if(_newfcb(name, fcb) == 0) return -1;

  p = COMM_FCB;
  commfcb = *p;
  src = fcb;
  dst = commfcb;
  i = FCBSIZ;
  while(i--) *dst++ = *src++;

  result = _cpm(19, commfcb);
  if(result < 0 || result == 0xFF) return -1;
  return 0;
  }

fclose(unit) int unit; {
  int index, werror, nbytes;
  char *end, *ptr, *fcb;

  index = unit - 5;
  if(_fchk(index) == MREAD) {
    _fmode[index] = MFREE;
    return 1;
    }

  nbytes = _fnext[index] - _ffirst[index];
  if(nbytes == 0) nbytes = SECTOR;

  ptr = _fnext[index];
  end = _flast[index];
  while(ptr < end) *ptr++ = _eof_marker;

  werror = fflush(unit);
  fcb = _ffcb[index];
  fcb[FCB_LBUFCNT] = nbytes;

  _fmode[index] = MFREE;
  if((_cpm_fcb(16, index) < 0) || werror) {
    _ferr[index] = 1;
    return 0;
    }
  return 1;
  }
