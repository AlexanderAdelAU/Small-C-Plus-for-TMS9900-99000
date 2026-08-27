/*
** STDIO.H -- Standard Small-C Definitions
**
** Copyright 1984  L. E. Payne and J. E. Hendrix
*/
#define stdin    0
#define stdout   1
#define stderr   2
#define ERR   (-2)
#define EOF   (-1)
#define YES      1
#define NO       0
#define NULL     0
#define CR      0x0D /* '\r' */
#define LF      0x0A /* '\n' */
#define BELL     7
#define SPACE  ' '
#define NEWLINE LF      /*23*/ /*45*/
#define FILE char

/* Flags for the iobuf structure  */
#define	_IOREAD	1 /* currently reading */
#define	_IOWRT	2 /* currently writing */
#define	_IORW	0x0080 /* opened as "r+w" */

/* Flags for lseek and fseek */
#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2
