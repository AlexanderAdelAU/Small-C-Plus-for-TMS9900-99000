/*	name...
 *		iolib
 *
 *	purpose...
 *		to provide a "standard" interface between c
 *		programs and the CPM I/O system.
 *
 *	notes...
 *		Compile using -M option.
 *
 */

#include "stdio.h"

/* #define INT_OFF	*/


/* turn off interrupts, except during BDOS calls */
/* programs will run faster, but interrupts will be missed */
/* to keep interrupts on, comment out this define */
/* only do this if you understand the consequences */

/*#define NULL 0
 #define SPACE 32
 */

#define NBUFS 5
/*	= number of files which can be open at once */
/*  though buffers are only allocated as files are opened */
#define LGH 512
/*	length of each file buffer		*/
/*	= some multiple of 128: 128, 256, 384, 512... */
#define BUFLGH (LGH+36)
/*	includes fcb associated with buffer */

#define MFREE 		11387
#define MREAD 		22489
#define MWRITE 		17325
#define DIR					/* compile directory option */
#define CMDBUF		0xA0			/* location 160 is a pointer to command line */
#define CMDSIZ		0xA2			/* location 162 is size of command line */
#define INTBUF		0xA4			/* Internal Buffer pointer address  */
#define FREEMEM		0xA6		/* Pointer to Free Memory */
#define MEMLIMIT 	0xA8		/* Pointer to Memory Limt */

/*int dummy = 0; */
int _argc; /* # arguments on command line */
char **_argv; /* pointers to arguments in alloc'ed area */
int _heaptop;
int	*fmptr;		/*Free memory pointer */
int _current;
int _dfltdsk; /* "current disk" at beginning of execution */
int _ffcb[NBUFS], /* pointers to the fcb's
 if zero, no memory has been allocated */
_fnext[NBUFS], /* pointers to the next char to be fed
 to the program (for an input file) or
 the next free byte in the buffer (for
 an output file)			*/
_ffirst[NBUFS], /* ptrs to the starts of the buffers */
_flast[NBUFS]; /* ptrs to the ends of the buffers */
char *_dfltbuf;
int _fmode[NBUFS] = { MFREE, MFREE, MFREE, MFREE, MFREE };

/*
 MFREE => buffer is free
 MREAD => open for reading
 MWRITE => open for writing
 */

/*
 * stdin and stdout defintions
 */
/*
char *stdin, *stdout ;
*/

int _ex, _cr; /* extent & current record at beginning
 of this buffer full (used for "A" access) */

int _argcval;  /* argument count set for functions that need it in call.a99 */

/*
 ** -- setup default drive for CPM
 ** Process Command Line, Execute main(), and Exit to CP/M and Shell
 */

_main() {
	fmptr = FREEMEM;
	_dfltbuf = INTBUF;
	_heaptop = *fmptr;	/* This is loaded by the MONITOR  */
	_dfltdsk = _cpm(25, 0); /*;get current disk */
	_setargs();
	main(_argc, _argv);	/* Return to the loaded programme.  */
	exit(0);
}

_setargs() {
	char *inname, *outname; /* file names from command line */
	char *count; /* *count is # characters in command line */
	char *lastc; /* points to last character in command line */
	char *mode; /* mode for output file */
	char *next; /* where the next byte goes into alloc'ed area */
	char *ptr; /* *ptr is next character in command line */
	int  *vptr;  /* vector pointer removes the need for pointer to pointer confusion */

	vptr = CMDBUF;	/* point to the command line address */
	ptr = *vptr;	/* okay pointer is now pointer to the command line */
	vptr = CMDSIZ;	/* point to the command line size address */
	count = *vptr;; /* SHELL CP/M command buffer length vector */

	lastc = ptr + *count - 1;
	*lastc = SPACE; /* place a sentinel */
	_argv = alloc(30); /* space for 15 arg pointers */
	_argv[0] = next = alloc(*count + 2); /* allocate the buffer */
	 *next++ = NULL; /* place 0-th argument */
	_argc = 0;
	inname = outname = NULL;
	while (++ptr < lastc) {
		if (*ptr == SPACE)
			continue;
		if (*ptr == '<') { /* redirect input */
			while (*++ptr == SPACE)
				;
			inname = next;
		} else if (*ptr == '>') { /* redirect output */
			if (ptr[1] == '>') {
				++ptr;
				mode = "a";
			} else {
				mode = "w";
			}
			while (*++ptr == SPACE)
				;
			outname = next;
		} else { /* argument */
			_argv[_argc++] = next;
		}
		while (*ptr != SPACE) {
			*next++ = *ptr++;
		}
		*next++ = NULL;
	}
	_argv[_argc] = 0;
	_redirect(inname, "r", stdin);
	_redirect(outname, mode, stdout);

}

/*
 * _redirect - open file for redirected i/o
 *             (if filename pointer is non-NULL)
 */
_redirect(filename, mode, std)
	char *filename;char *mode;int *std; {
	if (filename) {
		if ((*std = fopen(filename, mode)) == 0) {
			err("CAN'T REDIRECT");
			exit(-1);
		}
	}
}

/* return address of a block of memory */

alloc(b)
	int b; { /* # bytes desired */
	_heaptop += b;
	if (_heaptop & 1) _heaptop++;
	return _heaptop;
}

/* reset the top of heap pointer to addr* */

free(addr)
	int addr; {
	_heaptop = addr;
}

/*
 * return number of bytes between top of heap
 * and end of TPA.  Remember that this includes
 * the stack!
 */

avail() {
	return (_heaptop);
}

/* error...print message & walkback trace (if available) */

err(s)
	char *s; {
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

/*
 #asm
  EVEN
STDIN WORD	0 ; 0 initially, or unit number for input file
 ; if input has been redirected by args()
STDOUT WORD 1	; 1 initially, or unit number for output file
 ; if output has been redirected by args()
  EVEN
 #endasm
 */

getchar() {
	return getc(stdin);
}

putchar(c)
	char c; {
	return (putc(c, stdout));
}

/* print a null-terminated string */

puts(buf)
	char *buf; {
	char c;
	while (c = *buf++)
		putchar(c);
}

/*
 * _newfcb - create CP/M file control block for named file
 *
 *           returns 0 on failure, 1 on success
 */
_newfcb(name, fcb)
	char *name;char *fcb; {
	char *c;
	int i;

	/* clear file name */
	i = 11;
	while (i)
		fcb[i--] = ' ';
	/* clear rest of fcb */
	i = 12;
	while (i < 36) {
		fcb[i++] = 0;
	}
	/* Note for CDOS the drive location is byte 33 in the fcb  */
	if (name[1] == ':') { /* transfer disk */
		fcb[33] = *name & 0xf;
		name += 2;
	} else
		fcb[33] = _dfltdsk;

	if (*name == 0)
		return 0; /* error if no filename */

	/* transfer name */
	i = 0;
	while (c = _upper(*name++)) {
		if (c == '.')
			break;
	/*	putc(c,1); */
		fcb[i++] = c; /*0X41; */
	}
	if (c == '.') { /* transfer extension */
		i = 9;
		while ((c = _upper(*name++)) && i < 12) {
			fcb[i++] = c;
		}
	}
	if (c == 0)
		return 1; /* OK if last char is NULL */
	return 0;
}

/* open file in fmode "r", "w", or "a" (upper or lower case)	*/

fopen(name, mode)
	char *name, *mode; {
	char c, *fcb;
	int index, i, unit;

	index = NBUFS;
	while (index--) { /* search for free buffer */
		if (_fmode[index] == MFREE)
			break;
	}
	if (index == -1) {
		err("NO BUFFERS");
		exit(-1);
	}
	unit = index + 5;

	/* allocate memory if required */
	if (_ffcb[index] == 0)
		_ffcb[index] = alloc(BUFLGH);

	_ffirst[index] = _ffcb[index] + 36;
	_flast[index] = _ffirst[index] + LGH;

	fcb = _ffcb[index];

	/* initialise file control block */
	if (_newfcb(name, fcb) == 0) {
		return 0; 	/* invalid file name */
	}
	if ((c = _upper(*mode)) == 'R' || c == 'A') {
		if (_cpm(15, fcb) < 0) {
			return 0; /* file not found */
		}
		/* open for reading -  forces immed. read */
		_fmode[index] = MREAD;
		_fnext[index] = _flast[index];

		if (c == 'A') { /* append mode requested? */
			while (getc(unit) != -1)
				; /* read to EOF */
			fcb[12] = _ex; /* reset to values at...*/
			_cpm(15, fcb); /* ...beginning of buffer */
			fcb[32] = _cr;
			_fmode[index] = MWRITE;
		}
		return unit;
	} else if (c == 'W') {
		_cpm(19, fcb); /* delete file */
		if (_cpm(22, fcb) < 0) /* create file */
			return 0; /* creation failure */
		_fmode[index] = MWRITE; /* open for writing */
		_fnext[index] = _ffirst[index]; /* buffer is empty */
		return unit;
	}
	return 0;
}

/* close a file */

fclose(unit)
	int unit; {
	int index, werror;
	char *end, *ptr;

	index = unit - 5;
	if (_fchk(index) == MREAD) {
		/* don't close read files */
		_fmode[index] = MFREE;
		return 1; /* success */
	}

	putb(26, unit); /* append ^Z (CP/M EOF) */
	/* pad buffer out with ^Z */
	ptr = _fnext[index];
	end = _flast[index];
	while (ptr < end)
		*ptr++ = 26;

	werror = fflush(unit);
	_fmode[index] = MFREE;
	if ((_cpm(16, _ffcb[index]) < 0) || werror)
		return 0; /* failure */
	/* if free() worked properly we could do the following
	 free(_ffcb[index]) ;
	 _ffcb[index] = 0 ; */
	return 1; /* success */
}

/* check for legal index */

_fchk(index)
	int index; {
	int i;

	if ((index >= 0) & (index < NBUFS)) {
		i = _fmode[index];
		if ((i == MREAD) | (i == MWRITE))
			return i;
	}
	err("INVALID UNIT NUMBER");
	exit(-1);
}

/* get character from file (return -1 at EOF)  */

getc(unit)
	int unit; {
	int c;

	while ((c = getb(unit)) == LF) {
	}
	if (c == 26) {/*	CP/M EOF? */
		if (unit >= 5) {
			/* leave _fnext[index] pointing at the ^Z */
			_fnext[unit - 5];
		}
		return -1;
	}
	return c;
}

/*
 * _getbuf - fetch new buffer from disk
 *           this routine introduced rmy 31/8/86
 *           not used by cdos
 */

_getbuf(index)
	int index; {
	char *fcb, *last, *next;

	fcb = _ffcb[index];
	_ex = fcb[12];
	_cr = fcb[32]; /* save for fopen() */
	next = _ffirst[index];
	last = next + LGH;
	while (next < last) {
		_cpm(26, next); /* set DMA */
		if (_cpm(20, fcb))
			break;
		next += LGH;
	}
	_cpm(26, *_dfltbuf); /* reset DMA */
	if (next == _ffirst[index]) { /* no records read? */
		return -1;
	}
	_flast[index] = next;
	return (_ffirst[index]);
}

/*
 * getb - get byte from file (return -1 at EOF)
 */

getb(unit)
	int unit; {
	int c;
	int index;
	char *next;


	if (unit == 0) { /* STDIN */
		c = _cpm(1, 0);
		if (c == '\n')
			_cpm(2, LF); /* add LF after CR */
		return c;
	}
	index = unit - 5;
	if (_fchk(index) != MREAD) {
		err("CAN\'T READ OUTFILE");
		exit(-1);
	}
	next = _fnext[index];
	if (next == _flast[index]) { /* empty buffer? */
		next = _getbuf(index);
		if (next == -1) {
			return -1;
		}
	}
	c = (*next++) & 0xff;
	_fnext[index] = next;
	return c;
}

/*
 * write a character to a file - add LF after CR
 * return last character sent or -1 on write error
 */

putc(c, unit)
	char c;int unit; {
	int ret;
	ret = putb(c, unit);
	if (ret == '\n')
		ret = putb(LF, unit);
	return ret;
}

/* write a byte to a file */

putb(c, unit)
	char c;int unit; {
	int werror, index;
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
	if (_fnext[index] == _flast[index]) {
		werror = fflush(unit);
	} else {
		werror = 0;
	}
	next = _fnext[index];
	*next++ = c;
	_fnext[index] = next;
	if (werror)
		return werror;
	return c;
}

/* flush buffer to disk (on error returns nonzero) */

fflush(unit)
	int unit; {
	int index, i;
	char *next, *going;

	index = unit - 5;
	if (_fchk(index) != MWRITE) {
		err("CAN\'T FLUSH");
		exit(-1);
	}

	next = _fnext[index];
	going = _fnext[index] = _ffirst[index];
	while (going < next) {
		_cpm(26, going); /* set DMA */
		if (_cpm(21, _ffcb[index]))
			return -1; /* error? */
		going += LGH;
	}
	_cpm(26, *_dfltbuf); /* reset DMA */
	return 0; /* no error */
}

_upper(c)
	int c; /* converts to upper case */
{
	if (c >= 'a')
		return c - 32;
	return c;
}

/* Exit value will govern Shell messages and behaviour */

exit(value)
int value;
{
	int index;

	/* ensure that all files open for write have their buffers flushed */
	index = NBUFS;
	while (index--) {
		if (_fmode[index] == MWRITE)
			fclose(index + 5);
	}
	_shell(value); /*return to shell - _cbdos.a99 source file */
}
