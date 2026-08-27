/*
 * cc6.c - sixth part of Small-C/Plus compiler
 *         code generator
 */

#include "stdio.h"
#include "string.h"
#include "ccdefs.h"
#include "cclvalue.h"

extern char Banner[], Author[], Version[], Filename[];
extern int litlab, Zsp, mainflg;
extern SYMBOL *symtab;

/* Begin a comment line for the assembler */
comment() {
	outbyte(';');
}

/* Put out assembler info before any code is generated */
/* for TMS 9900/99000/99105 */
/*  HL == R4
 *  DE == R3
 *  BC == R2
 *
 */
header() {
	comment();
	outstr(Banner);
	nl();
	comment();
	outstr(Author);
	nl();
	comment();
	outstr(Version);
	nl();
	comment();
	nl();
	ol("R0\tEQU 0");
	ol("R1\tEQU 1");
	ol("R2\tEQU 2");
	ol("R3\tEQU 3");
	ol("R4\tEQU 4");
	ol("R8\tEQU 8");
	ol("SL\tEQU 9");
	ol("R9\tEQU 9");
	ol("SP\tEQU 10");
	ol("WP\tEQU 13");
	ol("R11\tEQU 11"); /* R11 is used by BL as the return address and needs to be saved */

	ol("\tDXOP CALL,6"); /*  These are defined in the MONITOR  */
	ol("\tDXOP RET,7");
	ol("\tDXOP WHEX,10");
	ol("\tDXOP WRITE,12    ;WRITE CHAR IN MSB ");
	ol("\tDXOP	MESG,14");
	ol(
			"\tDXOP DEBUG,15    ;TRACE THE PRECEDING INSTRUCTION, PC,ST and REGISTERS ");
	nl();
	ol(";-------- START MODULE -----------");
	/*	if ( trace ) {
	 /*	ol("global ccregis"); /* declare these */
	/*	ol("global ccleavi"); /* tracing routine */
	/*	} */

	if (mainflg) { /* do stuff needed for first */
		/* set default code */

		/* FREEMEM and MEMLIMIT are setup by the SHELL */
		/* WP used to in some offset instructions (i.e. floating point  */
		ol("\tSTWP WP ");
		/* Branch is used because IOLIB will vector back to "main" */
		ol("\tB @_main## ; This calls iolib99 entry point");

	} else {
		/* not main program, output module name */
		/*	ot("module NAME"); */
		ot("\tNAM ");
		if (Filename[1] == ':')
			outstr(Filename + 2);
		else
			outstr(Filename);
		nl();
/* DEPRECATED
		ol(	"\tNOP; Required to terminate link chain with non zero for global variables");
		ot("\tNOP ");
*/
	}
}

/* Print any assembler stuff needed after all code */
trailer() {
	/* Output EXT and ENT Declarations  */
	SYMBOL *ptr;
	ptr = STARTGLB;
	while (ptr < ENDGLB) {
		if (ptr->class == EXT) /*37*/
			external(ptr->name);
		if (ptr->class == ENT) /*37*/
			entry(ptr->name); /*37*/
		++ptr; /*37*/
	}

	ol("\tEVEN"); /*Just in case the pc is still odd.  This can effect loading of other modules */
	ol("\tEND");
	nl();
	comment();
	outstr(" --- End of Compilation ---\n");
}

/*
 * Print out a name such that it won't annoy the assembler
 *	(by matching anything reserved, like opcodes.)
 */
outname(sname)
	char *sname; {
	int i;

	if ((sname[1] == '\0')) {
		outbyte('q'); /* helps make variables unique that are short in length */
		outbyte('_');
	}
	if (strlen(sname) > ASMLEN) {
		i = ASMLEN;
		while (i--)
			/*outbyte(raise(*sname++));*/
			outbyte(*sname++);
	} else
		outstr(sname);
}

/* Fetch a static memory cell into the primary register */
getmem(sym)
	SYMBOL *sym; {
	ol(";getmem(sym)");
	if (sym->ident != POINTER && sym->type == CCHAR) {
		ot("\tMOVB @");
		outname(sym->name);
		ol(",R4");
		ol("\tSRA R4,8 ;ccsxt - sign extend");
	} else if (sym->ident != POINTER && sym->type == DOUBLE) {
		address(sym->name);
		fpcall("_fload##");
	} else {
		ot("\tMOV @");
		outname(sym->name);
		ol(",R4");
	}
}

/*
 ** output EVEN to align boundaries for TMS9900
 */
even() {
	ol("\tEVEN");
}

/* Fetch the address of the specified symbol */
/*	into the primary register */
getloc(sym, off)
	SYMBOL *sym;int off; {
	int offset;
	ol(";getloc(sym, off)");
	offset = sym->offset.i - Zsp + off;

	if (sym->type == CCHAR && sym->ident != POINTER) {
		ol(";Char offset");
		offset += 1; /* [AC-9900] Need to offset as CCHAR takes two bytes on the stack */
	}

	if (offset) {
		ot("\tLI R4,");
		outdec(offset);
		nl();
		ol("\tA SP,R4");
	} else
		ol("\tMOV SP,R4");

}

/* Store the primary register into the specified */
/*	static memory cell */
putmem(sym)
	SYMBOL *sym; {
	ol(";putmem(sym)");
	if (sym->ident != POINTER && sym->type == DOUBLE) {
		address(sym->name);
		fpcall("_fstore##");
	} else {
		if (sym->ident != POINTER && sym->type == CCHAR) {
			ot("\tMOVB @2*R4+1(WP),@"); /* CHARs are stored as LSB of a 16 bit word */
			outname(sym->name);
			nl();
		} else {
			ot("\tMOV R4,@");
			outname(sym->name);
			nl();
		}
	}
}

/* Store the specified object type in the primary register
 *	at the address on the top of the stack  - because it is on the stack
 *  the char must be stored as a word  i.e. in the LSB.
 *
 *  WP is loaded at the start of the programme to point to the current workspace
 *
 */

putstk(typeobj)
	char typeobj; {
	switch (typeobj) {
	case DOUBLE:
		mainpop();
		fpcall("_fstore##");
		break;
	case CCHAR:
		zpop();
		ol(";putstk - char");
		ol("\tMOVB @2*R4+1(WP),*R3"); /* Offset by 1 as CHARs are LSB of 16 bit word */
		break;
	default:
		zpop();
		ol(";putstk - int");
		ol("\tMOV R4,*R3");
	}
}

/* store a two byte object in the primary register at TOS */
puttos() {
	ol(";puttos()");
	/*	ol("\tMOV *SP,R2"); /* Probably redundant and not needed */
	ol("\tMOV R4,*SP");
}

/* store a two byte object in the primary register at 2nd TOS */
put2tos() {
	ol(";put2tos()");
	/*	ol("\tMOV *SP,R3");  /* Probably redundant and not needed */
	/*	ol("\tMOV @2(SP),R2"); /* Probably redundant and not needed */
	ol("\tMOV R4,@2(SP)");
}

/*
 * loadargc - load accumulator with number of args
 *            no special treatment of n==0, as this
 *            should never arise for printf etc.
 */
loadargc(n)
	int n; {
	ol(";loadarg(n)");
	ot("\tLI R1,");
	outdec(n >> 1);
	nl();
	callrts("_setargc##"); /* set the argcnt in iolib99 so it can be reused */

}

/* Fetch the specified object type indirect through the
 *	primary register into the primary register
 *
 * TMS99105 Structure for stack and memory
 * 16bit word values
 * R4 = [MSB|LSB] for 16bit values
 *
 * For Byte values CHAR types are sign extended and
 * stored in the LSB of stack. Stack is always even memory that is
 *
 *  SP ->  [0x00 | CHAR ]
 *
 *
 */

indirect(typeobj)
	char typeobj; {
	switch (typeobj) {
	case CCHAR: /* Fetch a single character from the address in R4 and extend it to lower byte*/
		ol(";indirect ccgchar");
		ol("\tMOVB *R4,R4");
		ol("\tSRA R4,8");
		break;
	case DOUBLE:
		ol(";indirect float");
		fpcall("_fload##");
		break;
	default:
		ol(";indirect ccgint");
		ol("\tMOV *R4,R4");
	}
}

/* Swap the primary and secondary registers */
swap() {
	ol(";swap()");
	ol("\tMOV R3,R0;;"); /* peephole() uses trailing ";;" */
	ol("\tMOV R4,R3");
	ol("\tMOV R0,R4");
}

/* Print partial instruction to get an immediate value */
/*	into the primary register */
immed0() {
	ot("\tCLR R4");
}

/* Print full instruction as 0xFFFF or -1 */
/*	into the primary register */
immed1() {
	ot("\tSETO R4");
}

immed() {
	ot("\tLI R4,");
}

/* Print partial instruction to get an immediate value */
/*	into the secondary register */
immed2() {
	ot("\tLI R3,");

}

/* Print partial instruction to get an immediate value */
/*	into the scratch register R0*/
immed3() {
	ot("\tLI R0,");

}

/* Partial instruction to access literal */
immedlit() {
	immed();
	printlabel(litlab);
	outbyte('+');
}

/* Push the primary register onto the stack */
zpush() {
	oc(";zpush()", OPTIMIZE); /*Remove comment if we are optimising code */
	ol("\tDECT SP");
	ol("\tMOV R4,*SP");
	Zsp -= 2;
}
/* This is called from callfunction() - Was need to apply special code to CCHARs,
 * but not needed now and can be removed at some stage as it is equivalent to zpush()
 */
zpushC() {
	oc(";zpushC()", OPTIMIZE); /*Remove comment if we are optimising code */
	ol("\tDECT SP");
	ol("\tMOV R4,*SP");
	Zsp -= 2;
}

/* Push the primary register onto the stack */
cpush() {
	oc(";cpush()", OPTIMIZE); /*Remove comment if we are optimising code */
	ol("\tDECT SP");
	ol("\tMOV R4,*SP");
	Zsp -= 2;
}

/* Push the primary floating point register onto the stack */
fpush() {
	ol(";fpush()");
	fpcall("_fpush##");
	Zsp -= 6;
}

/* Push the primary floating point register, preserving
 the top value  */
/* TODO AC 9900  Need to just push R4 */
fpush2() {
	ol(";fpush2()");
	fpcall("_fpush2##");
	Zsp -= 6;
}

/* Pop the top of the stack into the primary register */
mainpop() {
	ol(";mainpop()");
	ol("\tMOV *SP+,R4");
	Zsp += 2;
}

/* Pop the top of the stack into the secondary register */
zpop() {
	oc(";zpop()", OPTIMIZE); /*Remove comment if we are optimising code */
	ol("\tMOV *SP+,R3");
	Zsp += 2;
}

/* Swap the primary register and the top of the stack */
swapstk() {
	ol(";swapstk()");
	ol("\tMOV R4,R0");
	ol("\tMOV *SP,R4");
	ol("\tMOV R0,*SP");
}

/* process switch statement - return address points to switch table */
sw() {
	tspgraph_call_raw("CALL", "_ccswitc##");
	ol("\tCALL @_ccswitc##"); /* Must use call for switch for return address*/
}

/* Call the specified subroutine name */
zcall(sym)
	char *sym; {
	tspgraph_call_sym("CALL", sym);
	ol(";zcall()");
	ot("\tCALL @");
	outname(sym);
	nl();
}

/* Call a run-time library routine - Because they don't call any other
 * routines we can use BL and return using B *R11
 */
callrts(sname)
	char *sname; {
	tspgraph_call_raw("BL", sname);
	ot("\tBL @");
	outstr(sname);
	nl();
}

/* Call a floating point library routine */
fpcall(sname)
	char *sname; {
	tspgraph_call_raw("CALL", sname);
	ol(";fpcall()");
	ot("\tCALL @");
	outstr(sname);
	nl();
}

/* Return from subroutine */
zret() {
	ol("\tRET");
}

/*
 * Perform subroutine call to value on top of stack
 * Put arg count in A (R1) in case subroutine needs it
 */
callstk(n)
	int n; {
	tspgraph_icall();
	ol(";callstk()");
	loadargc(n);
	ol("\tCALL *R4");

}

/* Jump to specified internal label number */
jump(label)
	int label; {
	ol(";jump(label)");
	ot("\tB @");
	printlabel(label);
	nl();
}

/* Test the primary register and jump if false to label */
testjump(label)
	int label; {
	ol("\tMOV R4,R4");
	ol("\tJNE $+6");
	ot("\tB @");
	printlabel(label);
	nl();
}

/* Test the primary register and jump if false to label */
testjump2(label)
	int label; {
	ol(";testjump()");
	ot("\tB @");
	printlabel(label);
	nl();
}

/* test primary register against zero and jump if false */
zerojump(oper, label, lval)
	int (*oper)(), label;
	LVALUE *lval; {
	clearstage(lval->stage_add, 0); /* purge conventional code */
	(*oper)(label);
}

/* Print pseudo-op to define a byte */
defbyte() {
	ot("\tBYTE ");
}

/*Print pseudo-op to define storage */
defstorage() {
	ot("\tBSS ");
}

/* Print pseudo-op to define a word */
defword() {
	ot("\tWORD ");
}

/* Point to following object */
point() {
	ol("\tWORD $+2");
}

/* Modify the stack pointer to the new value indicated */
modstk(newsp, save)
	int newsp;int save; /* if true preserve contents of HL */
{
	int k;
	ol(";modstk(newsp,save)");
	k = newsp - Zsp;
	if (k == 0)
		return newsp;

	if (k & 1)
		k++; /* TMS 9900 Can't be odd if on stack */

	switch (k) {
	case 2:
		ol("\tINCT SP");
		return newsp;
	case -2:
		ol("\tDECT SP");
		return newsp;
	default:
		ot("\tAI SP,");
		outdec(k);
		nl();
		return newsp;
	}
	if (save)
		swap();
	return newsp;
}

/* Multiply the primary register by the length of some variable */
scale(type, tag)
	int type;
	TAG_SYMBOL *tag; {
	ol(";scale(type,tag)");
	switch (type) {
	case CINT:
		doublereg();
		break;
	case DOUBLE:
		sixreg();
		break;
	case STRUCT:
		/* try to avoid multiplying if possible */
		switch (tag->size) {
		case 16:
			doublereg();
		case 8:
			doublereg();
		case 4:
			doublereg();
		case 2:
			doublereg();
			break;
		case 12:
			doublereg();
		case 6:
			sixreg();
			break;
		case 9:
			threereg();
		case 3:
			threereg();
			break;
		case 15:
			threereg();
		case 5:
			fivereg();
			break;
		case 10:
			fivereg();
			doublereg();
			break;
		case 14:
			doublereg();
		case 7:
			sixreg();
			addbc(); /* BC contains original value */
			break;
		default:
			ol("\tDECT SP");
			ol("\tMOV R3,*SP");
			const2(tag->size);
			mult();
			ol("\tMOV *SP+,R3");
			break;
		}
	}
}

/* add BC to the primary register */
addbc() {
	ol(";\taddbc()");
	ol("\tA R2,R4");
}

/* load BC from the primary register */
ldbc() {
	ol(";\tldbc()");
	ol("\tMOV R4,R2");
}

/* Double the primary register */
doublereg() {
	ol(";\tdoublereg()");
	ol("\tA R4,R4");
}

/* Multiply the primary register by three */
threereg() {
	ldbc();
	addbc();
	addbc();
}

/* Multiply the primary register by five */
fivereg() {
	ldbc();
	doublereg();
	doublereg();
	addbc();
}

/* Multiply the primary register by six */
sixreg() {
	threereg();
	doublereg();
}
/* Multiply the primary register by six */
fourreg() {
	doublereg();
	doublereg();
}
/* Add the primary and secondary registers (result in primary) */
zadd() {
	ol(";\tzadd()");
	ol("\tA R3,R4");
}

/* Add the primary floating point register to the
 value on the stack (under the return address)
 (result in primary) */
fadd() {
	fpcall("_fpadd##");
	Zsp += 6;
}

/* Subtract the primary register from the secondary */
/*	(results in primary) */
zsub() {
	ol(";zsub()");
	ol("\tS R4,R3");
	ol("\tMOV R3,R4");
}

/* Subtract the primary floating point register from the
 value on the stack (under the return address)
 (result in primary) */
fsub() {
	fpcall("_fpsub##");
	Zsp += 6;
}

/* Multiply the primary and secondary registers */
/*	(results in primary */
mult() {
	callrts("_ccmult##");
}

/* Multiply the primary floating point register by the value
 on the stack (under the return address)
 (result in primary) */
fmul() {
	fpcall("_fpmul##");
	Zsp += 6;
}

/* Divide the secondary register by the primary */
/*	(quotient in primary, remainder in secondary) */
div() {
	callrts("_ccdiv##");
}

/* Divide the value on the stack (under the return address)
 by the primary floating point register (quotient in primary) */
fdiv() {
	fpcall("_fpdiv##");
	Zsp += 6;
}

/* Compute remainder (mod) of secondary register divided
 *	by the primary
 *	(remainder in primary, quotient in secondary)
 */
zmod() {
	div();
	swap();
}

/* Inclusive 'or' the primary and secondary */
/*	(results in primary) */
zor() {
	ol(";zor()");
	ol("\tSOC R3,R4");
}

/* Exclusive 'or' the primary and secondary */
/*	(results in primary) */
zxor() {
	ol(";zxor()");
	ol("\tXOR R3,R4");
}

/* 'And' the primary and secondary */
/*	(results in primary) */
zand() {
	oc(";zand()", 0);
	ol("\tINV R3");
	ol("\tSZC R3,R4");

}

/* Arithmetic shift right the secondary register number of */
/* 	times in primary (results in primary) */
asr() {
	callrts("_ccasr##");
}

/* Arithmetic left shift the secondary register number of */
/*	times in primary (results in primary) */
asl() {
	callrts("_ccasl##");
}

/* Form logical negation of primary register */
lneg() {
	callrts("_cclneg##");
}

/* Form two's complement of primary register */
neg() {
	/* callrts("_ccneg##"); */
	ol(";neg()");
	ol("\tNEG R4");
}

/* Negate the primary floating point register */
dneg() {
	fpcall("_minusfa##");
}

/* Form one's complement of primary register */
com() {
	/*callrts("_cccom##");*/
	ol(";com()");
	ol("\tINV R4");
}

/* Increment the primary register by one */
inc() {
	ol(";inc()");
	ol("\tINC R4");
}
/* Increment the primary register by one */
inct() {
	ol(";inct()");
	ol("\tINCT R4");
}

/* Decrement the primary register by one */
dec() {
	ol(";\tdec()");
	ol("\tDEC R4");
}
/* Decrement the primary register by one */
dect() {
	ol(";\tdect()");
	ol("\tDECT R4");
}

/* Following are the conditional operators */
/* They compare the secondary register against the primary */
/* and put a literal 1 in the primary if the condition is */
/* true, otherwise they clear the primary register */

/* Test for equal   TEST IF R4 = R3 */
zeq() {
	ol(";_cceq");
	callrts("_cceq##");
}

/* test for equal to zero */
eq0(label)
	int label; {
	oc(";eq0(label)", OPTIMIZE);
	ol("\tMOV R4,R4");
	ol("\tJEQ $+6");
	ot("\tB @"); /* Branch if not equal */
	printlabel(label);
	nl();
}

/* Test for not equal
 *
 * TEST IF R3 != R4
 * */
zne() {
	ol(";_ccne");
	callrts("_ccne##");
}

/* Test for less than (signed)
 *
 * TEST IF R3 < R4 (SIGNED)
 * */
zlt() {
	ol(";_cclt");
	callrts("_cclt##");
}

/* Test for less than zero */
lt0(label)
	int label; {
	ol(";lt0(label)");
	ol("\tMOV R4,R4");
	ol("\tJLT $+6");
	ot("\tB @");
	printlabel(label); /* Branch if positive */
	nl();
}

/* Test for less than or equal to (signed)
 * TEST IF R3 <= R4 (SIGNED)
 */
zle() {
	ol(";_ccle");
	callrts("_ccle##");
}

/* Test for less than or equal to zero */

le0(label)
	int label; {
	ol(";le0(label)");
	ol("\tCI R4,0");
	ol("\tJLE $+6");
	ot("\tB @");
	printlabel(label);
	nl();
}

/* Test for less than or equal to zero
 le0(label)
 int label; {
 ol(";le0(label)");
 ol("\tMOV R4,R4");
 ol("\tJEQ $+8");
 ol("\tJLT $+6");
 ot("\tB @");		/* Branch if positive
 printlabel(label);
 nl();
 }*/

/* Test for greater than (signed)
 *
 * TEST IF R3 > R4  (SIGNED)
 * */
zgt() {
	ol(";_ccgt");
	callrts("_ccgt##");
}

/* test for greater than zero */
gt0(label)
	int label; {
	ol(";gt0(label)");
	ol("\tMOV R4,R4");
	ol("\tJGT $+6"); /* Jump is A >   */
	ot("\tB @"); /* Branch if zero or less than  */
	printlabel(label);
	nl();
}

/* Test for greater than or equal to (signed)
 *
 * TEST IF R3 >= R4 (SIGNED)
 * */
zge() {
	ol(";_ccge");
	callrts("_ccge##");
}

/* test for greater than or equal to zero */
ge0(label)
	int label; {
	ol(";ge0(label)");
	ol("\tMOV  R4,R4");
	ol("\tJEQ  $+8");
	ol("\tJGT  $+6");
	ot("\tB @");
	printlabel(label); /* Jump if sign bit set */
	nl();
}

/* Test for less than (unsigned)
 * Note: these are optimised through the peephole in most cases
 * result in a single compare operation.
 *
 * TEST IF R3 < R4 (UNSIGNED)
 *
 */
ult() {
	ol(";_ccult");
	callrts("_ccult##");
}

/* Test for less than or equal to (unsigned)
 *
 * TEST IF R3 <= R4 (UNSIGNED)

 */
ule() {
	ol(";_ccule");
	callrts("_ccule##");
}

/* Test for greater than (unsigned)
 *
 * TEST IF R3 > R4 (UNSIGNED)
 *
 */
ugt() {
	ol(";_ccugt");
	callrts("_ccugt##");
}

/* Test for greater than or equal to (unsigned)
 *
 *  TEST IF R3 > R4 (UNSIGNED)
 */
uge() {
	ol(";_ccuge");
	callrts("_ccuge##");
}

/* The following conditional operations compare the
 top of the stack (TOS) against the primary floating point
 register (FA), resulting in 1 if true and 0 if false */

/* Test for floating equal */
deq() {
	fpcall("_fpeq##");
	Zsp += 6;
}

/* Test for floating not equal */
dne() {
	fpcall("fpne##");
	Zsp += 6;
}

/* Test for floating less than   (that is, TOS < FA)	*/
dlt() {
	fpcall("_fplt##");
	Zsp += 6;
}

/* Test for floating less than or equal to */
dle() {
	fpcall("_fple##");
	Zsp += 6;
}

/* Test for floating greater than */
dgt() {
	fpcall("_fpgt##");
	Zsp += 6;
}

/* Test for floating greater than or equal */
dge() {
	fpcall("_fpge##");
	Zsp += 6;
}
/*
 ** declare external reference
 */
external(name)
	char *name; {
	if (*name == NULL)
		return;
	ot("\tEXT ");
	outname(name);
	nl();

}
/*
 ** declare external reference
 */
entry(name)
	char *name; {
	if (*name == NULL)
		return;
	ot("\tENT ");
	outname(name);
	nl();
}

strlen_newline(str, start)
	char *str;int start; {
	int count;
	int i;

	count = 0;
	/* Check if starting point is within the string */
	if (str[start] == '\0') {
		return 0; /* Return 0 if start is out of bounds */
	}

	/*  Count until we find a newline or end of string */
	for (i = start; str[i] != '\0'; i++) {
		if (str[i] == '\n') {
			break;
		}
		count++;
	}

	return count;
}

/*
 * Peephole optimiser.
 *
 * Operates on the NUL-terminated staging buffer built for one statement.
 * Matching is COMMENT-TRANSPARENT: the descriptive codegen comment lines
 * (those beginning with ';') are copied straight through to the output but
 * are skipped over while an idiom is being matched, so the comments that
 * document the generated code no longer stop the optimiser from firing.
 *
 * Labels, blank lines and assembler directives (anything that starts in
 * column 0 and is not a comment) act as BARRIERS and are never crossed,
 * because they may be branch targets.  Every rule below only ever folds a
 * fixed instruction idiom into a shorter, equivalent one; nothing is
 * matched across a CALL, a branch, a label, or any stack adjustment other
 * than the push/pop pair a rule is explicitly removing.
 */

/* advance to the first character of the line following p */
char *pp_eol(p)
	char *p; {
	while (*p && *p != '\n')
		++p;
	if (*p)
		++p;
	return p;
}

/* skip a run of comment lines; return the first line that is not a comment
   (an instruction, a barrier, or the end of the buffer) */
char *pp_skipcom(p)
	char *p; {
	while (*p == ';')
		p = pp_eol(p);
	return p;
}

/* copy the single line at p (up to and including its newline) to the output
   and return a pointer to the next line */
char *pp_copyline(p, output)
	char *p; int output; {
	char *e;
	e = pp_eol(p);
	while (p < e)
		cout(*p++, output);
	return p;
}

/* p points at a "\tLI R4,<n>\n" line.  Emit  pre + <n> + post , isolating
   the numeric field with a temporary NUL so ot() can be reused. */
pp_linimm(p, pre, post)
	char *p, *pre, *post; {
	char *n, *e, save;
	n = p + 7;			/* first char after "\tLI R4," */
	e = n;
	while (*e && *e != '\n')
		++e;
	save = *e;
	*e = '\0';
	ot(pre);
	ot(n);
	ot(post);
	*e = save;
}

/* true if the line at p is a safe "middle" for push/pop elimination:
   an instruction that loads R4 without touching R3, SP or the stack.
   Restricted to LI R4,n / CLR R4 / SETO R4 / MOV @<sym>,R4 (absolute, i.e.
   no indexed "(...)" operand), so we never fold across a CALL, a branch,
   or any SP-relative addressing. */
pp_safemid(p)
	char *p; {
	char *q;
	if (streq(p, "\tLI R4,"))
		return 1;
	if (streq(p, "\tCLR R4\n"))
		return 1;
	if (streq(p, "\tSETO R4\n"))
		return 1;
	if (streq(p, "\tMOV @")) {
		q = p + 6;
		if (!alpha(*q))		/* must be a symbol, not @n(SP)/@n(WP) */
			return 0;
		while (*q && *q != '\n') {
			if (*q == '(')	/* any indexed operand -> reject */
				return 0;
			++q;
		}
		if (streq(q - 3, ",R4\n"))
			return 1;
	}
	return 0;
}

/* Comment-transparent comparison fold.  If the idiom
     BL @_ccXX## / MOV R4,R4 / (JNE|JEQ) $+6
   begins at ptr - with comment lines between the three instructions
   ignored - emit the equivalent inline compare and return a pointer to
   the first line after the idiom.  Otherwise return 0.  The input jump is
   JNE $+6 in every case except the alternate _cceq form (JEQ $+6). */
char *pp_compare(ptr, output)
	char *ptr; int output; {
	char *p1, *p2, *rep;

	if (!streq(ptr, "\tBL @_cc"))
		return 0;
	p1 = pp_skipcom(pp_eol(ptr));
	if (!streq(p1, "\tMOV R4,R4\n"))
		return 0;
	p2 = pp_skipcom(pp_eol(p1));

	rep = 0;
	if (streq(p2, "\tJNE $+6\n")) {
		if (streq(ptr, "\tBL @_ccuge##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJHE $+6\n";
		else if (streq(ptr, "\tBL @_ccult##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJL $+6\n";
		else if (streq(ptr, "\tBL @_ccugt##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJH $+6\n";
		else if (streq(ptr, "\tBL @_ccule##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJLE $+6\n";
		else if (streq(ptr, "\tBL @_cceq##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJEQ $+6\n";
		else if (streq(ptr, "\tBL @_ccne##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJNE $+6\n";
		else if (streq(ptr, "\tBL @_ccgt##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJGT $+6\n";
		else if (streq(ptr, "\tBL @_cclt##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJLT $+6\n";
		else if (streq(ptr, "\tBL @_ccge##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJGT $+8\n\tJEQ $+6\n";
		else if (streq(ptr, "\tBL @_ccle##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJLT $+8\n\tJEQ $+6\n";
	} else if (streq(p2, "\tJEQ $+6\n")) {
		if (streq(ptr, "\tBL @_cceq##\n"))
			rep = "\tC R3,R4 ;optimised\n\tJNE $+6\n";
	}
	if (rep == 0)
		return 0;
	ot(rep);
	return pp_eol(p2);
}


peephole(ptr, output)
	char *ptr; int output; {
	char *p1, *p2, *p3;
	int n;

	while (*ptr) {

		/* comment line: transparent to matching, but preserved */
		if (*ptr == ';') {
			ptr = pp_copyline(ptr, output);
			continue;
		}

		/*
		 * Load of a local:
		 *   LI R4,n / A SP,R4 / MOV  *R4,R4  ->  MOV  @n(SP),R4
		 *   LI R4,n / A SP,R4 / MOVB *R4,R4  ->  MOVB @n(SP),R4
		 * (a trailing "SRA R4,8" for a signed char simply flows on).
		 */
		if (streq(ptr, "\tLI R4,")) {
			p1 = pp_skipcom(pp_eol(ptr));
			if (streq(p1, "\tA SP,R4\n")) {
				p2 = pp_skipcom(pp_eol(p1));
				if (streq(p2, "\tMOV *R4,R4\n")) {
					pp_linimm(ptr, "\tMOV @", "(SP),R4 ;optimised load\n");
					ptr = pp_eol(p2);
					continue;
				}
				if (streq(p2, "\tMOVB *R4,R4\n")) {
					pp_linimm(ptr, "\tMOVB @", "(SP),R4 ;optimised load\n");
					ptr = pp_eol(p2);
					continue;
				}
			}
		}

		/*
		 * Load of the offset-0 local:
		 *   MOV SP,R4 / MOV  *R4,R4  ->  MOV  *SP,R4
		 *   MOV SP,R4 / MOVB *R4,R4  ->  MOVB *SP,R4
		 */
		if (streq(ptr, "\tMOV SP,R4\n")) {
			p1 = pp_skipcom(pp_eol(ptr));
			if (streq(p1, "\tMOV *R4,R4\n")) {
				ot("\tMOV *SP,R4 ;optimised load\n");
				ptr = pp_eol(p1);
				continue;
			}
			if (streq(p1, "\tMOVB *R4,R4\n")) {
				ot("\tMOVB *SP,R4 ;optimised load\n");
				ptr = pp_eol(p1);
				continue;
			}
		}

		/*
		 * Push / pop round-trip elimination:
		 *   DECT SP / MOV R4,*SP / <M> / MOV *SP+,R3   ->   MOV R4,R3 / <M>
		 * where <M> is a safe R4-only middle (see pp_safemid).  The pushed
		 * value (R4) is copied straight to R3; <M> then reloads R4.  Result
		 * R3=first operand, R4=second operand, SP unchanged - identical to
		 * the stacked version, minus three instructions.
		 */
		if (streq(ptr, "\tDECT SP\n")) {
			p1 = pp_skipcom(pp_eol(ptr));
			if (streq(p1, "\tMOV R4,*SP\n")) {
				p2 = pp_skipcom(pp_eol(p1));
				if (pp_safemid(p2)) {
					p3 = pp_skipcom(pp_eol(p2));
					if (streq(p3, "\tMOV *SP+,R3\n")) {
						ot("\tMOV R4,R3 ;optimised push/pop\n");
						pp_copyline(p2, output);
						ptr = pp_eol(p3);
						continue;
					}
				}
			}
		}

		/*
		 * Comparison folds.  The runtime helper call
		 *     BL @_ccXX## / MOV R4,R4 / Jcc $+6
		 * is replaced by an inline compare, saving a BL and four bytes.
		 * Matching is comment-transparent (pp_compare), so a comment such
		 * as ;eq0(label) sitting between the lines no longer blocks it.
		 */
		p3 = pp_compare(ptr, output);
		if (p3) {
			ptr = p3;
			continue;
		}

		/* nothing matched: copy this one line through unchanged */
		ptr = pp_copyline(ptr, output);
	}
}

cout(c, fd)
	char c;int fd; {
	if (fputc(c, fd) == -1) {
		fabort();
	}
}
/*	<<<<<  End of Small-C/Plus compiler  >>>>>	*/
