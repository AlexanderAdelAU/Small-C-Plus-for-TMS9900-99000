/*
** IOSEEK.C -- random positioning for split IOLIB99
**
** New module, Aug 2026.  Adds lseek() and tell().
**
** NOTHING ELSE IN THE LIBRARY CHANGES.  IOCORE, IOOPEN, IOREAD and
** IOWRITE are untouched, so a program that never calls lseek does not
** grow by a single byte and cannot behave differently than before.
**
** How it works
** ------------
** The BDOS drives sequential I/O from the FCB's own cursor: CRN, the
** current record number (word at offset 26), with CBN, the current
** block (word 24).  Those live in the caller's copy of the FCB, which
** _stage_fcb hands to the BDOS for each call, so setting them here
** repositions the next sequential read or write.
**
** This is not a guess: fopen("a") in IOOPEN already does it.  It reads
** to end of file, decrements CRN by hand - borrowing into the high
** byte when it wraps - switches the slot to MWRITE, and lets the
** ordinary sequential write path carry on from there.  lseek does the
** same thing with an arbitrary record number, and clears CBN so the
** BDOS re-derives the block from the allocation chain, which is what
** BDTEST59 does whenever it plants a cursor by hand.
**
** Read units:  the buffer is invalidated so the next getc/getb pulls
**              the target record in through the normal path, then any
**              part-record offset is stepped over with getb.
** Write units: the held buffer is flushed first.  Only RECORD-ALIGNED
**              positions are accepted - a byte offset inside a record
**              would need that record read back and merged, and this
**              library cannot read a unit that is open for writing.
**              A non-aligned write seek returns -1 rather than quietly
**              corrupting the record.
**
** Limits, both from the 16-bit int
** -------------------------------
** A byte offset caps at 65535, i.e. 128 records.  lseek(unit, 0,
** SEEK_END) returns -1 for a file over 64KB rather than a wrapped
** value; use the BDOS GETSIZ call (35) where a large size is needed.
**
** Small-C notes
** -------------
** '/' and '%' compile to the SIGNED _ccdiv, so an offset above 32767
** would divide wrongly.  The record and remainder are taken with a
** shift and a mask instead: the quotient of any 16-bit value by 512
** fits in 7 bits, so (offset >> 9) & 0x7F is right even though >> is
** an arithmetic shift.
** NEVER use '\r' here - smallcp compiles it as the letter 'r'.
*/
#include "stdio.h"

/* Constants are local deliberately: Small-C/Plus 1.06a has limited include handling. */
#define NBUFS   5
#define SECTOR  512
#define MREAD   22489
#define MWRITE  17325

/* FCB word fields.  High byte at the even offset. */
#define FCB_CBN_HI   24
#define FCB_CRN_HI   26
#define FCB_FSZ_HI   14
#define FCB_LRBL_HI  20

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern int _ffcb[NBUFS];
extern int _fnext[NBUFS];
extern int _ffirst[NBUFS];
extern int _flast[NBUFS];
extern int _wascr[NBUFS];

/*
** Read one FCB word.  Words are big-endian: high byte at the even
** offset, low byte at the odd one.
*/
_fcbword(fcb, off) char *fcb; int off; {
  int hi, lo;
  hi = fcb[off] & 0xFF;
  lo = fcb[off + 1] & 0xFF;
  return (hi << 8) | lo;
  }

_setfcbword(fcb, off, value) char *fcb; int off, value; {
  fcb[off] = value >> 8;
  fcb[off + 1] = value;
  }

/*
** Size of an open file in bytes, or -1 if it will not fit in 16 bits.
** Taken from the FCB copy of the directory entry: FSZ counts records
** and LRBL is the byte count of the last one - the arithmetic DIR2
** uses.
*/
_fsize(index) int index; {
  char *fcb;
  int recs, lrbl;

  fcb  = _ffcb[index];
  recs = _fcbword(fcb, FCB_FSZ_HI);
  lrbl = _fcbword(fcb, FCB_LRBL_HI);

  if(recs > 127) return -1;
  if(lrbl > 0 && lrbl <= SECTOR)
    return ((recs - 1) << 9) + lrbl;
  return recs << 9;
  }

/*
** Current byte offset of the next byte to be read or written.
**
** After a record is READ, CRN names the record that will be fetched
** NEXT, so the one sitting in the buffer is CRN - 1.  Straight after
** fopen nothing is buffered, but _fnext == _flast == _ffirst + SECTOR
** and CRN is 0, which the same expression evaluates to 0.
**
** For a WRITE unit CRN names the record the next flush will write and
** the buffer holds bytes belonging to that record, so there is no
** minus one.
*/
tell(unit) int unit; {
  int index, mode, crn;
  char *fcb;

  index = unit - 5;
  mode  = _fchk(index);
  fcb   = _ffcb[index];
  crn   = _fcbword(fcb, FCB_CRN_HI);

  if(mode == MWRITE)
    return (crn << 9) + (_fnext[index] - _ffirst[index]);
  return ((crn - 1) << 9) + (_fnext[index] - _ffirst[index]);
  }

/*
** Position 'unit' to a byte offset.  whence is SEEK_SET, SEEK_CUR or
** SEEK_END.  Returns the new offset, or -1 on failure.
*/
lseek(unit, offset, whence) int unit, offset, whence; {
  int index, mode, target, rec, rem, size;
  char *fcb;

  index = unit - 5;
  mode  = _fchk(index);

  if(whence == SEEK_SET) target = offset;
  else if(whence == SEEK_CUR) target = tell(unit) + offset;
  else if(whence == SEEK_END) {
    size = _fsize(index);
    if(size < 0) return -1;
    target = size + offset;
    }
  else return -1;

  if(target < 0) return -1;

  /* shift and mask, never divide - see the note at the top */
  rec = (target >> 9) & 0x7F;
  rem = target & 0x1FF;

  fcb = _ffcb[index];

  if(mode == MWRITE) {
    if(rem != 0) return -1;             /* record-aligned writes only */
    if(fflush(unit) < 0) return -1;     /* leaves _fnext at _ffirst   */
    _setfcbword(fcb, FCB_CBN_HI, 0);
    _setfcbword(fcb, FCB_CRN_HI, rec);
    return target;
    }

  _setfcbword(fcb, FCB_CBN_HI, 0);
  _setfcbword(fcb, FCB_CRN_HI, rec);
  _fnext[index] = _flast[index];        /* force a refill on next read */
  _wascr[index] = 0;

  while(rem) {                          /* step into the record */
    if(getb(unit) == -1) return -1;
    rem = rem - 1;
    }

  return target;
  }
