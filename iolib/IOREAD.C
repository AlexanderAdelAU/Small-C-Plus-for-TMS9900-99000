/*
** IOREAD.C -- buffered input for split IOLIB99
**
** Updated Aug 2026:
**  - getc() now NORMALIZES line endings instead of blindly eating LF.
**    Rule (per file unit): an LF that immediately follows a CR is the
**    second half of a CRLF pair and is swallowed; a LONE LF is treated
**    as a line ending and returned as '\n' (== CR, 0x0D, in this
**    dialect).  Result: CRLF, LF-only and CR-only files all arrive as
**    identical CR-terminated lines, blank lines are preserved, and no
**    doubled blank lines are invented for CRLF input.
**  - fgets does not STORE the terminating CR in the caller's buffer.
**    The CR is consumed and ends the line; the buffer is always
**    NUL-terminated.
**  - _getbuf's trailing default-DMA restore passed *_dfltbuf (the first
**    BYTE of the buffer) as the DMA address.  Fixed to pass _dfltbuf.
**
** Notes:
**  - _wascr[] is per-unit pairing state.  Globals load zeroed, so it
**    starts clean.  If a file ENDS with a bare CR and the same unit is
**    immediately reused for a file BEGINNING with a bare LF, that LF is
**    swallowed (one lost blank line).  To be fussy, clear _wascr[u-5]
**    in fopen (ioopen).
**  - stdin keeps the old behaviour (skip LFs); the console talks CR.
**  - NEVER use the '\r' escape in this codebase: smallcp does not know
**    it and compiles '\r' as the LETTER 'r' (0x72).
*/
#include "stdio.h"

/* Constants are local deliberately: Small-C/Plus 1.06a has limited include handling. */
#define NBUFS   5
#define LGH     512
#define SECTOR  512
#define MREAD   22489
#define FCB_LBUFCNT  35

extern int _eof_marker;
extern int _ffcb[NBUFS];
extern int _fnext[NBUFS];
extern int _ffirst[NBUFS];
extern int _flast[NBUFS];
extern char *_dfltbuf;
extern char *_commbuf;
extern char *_commfcb;

int _wascr[NBUFS];   /* per-unit: last char delivered was a CR */

_getbuf(index) int index; {
  char *fcb, *dst, *src;
  int sectors_read, i, result, lbufcnt;

  fcb = _ffcb[index];
  dst = _ffirst[index];
  sectors_read = 0;

  while(sectors_read < (LGH / SECTOR)) {
    _stage_fcb(index);
    _cpm(26, _commbuf);
    result = _cpm(20, _commfcb);
    _unstage_fcb(index);
    if(result) break;

    lbufcnt = fcb[FCB_LBUFCNT] & 0xFF;
    if(lbufcnt == 0 || lbufcnt >= SECTOR) lbufcnt = SECTOR;

    src = _commbuf;
    i = lbufcnt;
    while(i--) *dst++ = *src++;
    sectors_read++;
    if(lbufcnt < SECTOR) break;
    }

  _cpm(26, _dfltbuf);
  if(sectors_read == 0) return -1;
  _flast[index] = dst;
  return _ffirst[index];
  }

getc(unit) int unit; {
  int c, index;

  if(unit == stdin) {
    while((c = getb(unit)) == LF);
    if(c == _eof_marker) return -1;
    return c;
    }

  index = unit - 5;
  c = getb(unit);

  if(c == LF && _wascr[index]) {
    c = getb(unit);              /* second half of CRLF: drop it */
    }

  if(c == LF) {                  /* lone LF: it IS the line ending */
    _wascr[index] = 0;
    c = '\n';
    }
  else if(c == '\n') {
    _wascr[index] = 1;
    }
  else {
    _wascr[index] = 0;
    }

  if(c == _eof_marker) return -1;
  return c;
  }

getb(unit) int unit; {
  int c, index;
  char *next;

  if(unit == stdin) {
    c = _cpm(1, 0);
    if(c == '\n') _cpm(2, LF);
    return c;
    }

  index = unit - 5;
  if(_fchk(index) != MREAD) {
    err("CAN'T READ OUTFILE");
    exit(-1);
    }

  next = _fnext[index];
  if(next == _flast[index]) {
    next = _getbuf(index);
    if(next == -1) return -1;
    }

  c = (*next++) & 0xFF;
  _fnext[index] = next;
  return c;
  }

getchar() {
  return getc(stdin);
  }

fgetc(unit) int unit; {
  return getc(unit);
  }

/* Reads up to size-1 characters or through the end of the line,
   whichever comes first.  The terminating CR ('\n' == 0x0D) is
   consumed but NOT stored; the buffer is always NUL-terminated.
   Returns buf, or 0 if EOF was reached before any character. */
fgets(buf, size, unit) char *buf; int size, unit; {
  char *p;
  int c;

  if(size <= 0) return 0;
  p = buf;
  c = -1;

  while(--size > 0) {
    c = fgetc(unit);
    if(c == -1) break;
    if(c == '\n') break;
    *p++ = c;
    }
  *p = 0;

  if(p == buf && c == -1) return 0;
  return buf;
  }
