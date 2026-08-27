/*
** IODEFS.H -- REFERENCE ONLY for the split Small-C/Plus 1.06a build.
**
** Do not include this file from IOCORE/IOOPEN/IOREAD/IOWRITE.
** The compiler's old include handling is why the required constants are kept
** locally in each translation unit.  Keep this file synchronized if constants
** change; it is retained as a compact layout/reference list.
*/

#define NBUFS   5
#define LGH     512
#define BUFLGH  (LGH + 36)
#define SECTOR  512

#define MFREE   11387
#define MREAD   22489
#define MWRITE  17325

#define CMDBUF      0xA0
#define CMDSIZ      0xA2
#define COMM_BUF    0xA4
#define FREEMEM     0xA6
#define COMM_FCB    0xAC
#define MEMLIMIT    0xB0

#define NAMSIZ      11
#define EXTSIZ       3
#define FCBSIZ      36

#define FCB_CRN      27
#define FCB_LBUFCNT  35
