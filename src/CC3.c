/*
 * cc3.c - third part of Small-C/Plus compiler
 */

#include <stdio.h>
#include <string.h>
#include "ccdefs.h"
#include "ccfunc.h"

extern SYMBOL *symtab, *glbptr, *loctab, *locptr;
extern int glbcnt;
extern TAG_SYMBOL *tagtab, *tagptr;
extern SYMBOL *membtab, *membptr;
extern WHILE_TAB *wqueue, *wqptr;
extern char macq[];
extern int macptr;
extern char *stage, *stagenext, *stagelast;
extern char line[], mline[];
extern int skiplevel, iflevel;
extern int lptr;
extern int mptr;
extern int nxtlab, Zsp, errcnt, errstop, eof;
extern int ctext, cmode, fnstart, lineno, infunc;
extern SYMBOL *currfn;
extern FILE *inpt2, *input, *output, *saveout;

#ifdef SMALL_C
extern int minavail;
#endif

/*
 *	Perform a function call
 *
 * called from heirb, this routine will either call
 *	the named function, or if the supplied ptr is
 *	zero, will call the contents of HL
 */
callfunction(ptr)
	SYMBOL *ptr; /* symbol table entry (or 0) */
{
	int nargs, con, val;

	nargs = 0;
	blanks(); /* already saw open paren */

	while (ch() != ')') {
		if (endst())
			break;
		if (ptr) {
			/* ordinary call */
			if (expression(&con, &val) == DOUBLE) {
				fpush();
				nargs += 6;
			} else {
				zpush();
				nargs += 2;
			}
		} else { /* call to address in HL */
			zpush(); /* push argument */
			if (expression(&con, &val) == DOUBLE) {
				fpush2();
				nargs += 6;
			} else {
				nargs += 2;
			}
			swapstk();
		}
		if (cmatch(',') == 0)
			break;
	}
	needchar(')');
	if (ptr) {
		if (nospread(ptr->name)) {
			loadargc(nargs);
		}
		zcall(ptr);
	} else
		callstk(nargs);
	Zsp = modstk(Zsp + nargs, YES); /* clean up arguments */
}

nospread(sym)
	char *sym; {
	if (strcmp(sym, "printf") == 0)
		return 1;
	if (strcmp(sym, "fprintf") == 0)
		return 1;
	if (strcmp(sym, "sprintf") == 0)
		return 1;
	if (strcmp(sym, "scanf") == 0)
		return 1;
	if (strcmp(sym, "fscanf") == 0)
		return 1;
	if (strcmp(sym, "sscanf") == 0)
		return 1;
	return 0;
}

junk() {
	if (an(inbyte()))
		while (an(ch()))
			gch();
	else
		while (an(ch()) == 0) {
			if (ch() == 0)
				break;
			gch();
		}
	blanks();
}

endst() {
	blanks();
	return (ch() == ';' || ch() == 0);
}

illname() {
	error("illegal symbol name");
	junk();
}

multidef() {
	error("already defined");
}

char missing[] = "missing token";

needtoken(str)
	char *str; {
	if (match(str) == 0)
		error(missing);
}

needchar(c)
	char c; {
	if (cmatch(c) == 0)
		error(missing);
}

needlval() {
	error("must be lvalue");
}

hash(sname)
	char *sname; {
	int c, h;

	h = *sname;
	while (c = *(++sname))
		h = (h << 1) + c;
	return (h & MASKGLBS);
}

/*
 * find entry in global symbol table,
 * glbptr is set to relevant entry
 * return pointer if match is found,
 * else return zero and glbptr points to empty slot
 */

SYMBOL* findglb(sname)
	char *sname; {
	glbptr = STARTGLB + hash(sname);
	while (strcmp(sname, glbptr->name)) {
		if (glbptr->name[0] == 0)
			return 0;
		++glbptr;
		if (glbptr == ENDGLB)
			glbptr = STARTGLB;
	}
	return glbptr;
}

SYMBOL* findloc(sname)
	char *sname; {
	SYMBOL *ptr;

	ptr = STARTLOC;
	while (ptr != locptr) {
		if (strcmp(sname, ptr->name) == 0)
			return ptr;
		++ptr;
	}
	return 0;
}

/*
 * find symbol in structure tag symbol table, return 0 if not found
 */

TAG_SYMBOL* findtag(sname)
	char *sname; {
	TAG_SYMBOL *ptr;

	ptr = STARTTAG;
	while (ptr != tagptr) {
		if (strcmp(ptr->name, sname) == 0)
			return ptr;
		++ptr;
	}
	return 0;
}

/*
 * determine if 'sname' is a member of the struct with tag 'tag'
 * return pointer to member symbol if it is, else 0
 */

SYMBOL* findmemb(tag, sname)
	TAG_SYMBOL *tag;char *sname; {
	SYMBOL *ptr;

	ptr = tag->ptr;

	while (ptr < tag->end) {
		if (strcmp(ptr->name, sname) == 0)
			return ptr;
		++ptr;
	}
	return 0;
}

char Overflow[] = "symbol table overflow";

SYMBOL * addglb(sname, id, typ, class, value, more, itag)
char *sname, id, typ, class ;
int value, more, itag;

{
	char *sname2, c;
	if ( findglb(sname) ) {
		multidef() ;
		return glbptr ;
	}
	if ( id != MACRO && *sname != '0' ) {
		/* declare exported name */
	/*	ot("global ");
		outname(sname); nl(); */ /* TMS900 not needed when using a relocating linker loader */
	}
	if ( glbcnt >= NUMGLBS-1 ) {
		error(Overflow);
		return 0;
	}
	addsym(glbptr, sname, id, typ, class, more, itag) ;
	glbptr->offset.i = value ;
	++glbcnt ;
	return glbptr;
}

SYMBOL* addloc(sname, id, typ, more, itag)
	char *sname, id, typ;
	int more, itag; {
	SYMBOL *cptr;

	if (cptr = findloc(sname)) {
		multidef();
		return cptr;
	}
	if (locptr >= ENDLOC) {
		error(Overflow);
		return 0;
	}
	cptr = locptr++;
	addsym(cptr, sname, id, typ, 0, more, itag) ;
	return cptr;
}

/*
 * add new structure member to table
 */
addmemb(sname, id, typ, class, value, storage, more, itag)
char *sname, id, typ, class ;
int value, storage, more, itag ;
{
	if ( membptr >= ENDMEMB ) {
		error(Overflow) ;
		ccabort() ;
	}
	addsym(membptr, sname, id, typ, class, more, itag) ;
	membptr->offset.i = value ;
	++membptr ;
}

/*
 * insert values into symbol table
 */
addsym(ptr, sname, id, typ, class, more, itag)
SYMBOL *ptr ;
char *sname, id, typ, class ;
int more, itag ;
{
	char mtyp;
	strcpy(ptr->name, sname) ;
	ptr->ident = id ;
	ptr->modifier = (typ == UCCHAR || typ == UCINT) ? UNSGND: 0;
	ptr-> type = typ;
	if (typ == UCCHAR) ptr-> type = CCHAR;
	if (typ == UCINT) ptr-> type = CINT;
	ptr->class = class ;
	ptr->more = more ;
	ptr->tag_idx = itag ;
}

/*
 * get integer of length len bytes from address addr
 */
getint(addr, len)
	char *addr;int len; {
	int i;

	i = *(addr + --len); /* high order byte sign extended */
	while (len--)
		i = (i << 8) | (*(addr + len) & 255);
	return i;
}

/*
 * put integer of length len bytes into address addr
 * (low byte first)
 */
putint(i, addr, len)
	char *addr;int i, len; {
	while (len--) {
		*addr++ = i;
		i >>= 8;
	}
}

/*
 * Test if next input string is legal symbol name
 * if it is, truncate it and copy it to sname
 */
symname(sname)
	char *sname; {
	int k;

	/* Added for TMS 9900 Implementation */
	k = 0;
#ifdef SMALL_C
	{
		char *p;
		char c;

		/* this is about as deep as nesting goes, check memory left */
		p = alloc(1);
		/* &c is top of stack, p is end of heap */
		if ((k = &c - p) < minavail)
			minavail = k;
		free(p);
	}
#endif

	blanks();
	if (alpha(ch()) == 0)
		return (*sname = 0);
	k = 0;
	while (an(ch())) {
		sname[k] = gch();
		if (k < NAMEMAX)
			++k;
	}
	sname[k] = 0;
	return 1;
}

/* Return next avail internal label number */
getlabel() {
	return (++nxtlab);
}

/* Print specified number as label */
printlabel(label)
	int label; {
	outstr("cc");
	outdec(label);
}

/* print label with colon and newline */
postlabel(label)
	int label; {
	printlabel(label);
	col();
	nl();
}

/* Test if given character is alpha */
alpha(c)
	char c; {
	if (c >= 'a')
		return (c <= 'z');
	if (c <= 'Z')
		return (c >= 'A');
	return (c == '_');
}

/* Test if given character is numeric */
numeric(c)
	char c; {
	if (c <= '9')
		return (c >= '0');
	return 0;
}

/* Test if given character is alphanumeric */
an(c)
	char c; {
	if (alpha(c))
		return 1;
	return numeric(c);
}

/* Print a carriage return and a string only to console */
pl(str)
	char *str; {
	putchar('\n');
	while (*str)
		putchar(*str++);
}

addwhile(ptr)
	WHILE_TAB *ptr; {
	wqptr->sp = ptr->sp = Zsp; /* record stk ptr */
	wqptr->loop = ptr->loop = getlabel(); /* and looping label */
	wqptr->exit = ptr->exit = getlabel(); /* and exit label */
	if (wqptr >= WQMAX) {
		error("too many active whiles");
		return;
	}
	++wqptr;
}

delwhile() {
	if (wqptr > wqueue)
		--wqptr;
}

WHILE_TAB* readwhile(ptr)
	WHILE_TAB *ptr; {
	if (ptr <= wqueue) {
		error("out of context");
		return 0;
	} else
		return (ptr - 1);
}

ch() {
	return line[lptr];
}

nch() {
	if (ch())
		return line[lptr + 1];
	return 0;
}

nch2() {
	if (ch())
		return line[lptr + 2];
	return 0;
}

gch() {
	if (ch())
		return line[lptr++];
	return 0;
}

clear() {
	lptr = 0;
	line[0] = 0;
}

inbyte() {
	while (ch() == 0) {
		if (eof)
			return 0;
		preprocess();
	}
	return gch();
}

inline()
{
	FILE *unit;
	int k;

	while(1) {
		if ( input == NULL ) openin();
		if ( eof ) return;
		if( (unit=inpt2) == NULL ) unit = input;
		clear();
		while ( (k=getc(unit)) > 0 ) {
			if ( k == '\n' || lptr >= LINEMAX ) break;
			line[lptr++] = k;
		}
		line[lptr] = 0; /* append null */
		++lineno; /* read one more line */
		if ( k <= 0 ) {
			fclose(unit);
			if ( inpt2 != NULL ) endinclude();
			else input = 0;
		}
		if ( lptr ) {
			if( ctext && cmode ) {
				comment();
				outstr(line);
				nl();
			}
			lptr=0;
			return;
		}
	}
}

/*
 * ifline - part of preprocessor to handle #ifdef, etc
 */
ifline() {
	char sname[NAMESIZE];

	while (1) {

		inline();
		if (eof)
			return;

		if (ch() == '#') {

			if (match("#undef")) {
				delmac();
				continue;
			}

			if (match("#ifdef")) {
				++iflevel;
				if (skiplevel)
					continue;
				symname(sname);
				if (findmac(sname) == 0)
					skiplevel = iflevel;
				continue;
			}

			if (match("#ifndef")) {
				++iflevel;
				if (skiplevel)
					continue;
				symname(sname);
				if (findmac(sname))
					skiplevel = iflevel;
				continue;
			}

			if (match("#else")) {
				if (iflevel) {
					if (skiplevel == iflevel)
						skiplevel = 0;
					else if (skiplevel == 0)
						skiplevel = iflevel;
				} else
					noiferr();
				continue;
			}

			if (match("#endif")) {
				if (iflevel) {
					if (skiplevel == iflevel)
						skiplevel = 0;
					--iflevel;
				} else
					noiferr();
				continue;
			}
		}

		if (skiplevel)
			continue;

		if (ch() == 0)
			continue;

		break;
	}
}

noiferr() {
	error("no matching #if");
}

keepch(c)
	char c; {
	mline[mptr] = c;
	if (mptr < MPMAX)
		++mptr;
}

preprocess() {
	char c, sname[NAMESIZE];
	int k;

	ifline();
	if (eof || cmode == 0) {
		/* while passing through assembler, only do #if, etc */
		return;
	}
	mptr = lptr = 0;
	while (ch()) {
		if (ch() == ' ' || ch() == '\t') {
			keepch(' ');
			while (ch() == ' ' || ch() == '\t')
				gch();
		} else if (ch() == '"') {
			keepch(ch());
			gch();
			while (ch() != '"' || (line[lptr - 1] == 92 && line[lptr - 2] != 92)) {
				if (ch() == 0) {
					error("missing quote");
					break;
				}
				keepch(gch());
			}
			gch();
			keepch('"');
		} else if (ch() == 39) {  /* Single quote, apostrophe */
			keepch(39);
			gch();
			while (ch() != 39 || (line[lptr - 1] == 92 && line[lptr - 2] != 92)) {
				if (ch() == 0) {
					error("missing apostrophe");
					break;
				}
				keepch(gch());
			}
			gch();
			keepch(39);
		} else if (ch() == '/' && nch() == '*') {
			lptr += 2;
			while (ch() != '*' || nch() != '/') {
				if (ch()) {
					++lptr;
				} else {
					inline();
					if (eof)
						break;
				}
			}
			lptr += 2;
		}
		/* Added to allow comments to be made using the forward slash TMS 9900*/

		else if (ch() == '/' && nch() == '/') {
			lptr += 2;
			inline();
			if (eof)
				break;

		} else if (alpha(ch())) {
			k = 0;
			while (an(ch())) {
				if (k < NAMEMAX)
					sname[k++] = ch();
				gch();
			}
			sname[k] = 0;
			if (k = findmac(sname))
				while (c = macq[k++])
					keepch(c);
			else {
				k = 0;
				while (c = sname[k++])
					keepch(c);
			}
		} else
			keepch(gch());
	}
	keepch(0);  /* terminate with null */
	if (mptr >= MPMAX)
		error("line too long");
	/* move expanded line (mline) into parse line (line) */
	/* use lptr as temporary storage, and set it up correctly */
	/*	lptr = mline ;
	 mline = line ;
	 line = lptr ; */
	strcpy(line, mline);
	lptr = 0;
}

addmac() {
	char sname[NAMESIZE];

	if (symname(sname) == 0) {
		illname();
		clear();
		return;
	}
	addglb(sname, MACRO, 0, 0, macptr, 0, 0) ;
	while (ch() == ' ' || ch() == '\t')
		gch();
	while (putmac(gch()))
		;
	if (macptr >= MACMAX)
		error("macro table full");
}

/*
 * delete macro from symbol table, but leave entry so hashing still works
 */
delmac() {
	char sname[NAMESIZE];
	SYMBOL *ptr;

	if (symname(sname)) {
		if ((ptr = findglb(sname))) {
			/* invalidate name */
			ptr->name[0] = '0';
		}
	}
}

putmac(c)
	char c; {
	macq[macptr] = c;
	if (macptr < MACMAX)
		++macptr;
	return c;
}

findmac(sname)
	char *sname; {
	if (findglb(sname) != 0 && glbptr->ident == MACRO) {
		return glbptr->offset.i;
	}
	return 0;
}

/*
 * defmac - takes macro definition of form name[=value] and enters
 *          it in table.  If value is not present, set value to 1.
 *          Uses some shady manipulation of the line buffer to set
 *          up conditions for addmac().
 */
defmac(text)
	char *text; {
	char *p;

	/* copy macro name into line buffer */
	p = &line;
	while (*text != '=' && *text) {
		*p++ = *text++;
	}
	*p++ = ' ';
	/* copy value or "1" into line buffer */
	strcpy(p, (*text++) ? text : "1");
	/* make addition to table */
	lptr = 0;
	addmac();
}

/* initialise staging buffer */

setstage(before, start)
	char **before, **start; {
	if ((*before = stagenext) == 0)
		stagenext = stage;
	*start = stagenext;
}

/* flush or clear staging buffer */

clearstage(before, start)
	char *before, *start; {
	*stagenext = 0;
	if (stagenext = before)
		return;

	if (start) {
#ifdef OPTIMIZE
		peephole(start, output);
#else
		if (output != NULL) {
			if (fputs(start, output) == -1) {
				fabort();
			}
		} else {
			fputs(start, stdout);
		}
#endif
	}
}

fabort() {
	closeout();
	error("Output file error");
	ccabort();
}

/* direct output to console */
toconsole() {
	saveout = output;
	output = 0;
}

/* direct output back to file */
tofile() {
	if (saveout)
		output = saveout;
	saveout = 0;
}

outbyte(c)
	char c; {
	if (c) {
		if (output != NULL) {
			if (stagenext) {
				return (outstage(c));
			} else {
				if ((putc(c, output)) <= 0) {
					fabort();
				}
			}
		} else
			putchar(c);
	}
	return c;
}

/* output character to staging buffer */

outstage(c)
	char c; {
	if (stagenext == stagelast) {
		error("staging buffer overflow");
		return 0;
	}
	return *stagenext++ = c;
}

outstr(ptr)
	char *ptr; {
	if (output == NULL)
		fputs(ptr, stdout);
	else if (stagenext) {
		while (*ptr)
			outstage(*ptr++);
	} else {
		if (fputs(ptr, output) == -1) {
			fabort();
		}
	}
}

nl() {
	outbyte('\n');
}

tab() {
	outbyte('\t');
}

col() {
	outbyte(58);
}

bell() {
	outbyte(7);
}

error(ptr)
	char ptr[]; {
	char *s;

	toconsole();
	bell();
	outstr("Line ");
	outdec(lineno);
	outstr(", ");
	if (infunc == 0)
		outbyte('(');
	if (currfn == NULL)
		outstr("start of File");
	else
		outstr(currfn->name);
	if (infunc == 0)
		outbyte(')');
	outstr(" + ");
	outdec(lineno - fnstart);
	outstr(": ");
	outstr(ptr);
	nl();

	outstr(line);
	nl();

	s = line; /* skip to error position */
	while (s < lptr) {
		if (*s++ == '\t')
			tab();
		else
			outbyte(' ');
	}
	outbyte('^');
	nl();
	++errcnt;

	if (errstop) {
		pl("Continue (Y/N) ? ");
		if (raise(getchar()) == 'N')
			ccabort();
	}
	tofile();
}

ol(ptr)
	char *ptr; {
	ot(ptr);
	nl();
}

ot(ptr)
	char *ptr; {
/*	tab(); */
	outstr(ptr);
}

blanks() {
	while (1) {
		while (ch() == 0) {
			preprocess();
			if (eof)
				break;
		}
		if (ch() == ' ')
			gch();
		else if (ch() == 9)
			gch();
		else
			return;
	}
}

streq(str1, str2)
	char str1[], str2[]; {
	int k;
	k = 0;
	while (*str2) {
		if ((*str1++) != (*str2++))
			return 0;
		++k;
	}
	return k;
}

/*
 * Compare strings.
 * Match only if we reach end of both strings or if, at end of one of the
 * strings, the other one has reached a non-alphanumeric character
 * (so that, for example, astreq("if", "ifline") is not a match)
 */
astreq(str1, str2)
	char *str1, *str2; {
	int k;

	k = 0;
	while (*str1 && *str2) {
		if (*str1 != *str2)
			break;
		++str1;
		++str2;
		++k;
	}
	if (an(*str1) || an(*str2))
		return 0;
	return k;
}

match(lit)
	char *lit; {
	int k;

	blanks();
	if (k = streq(line + lptr, lit)) {
		lptr += k;
		return 1;
	}
	return 0;
}

cmatch(lit)
	char lit; {
	blanks();
	if (line[lptr] == lit) {
		++lptr;
		return 1;
	}
	return 0;
}

amatch(lit)
	char *lit; {
	int k;

	blanks();
	if (k = astreq(line + lptr, lit)) {
		lptr += k;
		return 1;
	}
	return 0;
}

outdec(number)
	int number; {
	if (number < 0) {
		number = (-number);
		outbyte('-');
	}
	outd2(number);
}

outd2(n)
	int n; {
	if (n > 9) {
		outd2(n / 10);
		n %= 10;
	}
	outbyte('0' + n);
}

/* convert lower case to upper */
raise(c)
	char c; {
	if (c >= 'a') {
		if (c <= 'z')
			c -= 32; /* 'a'-'A'=32 */
	}
	return c;
}
