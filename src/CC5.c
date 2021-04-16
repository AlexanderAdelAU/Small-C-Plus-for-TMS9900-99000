/*
 * cc5.c - fifth part of Small-C/Plus compiler
 *         miscellaneous routines required during recursive descent
 */

#include "stdio.h"
#include "ccdefs.h"
#include "cclvalue.h"
#include "ccfunc.h"
typedef long long unsigned int real;
#define asreal_f(x) (*((float *) &x))
#define asreal_d(x) (*((double *) &x))

char fval[16];
/*
 #ifdef SMALL_C
 #include "ccfloat.h"
 #endif

 */

extern TAG_SYMBOL *tagtab;
extern char *litq;
extern int litptr;
extern char line[];
extern int lptr;
extern int Zsp;

int primary(lval)
	LVALUE *lval; {
	char sname[NAMESIZE];
	SYMBOL *ptr;
	int k;
	if (cmatch('(')) {
		do
			k = heir1(lval);
		while (cmatch(','));
		needchar(')');
		return k;
	}
	/* clear lval array */
	putint(0, lval, sizeof(LVALUE));
	if (symname(sname)) {
		if (strcmp(sname, "sizeof") == 0) {
			size_of(lval);
			return 0;
		} else if (ptr = findloc(sname)) {
			/*	ptr->class = 0;	/* Reset to non EXT  9900 */
			getloc(ptr, 0);
			lval->symbol = ptr;
			lval->val_type = lval->indirect = ptr->type;
			if (ptr->type == STRUCT)
				lval->tagsym = tagtab + ptr->tag_idx;

			if (ptr->ident == POINTER) {
				lval->indirect = CINT;
				lval->ptr_type = ptr->type;
				lval->val_type = CINT;
			}
			if (ptr->ident == ARRAY
					|| (ptr->ident == VARIABLE && ptr->type == STRUCT)) {
				lval->ptr_type = ptr->type;
				lval->val_type = CINT;
				return 0;
			} else
				return 1;
		}
		if (ptr = findglb(sname)) {
			if (ptr->ident != FUNCTION) {
				lval->symbol = ptr;
				lval->indirect = 0;
				lval->val_type = ptr->type;
				if (ptr->type == STRUCT)
					lval->tagsym = tagtab + ptr->tag_idx;
				if (ptr->ident != ARRAY
						&& (ptr->ident != VARIABLE || ptr->type != STRUCT)) {
					if (ptr->ident == POINTER) {
						lval->ptr_type = ptr->type;
						lval->val_type = CINT;
					}
					return 1;
				}
				/* object is global array or structure, load address */
				address(ptr->name);
				lval->indirect = lval->ptr_type = ptr->type;
				lval->val_type = CINT;
				return 0;
			}
		} else
			/* assume it's a function we haven't seen yet */
			/* NB value set to 0 and declare as class External EXT*/
			ptr = addglb(sname, FUNCTION, CINT, EXT, 0, STATIK, 0, 0);
		lval->symbol = ptr;
		lval->indirect = 0;
		lval->val_type = CINT;
		return 0;
	}
	if (constant(lval)) {
		lval->symbol = NULL_SYM;
		lval->indirect = 0;
		return 0;
	} else {
		error("invalid expression");
		const1(0);
		junk();
		return 0;
	}
}

/*
 * flag error if integer constant is found in double expression
 */
dcerror(lval)
	LVALUE *lval; {
	if (lval->val_type == FLOAT)
		error("int const in double expr");
}

/*
 * calculate constant expression
 */
calc(left, oper, right)
	int left, (*oper)(), right; {
	if (oper == zor)
		return (left | right);
	else if (oper == zxor)
		return (left ^ right);
	else if (oper == zand)
		return (left & right);
	else if (oper == mult)
		return (left * right);
	else if (oper == div)
		return (left / right);
	else if (oper == asr)
		return (left >> right);
	else if (oper == asl)
		return (left << right);
	else if (oper == zmod)
		return (left % right);
	else if (oper == zeq)
		return (left == right);
	else if (oper == zne)
		return (left != right);
	else if (oper == zle)
		return (left <= right);
	else if (oper == zge)
		return (left >= right);
	else if (oper == zlt)
		return (left < right);
	else if (oper == zgt)
		return (left > right);
	else
		return 0;
}

/* Complains if an operand isn't int */
intcheck(lval, lval2)
	LVALUE *lval, *lval2; {
	if (lval->val_type == FLOAT || lval2->val_type == FLOAT)
		error("operands must be int");
}

/* Forces result, having type t2, to have type t1 */
force(t1, t2)
	int t1, t2; {
	if (t1 == FLOAT) {
		if (t2 != FLOAT)
			fpcall("_float##");
	} else if (t2 == FLOAT) {
		if (t1 != FLOAT)
			fpcall("_ifix##");
	}
}

/*
 * If only one operand is FLOAT, converts the other one to
 * FLOAT.  Returns 1 if result will be FLOAT
 */
widen(lval, lval2)
	LVALUE *lval, *lval2; {
	if (lval2->val_type == FLOAT) {
		if (lval->val_type != FLOAT) {
			fpush2(); /* push 2nd operand UNDER 1st */
			mainpop();
			fpcall("_float##");
			fpcall("_fpswap##");
			lval->val_type = FLOAT; /* type of result */
		}
		return 1;
	} else {
		if (lval->val_type == FLOAT) {
			fpcall("_float##");
			return 1;
		} else
			return 0;
	}
}

/*
 * true if val1 -> int pointer or int array and
 * val2 not ptr or array
 */
dbltest(lval, lval2)
	LVALUE *lval, *lval2; {
	if (lval->ptr_type) {
		if (lval->ptr_type == CCHAR)
			return 0;
		if (lval2->ptr_type)
			return 0;
		return 1;
	} else
		return 0;
}

/*
 * determine type of binary operation
 */
result(lval, lval2)
	LVALUE *lval, *lval2; {
	if (lval->ptr_type && lval2->ptr_type)
		lval->ptr_type = 0; /* ptr-ptr => int */
	else if (lval2->ptr_type) { /* ptr +- int => ptr */
		lval->symbol = lval2->symbol;
		lval->indirect = lval2->indirect;
		lval->ptr_type = lval2->ptr_type;
	}
}

/*
 * prestep - preincrement or predecrement lvalue
 */
prestep(lval, n, step)
	LVALUE *lval;int n, (*step)(); {
	if (heira(lval) == 0) {
		needlval();
	} else {
		if (lval->indirect)
			zpush();
		rvalue(lval);
		intcheck(lval, lval);
		switch (lval->ptr_type) {
		case FLOAT:
			addconst(n * 6);
			break;
		case STRUCT:
			addconst(n * lval->tagsym->size);
			break;
		case CINT:
			(*step)();
		default:
			(*step)();
			break;
		}
		store(lval);
	}
}

/*
 * poststep - postincrement or postdecrement lvalue
 */
poststep(k, lval, n, step, unstep)
	int k;
	LVALUE *lval;int n, (*step)(), (*unstep)(); {
	if (k == 0) {
		needlval();
	} else {
		if (lval->indirect)
			zpush();
		rvalue(lval);
		intcheck(lval, lval);
		switch (lval->ptr_type) {
		case FLOAT:
			nstep(lval, n * 6);
			break;
		case STRUCT:
			nstep(lval, n * lval->tagsym->size);
			break;
		case CINT:
			(*step)();
		default:
			(*step)();
			store(lval);
			(*unstep)();
			if (lval->ptr_type == CINT)
				(*unstep)();
			break;
		}
	}
}

/*
 * generate code to postincrement by n
 */
nstep(lval, n)
	LVALUE *lval;int n; {
	zpush();
	addconst(n);
	store(lval);
	mainpop();
}

store(lval)
	LVALUE *lval; {
	if (lval->indirect == 0)
		putmem(lval->symbol);
	else
		putstk(lval);
}

/*
 * push address only if it's not that of a two byte quantity at TOS
 * or second TOS.  In either of those cases, forget address calculation
 * This should be followed by a smartstore()
 */
smartpush(lval, before)
	LVALUE *lval;char *before; {
	if (lval->indirect != CINT || lval->symbol == 0||
	lval->symbol->storage != STKLOC)
		zpush();
	else {
		switch (lval->symbol->offset.i - Zsp) {
		case 0:
		case 2:
			if (before)
				clearstage(before, 0);
			break;
		default:
			zpush();
		}
	}
}

/*
 * store thing in primary register at address taking account
 * of previous preparation to store at TOS or second TOS
 */
smartstore(lval)
	LVALUE *lval; {
	if (lval->indirect != CINT || lval->symbol == 0||
	lval->symbol->storage != STKLOC)
		store(lval);
	else {
		switch (lval->symbol->offset.i - Zsp) {
		case 0:
			puttos();
			break;
		case 2:
			put2tos();
			break;
		default:
			store(lval);
		}
	}
}

rvalue(lval)
	LVALUE *lval; {
	if (lval->symbol && lval->indirect == 0) {
		getmem(lval->symbol);
		/* lval->symbol->class =lval->symbol->class; */
	} else
		/*	indirect(lval->indirect); */
		indirect(lval); /* [AC] 9900  mod to pass lval  */
}

test(label, parens)
	int label, parens; {
	char *before, *start;
	LVALUE lval;
	int (*oper)();

	if (parens)
		needchar('(');
	while (1) {
		setstage(&before, &start);
		if (heir1(&lval))
			rvalue(&lval);
		if (cmatch(','))
			clearstage(before, start);
		else
			break;
	}
	if (parens)
		needchar(')');
	if (lval.is_const) { /* constant expression */
		clearstage(before, 0);
		if (lval.const_val) {
			/* true constant, perform body */
			return;
		}
		/* false constant, jump round body */
		jump(label);
		return;
	}
	if (lval.stage_add) { /* stage address of "..oper 0" code */
		oper = lval.binop; /* operator function pointer */
		if (oper == zeq || oper == ule)
			zerojump(eq0, label, &lval);
		else if (oper == zne || oper == ugt)
			zerojump(testjump, label, &lval);
		else if (oper == zgt)
			zerojump(gt0, label, &lval);
		else if (oper == zge)
			zerojump(ge0, label, &lval);
		else if (oper == uge)
			clearstage(lval.stage_add, 0);
		else if (oper == zlt)
			zerojump(lt0, label, &lval);
		else if (oper == ult)
			zerojump(jump(), label, &lval);
		else if (oper == zle)
			zerojump(le0, label, &lval);
		else
			testjump(label);
	} else
		testjump(label);
	clearstage(before, start);
}

/*
 * evaluate constant expression
 * return TRUE if it is a constant expression
 */
constexpr(val)
	int *val; {
	char *before, *start;
	int con;

	setstage(&before, &start);
	expression(&con, val);
	clearstage(before, 0); /* scratch generated code */
	if (con == 0)
		error("must be constant expression");
	return con;
}

/*
 * load constant into primary register
 */
const1(val)
	int val; {
	ol(";const1(val)");
	immed();
	outdec(val);
	nl();
}

/*
 * load constant into secondary register
 */
const2(val)
	int val; {
	ol(";const2(val)");
	immed2();
	outdec(val);
	nl();
}

/*
 * load constant into scratch register
 */
const3(val)
	int val; {
	immed3();
	outdec(val);
	nl();
}

/*
 * scale constant value according to type
 */
cscale(type, tag, val)
	int type;
	TAG_SYMBOL *tag;int *val; {
	switch (type) {
	case CCHAR:
		break;
	case CINT:
		*val <<= 1;
		break;
	case FLOAT:
		*val *= 6;
		break;
	case STRUCT:
		*val *= tag->size;
		break;
	}

}

/*
 * add constant to primary register
 */
addconst(val)
	int val; {
	switch (val) {

	/*	case -3 :	dec() ; */
	case -2:
		dect();
		break;
	case -1:
		dec();
		break;
	case 0:
		break;

		/*	case  3 :	inc() ; */
	case 2:
		inct();
		break;
	case 1:
		inc();
		break;

	default:
		const2(val);
		zadd();
	}
}

constant(lval)
	LVALUE *lval; {
	ol(";constant(lval)");
	lval->is_const = 1; /* assume constant will be found */
	if (fnumber(&lval->const_val)) {
		lval->val_type = FLOAT;
		immedlit();
		outdec(lval->const_val);
		nl();
		fpcall("_fload##");
		lval->is_const = 0; /*  floating point and not a constant */
		return 1;
	} else if (number(&lval->const_val) || pstr(&lval->const_val)) {
		lval->val_type = CINT;
		immed();
	} else if (qstr(&lval->const_val)) {
		lval->is_const = 0; /* string address not constant */
		lval->val_type = CINT;
		immedlit();
	} else {
		lval->is_const = 0;
		return 0;
	}
	outdec(lval->const_val);
	nl();
	return 1;
}

#ifdef SMALL_C
fnumber(val)
	int *val; {
	char *val2;
	unsigned char *dp; /* used to store the result */
	double sum, /* the partial result */
	scale; /* scale factor for next digit */
	real fpnum;
	char signbit; /*  sign bit  */

	int k, /* flag and mask */
	minus; /* negative if number is negative */
	char *start, /* copy of pointer to starting point */
	*s; /* points into source code */

	start = s = line + lptr; /* save starting point */
	k = minus = 1;

	--s;  /* back up to sign */
	while (k) {

		k = 0;
		if (*s == '+') {
			++s;
			k = 1;
		}
		if (*s == '-') {
			++s;
			k = 1;
			minus = (-minus);
		/*	toconsole(); */
		}
	}
	s++; /* reset the pointer */
	while (numeric(*s))
		++s;
	if (*s++ != '.')
		return 0; /* not floating point */
	while (numeric(*s))
		++s;
	lptr = (s--) - line; /* save ending point */
	sum = 0.0; /* initialise result */
	while (*s != '.') { /* handle digits to right of decimal */
		sum = (sum + (*(s--) - '0')) / 10.0;
	}
	scale = 1.0; /* initialize scale factor */
	while (--s >= start) {
		/* handle remaining digits */
		sum += scale * (float)(*s - '0');
		scale *= 10.;
	}
	if (cmatch('e')) { /* interpret exponent */
		int neg, /* nonzero if exp is negative */
		expon; /* the exponent */

		if (number(&expon) == 0) {
			error("bad exponent");
			expon = 0;
		}
		if (expon < 0) {
			neg = 1;
			expon = (-expon);
		} else
			neg = 0;
		if (expon > 38) { /* e38 represents a 2^127 exponent , ie 7 bits plus sign */
			error("overflow");
			expon = 0;
		}
		k = 32; /* set a bit in the mask */
		scale = 1.;
		/* find 10**expon by repeated squaring */
		while (k) {
			scale *= scale;
			if (k & expon)
				scale *= 10.;
			k >>= 1;
		}
		if (neg)
			sum /= scale;
		else
			sum *= scale;
	}
	/*if (minus < 0);
		sum = (-sum);*/
	if (litptr + 6 >= LITMAX) { /* Number of bytes per FP representation */
		error("string space exhausted");
		return 0;
	}
	/* get location for result & bump litptr */
	*val = litptr;
	dp = litq + litptr;
	litptr += 6; /* Number of bytes per FP representation */
	/* *dp = sum; /* store result */

	asreal_d(fpnum) = sum;
	/*
	 * Now IEE exponent is e-127 = exp2 where 2^exp is the radix 2 exponent
	 * As M48 exponent is L+128 (as FP = 0 means L = 0) then we can convert to
	 * M48 using L=e-127 + 128 + 1 = e + 2;   The + 1 is do to L = 0 for fp of zero
	 */


	signbit = ((fpnum >> 56) & 0x80);
	dp[0] = ((fpnum >> 45) & 0x7f) | signbit;
	dp[1] = (fpnum >> 37) & 0xff;
	dp[2] = (fpnum >> 29) & 0xff;
	dp[3] = (fpnum >> 21) & 0xff;
	dp[4] = (fpnum >> 13) & 0xff;
	dp[5] = ((fpnum >> 52) & 0x7f)|((fpnum >> 55) & 0x80);
	if (dp[5] != 0) dp[5] += 2; /* fix up the exponent */

/*
	printf("\n%02hhx",dp[0]);
	printf("%02hhx",dp[1]);
	printf("%02hhx",dp[2]);
	printf("%02hhx",dp[3]);
	printf("%02hhx",dp[4]);
	printf("%02hhx",dp[5]);
*/
	return 1; /* report success */
}

#else

fnumber(val)
int *val ;
{
	return 0 ;
}

#endif

number(val)
	int *val; {
	char c;
	int minus, k;

	k = minus = 1;
	while (k) {
		k = 0;
		if (cmatch('+'))
			k = 1;
		if (cmatch('-')) {
			minus = (-minus);
			k = 1;
		}
	}
	if (ch() == '0' && raise(nch()) == 'X') {
		gch();
		gch();
		if (hex(ch()) == 0)
			return 0;
		while (hex(ch())) {
			c = inbyte();
			if (c <= '9')
				k = (k << 4) + (c - '0');
			else
				k = (k << 4) + ((c & 95) - '7');
		}
		*val = k;
		return 1;
	}
	if (numeric(ch()) == 0)
		return 0;
	while (numeric(ch())) {
		c = inbyte();
		k = k * 10 + (c - '0');
	}
	if (minus < 0)
		k = (-k);
	*val = k;
	return 1;
}

hex(c)
	char c; {
	char c1;

	c1 = raise(c);
	return ((c1 >= '0' && c1 <= '9') || (c1 >= 'A' && c1 <= 'F'));
}

address(ptr)
	char *ptr; {
	ol(";address(ptr)");
	immed();
	outname(ptr);
	nl();
}

pstr(val)
	int *val; {
	int k;

	if (cmatch('\'')) {
		k = 0;
		while (ch() != 39)
			k = (k & 255) * 256 + litchar();
		++lptr;
		*val = k;
		return 1;
	}
	return 0;
}

qstr(val)
	int *val; {
	if (cmatch('"') == 0)
		return 0;
	*val = litptr;
	while (ch() != '"') {
		if (ch() == 0)
			break;
		stowlit(litchar(), 1);
	}
	gch();
	litq[litptr++] = 0;
	return 1;
}

/* store integer i of size size bytes in literal queue */
stowlit(value, size)
	int value, size; {
	if ((litptr + size) >= LITMAX) {
		error("literal queue overflow");
		ccabort();
	}
	putint(value, litq + litptr, size);
	litptr += size;
}

/* Return current literal char & bump lptr */
litchar() {
	int i, oct;

	if (ch() != 92)
		return gch();
	if (nch() == 0)
		return gch();
	gch();
	switch (ch()) {
	case 'b': {
		++lptr;
		return 8;
	} /* BS */
	case 't': {
		++lptr;
		return 9;
	} /* HT */
	case 'l': {
		++lptr;
		return 10;
	} /* LF */
	case 'f': {
		++lptr;
		return 12;
	} /* FF */
	case 'n': {
		++lptr;
		return 13;
	} /* CR */
	case 'r': {
		++lptr;
		return 10;
	} /* LF */
	}
	i = 3;
	oct = 0;
	while (i-- > 0 && ch() >= '0' && ch() <= '7')
		oct = (oct << 3) + gch() - '0';
	if (i == 2)
		return gch();
	else
		return oct;
}

/*
 * find size of type (not of variable)
 */
size_of(lval)
	LVALUE *lval; {
	char sname[NAMESIZE];
	TAG_SYMBOL *otag;

	needchar('(');
	if (amatch("struct") || amatch("union")) {
		if (symname(sname) == 0)
			illname();
		else if (otag = findtag(sname))
			lval->const_val = otag->size;
		else
			error("unknown struct");
	} else if (amatch("int"))
		lval->const_val = 2;
	else if (amatch("char"))
		lval->const_val = 2; /* [AC] needs to be 2 for 9900 */
	else if (amatch("double"))
		lval->const_val = 6;
	needchar(')');
	lval->is_const = 1;
	lval->val_type = CINT;
	const1(lval->const_val);
}

