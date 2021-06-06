/*
 * cc1.c - first part of Small-C/Plus compiler
 */

#include <stdio.h>
#include <string.h>
#include "ccdefs.h"
#include "ccfunc.h"


/*	Now reserve some storage words		*/

char Version[] = "          Original Z80 Version 21st April 1990       " ;
char Target[] = "        TMS9900 Relocatable Version 18th May, 2021 " ;
char Banner[] =  "* * *  Small-C/Plus Version 1.06a for the TMS 9900  * * *" ;
char Author[] =  "       Cain, Van Zandt, Hendrix, Yorston, Cameron" ;

/*   1.06  Original port for Z80 1.06 version
 *   1.06a Added unsigned modifier for char and int
 *   */

char Fnptrtail[]	= ")()" ;
char Hashasm[]		= "#asm" ;
char Comma[]		= "expected comma" ;

extern char Overflow[] ;

SYMBOL	*symtab, *loctab ;		/* global and local symbol tables */
SYMBOL	*glbptr, *locptr ;		/* ptrs to next entries */
int		glbcnt ;				/* number of globals used */

SYMBOL	*dummy_sym[NTYPE+NUMTAG+1] ;

WHILE_TAB	*wqueue ;			/* start of while queue */
WHILE_TAB	*wqptr ;			/* ptr to next entry */

char	*litq;			/* literal pool */
int		litptr;			/* index of next entry */

char	macq[MACQSIZE];		/* macro string buffer */
int		macptr;				/* and its index */

TAG_SYMBOL	*tagtab ;		/* start of structure tag table */
TAG_SYMBOL	*tagptr ;		/* ptr to next entry */

SYMBOL	*membtab ;			/* structure member table */
SYMBOL	*membptr ;			/* ptr to next member */

char	*stage ;			/* staging buffer */
char	*stagenext ;		/* next address in stage */
char	*stagelast ;		/* last address in stage */

SW_TAB	*swnext ;			/* address of next entry in switch table */
SW_TAB	*swend ;			/* address of last entry in switch table */

char line[LINESIZE]; /* parsing buffer */
char mline[LINESIZE]; /* temp macro buffer */
int lptr, mptr; /* indexes into buffers */
char Filename[24] ;		/* output file name */

/*	Misc storage	*/

int	nxtlab,		/* next avail label # */
	litlab,		/* label # assigned to literal pool */
	Zsp,		/* compiler relative stk ptr */
	undeclared,	/* # function arguments not yet declared */
	ncmp,		/* # open compound statements */
	nogo,		/* > 0 disables goto statements */
	noloc,		/* > 0 disables block locals */
	errcnt,		/* # errors in compilation */
	errstop,	/* stop on error */
	eof,		/* set non-zero on final input eof */
	ctext,		/* non-zero to intermix c-source */
	cmode,		/* non-zero while parsing c-code */
				/* zero when passing assembly code */
	declared,	/* number of local bytes declared, else -1 when done */
	lastst,		/* last executed statement type */
	mainflg, /* output is to be first asm file */
	iflevel,	/* current #if nest level */
	skiplevel,	/* level at which #if skipping started */
	fnstart,	/* line# of start of current fn. */
	lineno,		/* line# in current file */
	infunc,		/* "inside function" flag */
	savestart,	/* copy of fnstart "	" */
	saveline,	/* copy of lineno  "	" */
	saveinfn,	/* copy of infunc  "	" */
	swactive,	/* true inside a switch */
	swdefault,	/* default label number, else 0 */
	caller,		/* stack offset for caller links...
					local[caller] points to name of current fct
					local[caller-1] points to link for calling fct,
					where local[0] is 1st word on stack after ret addr  */
	fname;		/* label for name of current fct  */

FILE	*input,		/* iob # for input file */
		*output,	/* iob # for output file (if any) */
		*inpt2,		/* iob # for "include" file */
		*saveout;	/* holds output ptr when diverted to console */


#ifdef SMALL_C
int minavail = 48000 ;		/* minimum memory available */
#endif

SYMBOL	*currfn,	/* ptr to symtab entry for current fn. */
		*savecurr ;	/* copy of currfn for #include */

int gargc ;		/* global copies of command line args */
char **gargv ;


/*
 *	Compiler begins execution here
 */
main(argc, argv)
int argc ;
char **argv ;
{
	gargc = argc ;
	gargv = argv ;

	/* allocate space for arrays */
	litq = alloc(LITABSZ) ;
	symtab = SYM_CAST alloc(NUMGLBS*sizeof(SYMBOL)) ;
	loctab = SYM_CAST alloc(NUMLOC*sizeof(SYMBOL)) ;
	wqueue = WQ_CAST alloc(NUMWHILE*sizeof(WHILE_TAB)) ;

	tagptr = tagtab = TAG_CAST alloc(NUMTAG*sizeof(TAG_SYMBOL)) ;
	membptr = membtab = SYM_CAST alloc(NUMMEMB*sizeof(SYMBOL)) ;

	swnext = SW_CAST alloc(NUMCASE*sizeof(SW_TAB)) ;
	swend = swnext + (NUMCASE-1) ;

	stage = alloc(STAGESIZE) ;
	stagelast = stage+STAGELIMIT ;

	/*line = my_malloc(LINESIZE);
	mline = my_malloc(LINESIZE);
	*/

	/* empty symbol table */
	glbptr = STARTGLB;
	while ( glbptr < ENDGLB ) {
		glbptr->name[0] = 0 ;
		++glbptr ;
	}

	glbcnt = 0 ;			/* clear global symbols */
	locptr = STARTLOC ;		/* clear local symbols */
	wqptr = wqueue ;		/* clear while queue */
	litptr = 0 ;			/* clear literal pool */

  	Zsp =			/* stack ptr (relative) */
	errcnt =		/* no errors */
	errstop =		/* keep going after an error */
	eof =			/* not eof yet */
	swactive =		/* not in switch */
	skiplevel =		/* #if not encountered */
	iflevel =		/* #if nesting level = 0 */
	ncmp =			/* no open compound states */
	lastst =		/* not first file to asm */
	fnstart =		/* current "function" started at line 0 */
	lineno =		/* no lines read from file */
	infunc =		/* not in function now */
			0 ;		/*  ...all set to zero.... */

	stagenext = NULL ;		/* direct output mode */

	input =					/* no input file */
	inpt2 =					/* or include file */
	saveout =				/* no diverted output */
	output = NULL ;			/* no open units */

	currfn = NULL ;			/* no function yet */
	macptr = cmode = 1 ;	/* clear macro pool and enable preprocessing */
	/*
	 *	compiler body
	 */
	setup_sym() ;	/* define some symbols */
	ask();			/* get user options */
	openout();		/* get an output file */
	openin();		/* and initial input file */
	header();		/* intro code */
	parse(); 		/* process ALL input */
	trailer();		/* follow-up code */
	closeout();		/* close the output (if any) */
	errsummary();	/* summarize errors */
	return;			/* then exit to system */
}

/*
 * malloc() with error checking
 */
my_malloc(n)
int n ;
{
	char *s ;

	if ( (s=malloc(n)) == 0 ) {
		pl("cc0: memory allocation failure\n") ;
		ccabort() ;
	}
	return s ;
}

/*
 *	Abort compilation
 */
ccabort()
{
	if ( inpt2 != NULL )
		endinclude();
	if ( input != NULL )
		fclose(input);
	closeout();
	toconsole();
	pl("Compilation aborted\n");
	exit(1);
}

/*
 * Process all input text
 *
 * At this level, only static declarations,
 * defines, includes, and function
 * definitions are legal...
 */
parse()
{
	while ( eof == 0 ) {		/* do until no more input */
		if ( amatch("extern") )
			dodeclare(EXTERNAL, NULL, 0) ;
		else if ( dodeclare(STATIK, NULL, 0) )
			;
		else if ( ch() == '#' ) {
			if (match(Hashasm)) doasm();
			else if (match("#include")) doinclude() ;
			else if (match("#define")) addmac() ;
/*			else newfunc();  surely not? */
		}
		else newfunc(NULL, NO);
		blanks();	/* force eof if pending */
	}
}



/*
 *	Dump the literal pool if it's not empty
 */
dumplits(size, pr_label)
int size, pr_label ;
{
	int j, k ;

	if ( litptr ) {
		if ( pr_label ) {
			printlabel(litlab) ;
			col() ;
		}
		k = 0 ;
		while ( k < litptr ) {
			/* pseudo-op to define byte */
			if (size == 1) defbyte();
			else defword();
			j = 12 ;		/* max bytes per line */
			while ( j-- ) {
				outdec(getint(litq+k, size));
				k += size ;
				if ( j == 0 || k >= litptr ) {
					nl();		/* need <cr> */
					break;
				}
				outbyte(',');	/* separate bytes */
			}
		}
	}
	if(k & 1) even(); /* TMS 9900 addition to force even boundary *5 5*/
}

/*
 * dump zeroes for default initial value
 * (or rather, get loader to do it for us)
 */
dumpzero(size, count)
int size, count ;
{
	if (count <= 0) return ;
	defstorage() ;
	outdec(size*count) ;
	if (size*count & 1) /* for TMS9900 to force even *//*55*/
		even();
	else
		nl();
	nl();
}

/*
 *	Report errors for user
 */
errsummary()
{
	/* see if anything left hanging... */
	if (ncmp) error("missing closing bracket");
		/* open compound statement ... */
	nl();
#ifdef SMALL_C
	outstr("Bytes free: "); outdec(minavail); nl();
#endif
	outstr("Symbols:    "); outdec(glbcnt); nl();
	outstr("Errors:     "); outdec(errcnt); nl();
}

int	filenum;	/* next argument to be used */

/*
 * places in s the n-th argument (up to "size"
 * bytes). If successful, returns s. Returns 0
 * if the n-th argument doesn't exist.
 */

#ifndef SMALL_C
char *
#endif

nextarg(n, s, size)
int n;
char *s;
int size ;
{
	char *str;
	int i;

	if ( n < 0 || n >= gargc ) return NULL ;
	i = 0 ;
	str = gargv[n] ;
	while ( ++i < size && (*s++ = *str++) )
		;
	return s ;
}

ask()		/* fetch arguments */
{
	char *ptr;
	int i ;

	clear();		/* clear input line */
	pl(Banner);		/* print banner */
	nl();
	pl(Author);
	nl();
	pl(Version);
	nl();
	pl(Target);
	nl();
	nl();

	nxtlab =			/* start numbers at lowest possible */
	ctext =				/* don't include the C text as comments */
	errstop = 0 ;		/* don't stop after errors */
	mainflg = 1; /* Most files do not contain main.  Add -M to include i.e. -M Sieve  */
	i = 1 ;
	while ( i < gargc ) {
		ptr = gargv[i++] ;
		if ( ptr[0] != '-' ) break ;
		switch ( ptr[1] ) {
			case 'C':
				ctext = 1 ;
				break ;
			case 'D':
				defmac(ptr+2) ;
				break ;
			case 'M':
				mainflg = 0;
				break;
			case 'U':
				strcpy(line, ptr+2) ;
				delmac() ;
				break ;
			case 'E':
				errstop = 1 ;
				break ;
			default:
				pl("unknown flag: ");
				outstr(ptr);
				nl();
		}
	}
	filenum = --i ;		/* first file argument */
	clear();			/* erase line */
}

/*
 * make a few preliminary entries in the symbol table
 */
setup_sym()
{
	/* define some useful constants */
	defmac("CPM") ;
	defmac("Z80") ;
	defmac("PCW") ;
	defmac("SMALL_C") ;
	/* dummy symbols for pointers to char, int, double */
	/* note that the symbol names are not valid C variables */
	dummy_sym[0] = 0 ;
	dummy_sym[CCHAR] = addglb("0ch", POINTER, CCHAR, 0, 0, 0, 0) ;
	dummy_sym[CINT] = addglb("0int", POINTER, CINT, 0, 0, 0, 0) ;
	dummy_sym[DOUBLE] = addglb("0dbl", POINTER, DOUBLE, 0, 0, 0, 0) ;
}

/*
 *	Get output filename
 */
openout()
{
	clear() ;		/* erase line */
	output = 0 ;	/* start with none */
	if ( nextarg(filenum, line, 16) == NULL ) return ;
	/* copy file name to string */
	strcpy(Filename, line) ;
	strcat(line, ".A99") ;
	if ( (output=fopen(line, "w")) == NULL ) {
		error("Can't open output file") ;
	}
	clear() ;			/* erase line */
}

/*
 *	Get (next) input file
 */
openin()
{
	input = 0 ;				/* none to start with */
	while ( input == 0 ) {	/* any above 1 allowed */
		clear() ;			/* clear line */
		if ( eof ) break ;	/* if user said none */
		if ( nextarg(filenum++, line, 16) == NULL ) {
			/* none given... */
			eof = 1 ;
			break ;
		}
		strcat(line, ".c") ;
		pl(line); pl("\n") ;
		if ( (input=fopen(line,"r")) == NULL ) {
			pl("Can't open input file. ") ;
		}
		else {
			newfile() ;
		}
	}
	clear();		/* erase line */
}

/*
 *	Reset line count, etc.
 */
newfile()
{
	lineno  = 				/* no lines read */
	fnstart = 				/* no fn. start yet. */
	infunc  = 0 ;			/* therefore not in fn. */
	currfn  = NULL ;		/* no fn. yet */
}

/*
 *	Open an include file
 */
doinclude()
{
	char name[30], *cp ;

	blanks();	/* skip over to name */

	toconsole();
	outstr(line); nl();
	tofile();

	if ( inpt2 )
		error("Can't nest include files");
	else {
		/* ignore quotes or angle brackets round file name */
		strcpy(name, line+lptr) ;
		cp = name ;
		if ( *cp == '"' || *cp == '<' ) {
			name[strlen(name)-1] = '\000' ;
			++cp ;
		}
		if ( (inpt2=fopen(cp, "r") ) == NULL ) {
			error("Can't open include file") ;
		}
		else {
			saveline = lineno ;
			savecurr = currfn ;
			saveinfn = infunc ;
			savestart = fnstart ;
			newfile() ;
		}
	}
	clear();		/* clear rest of line */
				/* so next read will come from */
				/* new file (if open) */
}

/*
 *	Close an include file
 */
endinclude()
{
	toconsole();
	outstr("#end include\n");
	tofile();

	inpt2  = 0 ;
	lineno  = saveline ;
	currfn  = savecurr ;
	infunc  = saveinfn ;
	fnstart = savestart ;
}

/*
 *	Close the output file
 */
closeout()
{
	tofile() ;	/* if diverted, return to file */
	if ( output ) {
		/* if open, close it */
		fclose(output) ;
	}
	output = 0 ;		/* mark as closed */
}

/*
 * test for global declarations/structure member declarations
 */
dodeclare(storage, mtag, s_memb)
int storage ;
TAG_SYMBOL *mtag ;		/* tag of struct whose members are being declared, or zero */
int s_memb ;			/* TRUE if struct member is being declared, zero for union */
						/* only matters if mtag is non-zero */
{
	char sname[NAMESIZE] ;
	TAG_SYMBOL *otag ;		/* tag of struct object being declared */
	int sflag ;				/* TRUE for struct definition, zero for union */
	int ret ;				/* return value from dotype() */

	sflag = 0 ;
	switch ( (ret=dotype()) ) {
	default :				/* if no type specifier, but external, treat as int */
		if ( storage != EXTERNAL )
			return 0 ;
		ret = CINT ;
	case CCHAR :
	case CINT :
	case DOUBLE :
	case UCCHAR :
	case UCINT :
		declglb(ret, storage, mtag, NULL, s_memb) ;
		return 1 ;
	case STRUCT :
		sflag = 1 ;			/* flag as struct and fall through */
	case UNION :
		/* find structure tag */
		if ( symname(sname) == 0 )
			illname() ;
		if ( (otag=findtag(sname)) == 0 ) {
			/* structure not previously defined */
			otag = defstruct(sname, storage, sflag) ;
		}
		declglb(STRUCT, storage, mtag, otag, s_memb) ;
		return 1 ;
	}
	return 0 ;
}

/* name for dummy pointer to struct */
char nam[] = "0a" ;

/*
 * define structure/union members
 * return pointer to new structure tag
 */

TAG_SYMBOL *defstruct(sname, storage, s_memb)
char *sname ;
int storage ;
int s_memb ;
{
	int itag ;				/* index of tag in tag symbol table */
	TAG_SYMBOL *ptr ;

	if ( tagptr >= ENDTAG ) {
		error(Overflow) ;
		ccabort() ;
	}
	strcpy(tagptr->name, sname) ;
	tagptr->size = 0 ;
	tagptr->ptr = membptr ;

	/* increment tagptr to add tag to table */
	ptr = tagptr++ ;

	/* add dummy symbol */
	++nam[1] ;
	itag = ptr - tagtab ;
	dummy_sym[NTYPE+1+itag] = addglb(nam,POINTER, STRUCT, 0, 0,STRUCT, 0, 0, itag) ;

	needchar('{') ;
	while ( dodeclare(storage, ptr, s_memb) )
		;
	needchar('}') ;
	ptr->end = membptr ;
	return ptr ;
}


/*
 * make a first stab at determining the ident of a variable
 */
get_ident()
{
	if ( match("**") )
		return PTR_TO_PTR ;
	if ( cmatch('*') )
		return POINTER ;
	if ( match("(*") )
		return PTR_TO_FN ;
	return VARIABLE ;
}

/*
 * return correct index into dummy_sym
 */
dummy_idx(typ, otag)
int typ ;
TAG_SYMBOL *otag ;
{
	if ( typ == STRUCT )
		return NTYPE + 1 + (otag-tagtab) ;
	else
		return typ ;
}

/*
 *	Declare a static variable (i.e. define for use)
 *
 *  makes an entry in the symbol table so subsequent
 *  references can call symbol by name
 */
declglb(typ, storage, mtag, otag, s_memb)
int typ ;		/* typ is CCHAR, CINT, UCCHAR, UCINT, DOUBLE, STRUCT */
int storage;
TAG_SYMBOL *mtag ;		/* tag of struct whose members are being declared, or zero */
TAG_SYMBOL *otag ;		/* tag of struct for object being declared */
int s_memb ;			/* TRUE if struct member being declared, zero if union */
{
	char sname[NAMESIZE];
	int size, ident, more, itag, type ;
	SYMBOL *ptr ;

	do {
		if ( endst() ) break;	/* do line */

		type = typ ;
	/*	if (typ == UCCHAR) type = CCHAR;
		if (typ == UCINT) type = CINT;
		*/
		size = 1 ;		 /* assume 1 element  global chars can be single byte in TMS 9900 Architecture */
		more =					/* assume dummy symbol not required */
		itag = 0 ;				/* just for tidiness */

		ident = get_ident() ;

		if ( symname(sname) == 0 )	/* name ok? */
			illname() ; 			/* no... */

		if ( ident == PTR_TO_FN ) {
			needtoken(Fnptrtail) ;
			ident = POINTER ;
		}
		else if ( cmatch('(') ) {
			ptrerror(ident) ;
			if ( ident == POINTER ) {
				/* function returning pointer needs dummy symbol */
				more = dummy_idx(typ, otag) ;
				type = CINT ;
			}
			ident = FUNCTION ;
			size = 0 ;
		}
		else if (cmatch('[')) {		/* array? */
			ptrerror(ident) ;
			if ( ident == POINTER ) {
				/* array of pointers needs dummy symbol */
				more = dummy_idx(typ, otag) ;
				type = CINT ;
			}
			size = needsub() ;	/* get size */
			ident = ARRAY;
		}
		else if ( ident == PTR_TO_PTR ) {
			ident = POINTER ;
			more = dummy_idx(typ, otag) ;
			type = CINT ;
		}

		if ( otag ) {
			/* calculate index of object's tag in tag table */
			itag = otag - tagtab ;
		}

		/* add symbol */
		if ( mtag == 0 ) {
			/* this is a real variable, not a structure member */
			/* initialise variable (allocate storage space) */

			if (storage != EXTERNAL && ident != FUNCTION) {
				initials(sname, type, ident, size, more, otag);
			}

			/* check symbol against any previous declaration */
			if ( (ptr=findglb(sname)) ) {
				if ( ptr->ident != ident || ptr->type != type ||
							ptr->more != more ) {
					/* previously declared with different type */
					multidef() ;
				}
				else if ( storage == STATIK && ident != FUNCTION ) {
					/* same type, now check storage */
					/* functions are always EXTERNAL, newfunc() makes them STATIK */
					if ( ptr->offset.i == EXTERNAL ) {
						/* previous declaration was EXTERNAL, this is definition */
						ptr->offset.i = STATIK ;
					}
					else {
						/* there can be just one STATIK declaration */
						multidef() ;
					}
				}
			}
			/* We haven't found a previous declaration of this symbol so add it */
			/* TMS 9900 modify to define relocation variables */
			/* Note use of typ so unsigned can be determined */
			else {
				addglb(sname, ident, type, (storage != EXTERNAL && ident != FUNCTION) ? ENT:EXT,
									(ident == FUNCTION) ? EXTERNAL : storage, more, itag) ;
			}
		}
		else if ( s_memb ) {
			/* are adding structure member, mtag->size is offset */
			addmemb(sname, ident, type,  0, mtag->size, storage, more, itag);
			/* store (correctly scaled) size of member in tag table entry */
			if ( ident == POINTER) type = CINT ;
			cscale(type, otag, &size) ;
			mtag->size += size ;
		}
		else {
			/* are adding union member, offset is always zero */
			addmemb(sname, ident, type,  0, 0, storage, more, itag);
			/* store maximum member size in tag table entry */
			if ( ident == POINTER ) type = CINT ;
			cscale(type, otag, &size) ;
			if ( mtag->size < size )
				mtag->size = size ;
		}

		/* check for a function definition */
		if ( storage == STATIK && ident == FUNCTION ) {
			if ( cmatch(')') ) {
				blanks() ;
				if ( ch() != ',' && ch() != ';' ) {
					/* must be function definition, and we've read closing bracket */
					newfunc(sname, YES) ;
					return ;
				}
				/* must be function declaration, carry on */
			}
			else {
				/* must be function definition, and we've read opening bracket */
				newfunc(sname, NO) ;
				return ;
			}
		}
		else if ( ident == FUNCTION ) {
			needchar(')') ;
		}

	} while ( cmatch(',') ) ;
	ns() ;
}

/*
 *	Declare local variables (i.e. define for use)
 *
 *  works just like "declglb" but modifies machine stack
 *  and adds symbol table entry with appropriate
 *  stack offset to find it again
 */
declloc(typ, otag)
int typ ;			/* typ is CCHAR, CINT DOUBLE or STRUCT */
TAG_SYMBOL *otag ;	/* tag of struct for object being declared */
{
	char sname[NAMESIZE];
	SYMBOL *cptr ;
	int size, ident, more, itag, type ;

	if ( swactive || noloc ) error("not allowed here (switch/goto)") ;
	if ( declared < 0 ) error("must declare first in block") ;
	do {
		if ( endst() ) break ;

		type = typ ;
		more = 						/* assume dummy symbol not required */
		itag = 0 ;

		ident = get_ident() ;

		if ( symname(sname) == 0 )
			illname() ;

		if ( ident == PTR_TO_FN ) {
			needtoken(Fnptrtail) ;
			ident = POINTER ;
		}

		if ( cmatch('[') ) {
			ptrerror(ident) ;
			if ( ident == POINTER ) {
				/* array of pointers needs dummy symbol */
				more = dummy_idx(typ, otag) ;
				type = CINT ;
			}
			size = needsub() ;
			if (size & 1 ) size++;			/*For TMS 9900 Keep PC even on stack*/
			ident = ARRAY ;			/* null subscript array is NOT a pointer */
			cscale(type, otag, &size);
		}
		else if ( ident == PTR_TO_PTR ) {
			ident = POINTER ;
			more = dummy_idx(typ, otag) ;
			type = CINT ;
			size = 2 ;
		}
		else if ( ident == POINTER )
			size = 2 ;
		else {
			switch ( type ) {
			case CCHAR :
				/*		size = 1; */
				size = 2; /* For TMS 99000 */
				break ;
			case DOUBLE :
				size = 6 ;
				break ;
			case STRUCT :
				size = otag->size ;
				if (size & 1) size++; /* For TMS 9900 storage must be even bytes */
				break ;
			default :
				size = 2 ;
			}
		}
		declared += size ;
		if ( otag )
			itag = otag - tagtab ;
		cptr = addloc(sname, ident, type, more, itag);
		if ( cptr )
			cptr->offset.i = Zsp - declared ;
	} while ( cmatch(',') ) ;
	ns() ;
}

/*
 * test for function returning/array of ptr to ptr (unsupported)
 */
ptrerror(ident)
int ident ;
{
	if ( ident == PTR_TO_PTR )
		error("indirection too deep") ;
}

/*
 * initialise global object
 */
initials(sname, type, ident, dim, more, tag)
char *sname ;
int type, ident, dim, more ;
TAG_SYMBOL *tag ;
{
	int size ;
	if (type == CINT || ident == POINTER || type == STRUCT) /* TMS990 Check added for non char types */
		even(); 											/*TMS9900 requirement for even boundary */
	outname(sname); col() ;

	if ( cmatch('=') ) {
		/* initialiser present */
		litptr = 0 ;
		litlab = getlabel() ;
		if ( dim == 0 ) dim = -1 ;
		size = (type == CINT || type == UCINT) ? 2 : 1 ;

		if ( cmatch('{') ) {
			/* aggregate initialiser */
			if ( (ident == POINTER || ident == VARIABLE) && type == STRUCT ) {
				/* aggregate is structure or pointer to structure */
				dim = 0 ;
				if ( ident == POINTER ) point() ;
				str_init(tag) ;
			}
			else {
				/* aggregate is not struct or struct pointer */
				agg_init(size, type, ident, &dim, more, tag) ;
			}
			needchar('}');
		}
		else {
			/* single initialiser */
			init(size, ident, &dim, more, 0) ;
		}

		/* dump literal queue and fill tail of array with zeros */
		if ( (ident == ARRAY && more == CCHAR) || type == STRUCT ) {
			if ( type == STRUCT )
				dumpzero(tag->size, dim) ;
			else
				dumpzero(size, dim) ;
			dumplits(1, YES) ;
		}
		else {
			dumplits(size, NO) ;
			dumpzero(size, dim) ;
		}
	}
	else {
		/* no initialiser present, let loader insert zero */
		defstorage();
		if ( ident == POINTER )
			type = CINT ;
		cscale(type, tag, &dim) ;
		outdec(dim);
		nl() ;
	}		
}

/*
 * initialise structure
 */
str_init(tag)
TAG_SYMBOL *tag ;
{
	int dim ;
	SYMBOL *ptr ;

	ptr = tag->ptr ;
	while ( ptr < tag->end ) {
		init((ptr->type==CINT) ? 2 : 1, ptr->ident, &dim, ptr->more, 1) ;
		++ptr ;
		if ( cmatch(',') == 0 && ptr != tag->end ) {
			error("out of data") ;
			break ;
		}
	}
}

/*
 * initialise aggregate
 */
agg_init(size, type, ident, dim, more, tag)
int size, type, ident, *dim, more ;
TAG_SYMBOL *tag ;
{
	while ( *dim ) {
		if ( ident == ARRAY && type == STRUCT ) {
			/* array of struct */
			needchar('{') ;
			str_init(tag) ;
			--*dim ;
			needchar('}') ;
		}
		else {
			init(size, ident, dim, more, (ident == ARRAY && more == CCHAR) ) ;
		}
		if ( cmatch(',') == 0 ) break ;
	}
}

/*
 * evaluate one initialiser
 *
 * if dump is TRUE, dump literal immediately
 * save character string in litq to dump later
 * this is used for structures and arrays of pointers to char, so that the
 * struct or array is built immediately and the char strings are dumped later
 */
init(size, ident, dim, more, dump)
int size, ident, *dim, more, dump ;
{
	int value ;

	if ( qstr(&value) ) {
		if (ident == VARIABLE || (size != 1 && more != CCHAR) )
			error("must assign to char pointer or array");
		if ( dump ) {
			/* array of pointers to char or char ptr in struct */
			defword() ;
			printlabel(litlab) ;
			outbyte('+') ;
			outdec(value) ;
			nl() ;
			--*dim ;
		}
		else {
			/* array of or pointer to char */
			*dim -= (litptr - value) ;
			if (ident == POINTER) point() ;
		}
	}
	else if ( constexpr(&value) ) {
		if ( ident == POINTER ) {
			/* the only constant which can be assigned to a pointer is 0 */
			if ( value != 0 )
				error("cannot assign to pointer") ;
			size = 2 ;
		}
		if ( dump ) {
			/* struct member or array of pointer to char */
			if ( size == 1 ) defbyte() ;
			else defword() ;
			outdec(value) ;
			nl() ;
		}
		else
			stowlit(value, size) ;
		--*dim ;
	}
}

/*
 *	Get required array size
 *
 * invoked when declared variable is followed by "["
 *	this routine makes subscript the absolute
 *	size of the array.
 */
needsub()
{
	int num;

	if ( cmatch(']') ) return 0 ;	/* null size */
	if ( constexpr(&num) == 0 ) {
		num = 1 ;
	}
	else if ( num < 0 ) {
		error("negative size illegal");
		num = (-num) ;
	}
	needchar(']') ;		/* force single dimension */
	return num ;		/* and return size */
}

/*
 *	Begin a function
 *
 * Called from "parse" and "declglb" this routine tries to make a function
 *	out of what follows.
 */
newfunc(nam, seen_close)
char *nam ;
int seen_close ;
{
	char n[NAMESIZE],		/* ptr => currfn */
		 sname[NAMESIZE] ;	/* structure name for structure argument */
	SYMBOL *prevarg;		/* ptr to symbol table entry of most recent argument */
	SYMBOL *cptr ;			/* temporary symbol table pointer */
	TAG_SYMBOL *otag ;		/* structure tag for structure argument */
	int lgh,				/* size (bytes) of an argument */
		where,				/* offset to argument in stack (zero for last argument) */
		sflag,				/* TRUE to declare struct members, FALSE for union */
		*iptr;				/* temporary ptr for stepping along arg chain */
	int ret ;				/* return value from dotype */

	if ( nam ) {
		strcpy(n, nam) ;
	}
	else if ( symname(n) == 0 ) {
		error("illegal function or declaration");
		clear();	/* invalidate line */
		return;
	}

	nogo =					/* enable goto statements */
	noloc =					/* enable block-local declarations */
	lastst = 				/* no last statement */
	litptr = 0 ;			/* clear literal pool */
	litlab = getlabel() ;
	locptr = STARTLOC ;		/* deallocate all locals */
	fnstart = lineno ;		/* remember where fn began */
	infunc = 1 ;			/* note, in function now */

	/* check symbol against any previous declaration */
	currfn = findglb(n) ;
	/* We have found the function previously declared so we can set the ENT class */
	if ( currfn ) currfn->class = ENT;
	if ( nam == NULL ) {
		/* this is function definition without type, i.e. fn returning int */
		if ( currfn ) {

			/* symbol has been declared before, check type */
			if ( currfn->ident != FUNCTION || currfn->type != CINT ||
						currfn->more != 0 ) {
				/* previously declared with different type */
				multidef() ;
			}
			/*  if (currfn->ident == FUNCTION)
				/* same type, now check storage */
			if ( currfn->offset.i == EXTERNAL ) {
				/* previous declaration was EXTERNAL, this is definition */
				currfn->offset.i = STATIK ;
			}
			else {
				/* there can be just one STATIK declaration */
				multidef() ;
			}
		}
		else {
			/* if not in table, define as a function now */
			/* new functions are declared as Entry ENT */
				currfn = addglb(n, FUNCTION, CINT, ENT, STATIK, 0, 0) ;
		}
	}
	else if ( currfn == NULL ) {
		/* should have seen declaration earlier */
		error("missing declaration") ;
	}

	toconsole();
	outstr("====== "); outstr(currfn->name); outstr("()\n");
	tofile();

	/* we had better see open paren for args... */
	if ( nam == NULL && cmatch('(') == 0 )
		error("missing open paren");
	even(); /* TMS9900 ensure all functions begin on even boundary A99 Requirement */
	outname(n);col();nl();	/* print function name */
	locptr = STARTLOC ;		/* "clear" local symbol table */
	prevarg = 0 ;			/* initialize ptr to prev argument */
	undeclared = 0 ;		/* init arg count */

	if ( seen_close == NO ) {
		while ( cmatch(')') == 0 ) {	/* then count args */
			/* any legal name bumps arg count */
			if ( symname(n) ) {
				/* add link to argument chain */
				if ( (cptr=addloc(n,0,CINT,0,0)) )
					cptr->offset.p = prevarg ;
				prevarg = cptr ;
		    	++undeclared ;
			}
			else {
				error("illegal argument name");
				junk();
			}
			blanks();
			/* if not closing paren, should be comma */
			if ( ch() != ')' && cmatch(',') == 0 ) {
				error(Comma);
			}
			if ( endst() ) break ;
		}
	}

	Zsp = 0 ;			/* preset stack ptr */
	while ( undeclared ) {
		/* now let user declare the arguments */
		sflag = 0 ;
		switch ( (ret=dotype()) ) {
		case CCHAR :
		case CINT:
		case UCCHAR :
		case UCINT :
		case DOUBLE :
			getarg(ret, NULL) ;
			break ;
		case STRUCT :
			sflag = 1 ;
		case UNION :
			if ( symname(sname) == 0 )
				illname() ;
			if ( (otag=findtag(sname)) == 0 ) {
				/* structure not previously defined */
				otag = defstruct(sname, STATIK, sflag) ;
			}
			getarg(STRUCT, otag) ;
			break ;
		default : 
			error("wrong number args") ;
			undeclared = 0 ;
			break ;
		}
	}
	where = 2 ;
	while ( prevarg ) {
		lgh = 2 ;	/* all arguments except DOUBLE have length 2 bytes (even char) */
		if ( prevarg->type == DOUBLE && prevarg->type != POINTER )
			lgh=6;
		iptr = &prevarg->offset.i ;
		prevarg = prevarg->offset.p ;		/* follow ptr to prev. arg */
		*iptr = where ;						/* insert offset */
		where += lgh ;						/* calculate next offset */
	}
	statement() ;
	if ( lastst != STRETURN  && lastst != STGOTO ) {
		/* do a statement, but if it's a return, skip */
		/* cleaning up the stack */
		leave(NO) ;
	}

	/* dump literal queue, with label */
	dumplits(1, YES) ;
	infunc = 0 ;			/* not in fn. any more */
}

/*
 *	Declare argument types
 *
 * called from "newfunc" this routine adds an entry in the
 *	local symbol table for each named argument
 */
getarg(typ, otag)
int typ ;			/* typ = CCHAR, CINT, UINT, DOUBLE or STRUCT */
TAG_SYMBOL *otag ;	/* structure tag for STRUCT type objects */
{
	char n[NAMESIZE] ;
	SYMBOL *argptr ;
	int legalname, ident ;

	while ( undeclared ) {
		ident = get_ident() ;
		if ( (legalname=symname(n)) == 0 )
			illname();
		if ( ident == PTR_TO_FN ) {
			needtoken(Fnptrtail) ;
			ident = POINTER ;
		}
		if ( cmatch('[') ) {	/* pointer ? */
			ptrerror(ident) ;
			/* it is a pointer, so skip all */
			/* stuff between "[]" */
			while ( inbyte() != ']' )
				if ( endst() ) break;
			/* add entry as pointer */
			ident = (ident == POINTER) ? PTR_TO_PTR : POINTER ;
		}
		if ( legalname ) {
			if ( argptr=findloc(n) ) {
				/* add in details of the type of the name */
				if ( ident == PTR_TO_PTR ) {
					argptr->ident = POINTER ;
					argptr->type = CINT ;
					argptr->more = dummy_idx(typ, otag) ;
				}
				else {
					argptr->ident = ident ;
					argptr->type = typ ;
				}
			}
			else error("expected argument");
			if ( otag ) {
				argptr->tag_idx = otag - tagtab ;
				argptr->ident = POINTER ;
				argptr->type = STRUCT ;
			}
		}
		--undeclared;	/* cnt down */
		if ( endst() )
			break ;
		if ( cmatch(',') == 0 )
			error(Comma) ;
	}
	ns() ;
}
