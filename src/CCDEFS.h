/*	Define system dependent parameters	*/

/*	Stand-alone definitions			*/

#define NO		0
#define YES		1

/*	System wide name size (for symbols)	*/

#define SMALL_C /* - note this is predefined in the setup_sym()  function */

#ifdef SMALL_C
	#define NULL_FD 0
	#define NULL_FN 0
	#define NULL_CHAR 0
	#define alloc malloc
#else
	#define NULL_FD (FILE *)0
	typedef int (*FUNC)() ;
	#define NULL_FN (FUNC)0
	#define NULL_CHAR (char *)0
	#define alloc malloc
#endif


#define	NAMESIZE 12
#define NAMEMAX  11

/*	Define the symbol table parameters	*/

#define NUMGLBS		512
#define MASKGLBS	511
#define	STARTGLB	symtab
#define	ENDGLB		(STARTGLB+NUMGLBS)

#define NUMLOC		55
#define	STARTLOC	loctab
#define	ENDLOC		(STARTLOC+NUMLOC)

/*	Define symbol table entry format	*/

#define SYMBOL struct symb

SYMBOL {
	char name[NAMESIZE] ;
	char ident ;		/* VARIABLE, ARRAY, POINTER, FUNCTION, MACRO, LABEL		*/
	char type ;			/* DOUBLE, CINT, CCHAR, STRUCT							*/
	char class;			/* EXT, ENT , GLOBAL*/
	char modifier ;		/* UNSIGNED												*/
	char storage ;		/* STATIK, STKLOC, EXTERNAL */
	union xx  {			/* offset has a number of interpretations:				*/
		int i ;			/* local symbol:  offset into stack, or label			*/
						/* struct member: offset into struct					*/
						/* global symbol: EXTERNAL if symbol not yet defined	*/
						/*                STATIK if symbol has been defined		*/
						/*                macro table offset (ident = MACRO)	*/
						/*                else 0								*/ 
		SYMBOL *p ;		/* also used to form linked list of fn args				*/
	} offset ;
	char more ;			/* index of linked entry in dummy_sym */
	char tag_idx ;		/* index of struct tag in tag table */
} ;

#ifdef SMALL_C
#define NULL_SYM 0
#else
#define NULL_SYM (SYMBOL *)0
#endif

/*	Define possible entries for "ident"	*/

#define LABEL		0
#define	VARIABLE	1
#define	ARRAY		2
#define	POINTER		3
#define	FUNCTION	4
#define MACRO		5
/* the following only used in processing, not in symbol table */
#define PTR_TO_FN	6
#define PTR_TO_PTR	7

/*	Define possible entries for "type"	*/

/*      LABEL	0 */
#define DOUBLE	1
#define	CINT	2
#define	CCHAR	3
#define STRUCT	4
#define UNION	5		/* used only in processing, not in symbol table */
#define UCCHAR	6		/* Unsigned CHAR */
#define UCINT	7		/* Unsigned INT */


/*	Define possible modifier for "type"	*/
#define UNSGND	1

/* number of types to which pointers to pointers can be defined */
#define NTYPE	3

/*	Define possible entries for "storage"	*/

#define	STATIK		1
#define	STKLOC		2
#define EXTERNAL	3

/* SYMBOL Class */
#define	EXT	1
#define	ENT	2
#define GLOBAL 3

/*	Define the structure tag table parameters */

#define NUMTAG		10
#define STARTTAG	tagtab
#define ENDTAG		tagtab+NUMTAG

struct tag_symbol {
	char name[NAMESIZE] ;		/* structure tag name */
	int size ;					/* size of struct in bytes */
	SYMBOL *ptr ;				/* pointer to first member */
	SYMBOL *end ;				/* pointer to beyond end of members */
} ;

#define TAG_SYMBOL struct tag_symbol

#define OPTIMIZE 1		/* turns on peephole optimisation */
#ifdef SMALL_C
#define NULL_TAG 0
#else
#define NULL_TAG (TAG_SYMBOL *)0
#endif

/*	Define the structure member table parameters */

#define NUMMEMB		45
#define STARTMEMB	membtab
#define ENDMEMB		(membtab+NUMMEMB)

/* switch table */

#define NUMCASE	80

struct sw_tab {
	int label ;		/* label for start of case */
	int value ;		/* value associated with case */
} ;

#define SW_TAB struct sw_tab

/*	Define the "while" statement queue	*/

#define	NUMWHILE	15
#define	WQMAX		wqueue+(NUMWHILE-1)

struct while_tab {
	int sp ;		/* stack pointer */
	int loop ;		/* label for top of loop */
	int exit ;		/* label at end of loop */
} ;

#define WHILE_TAB struct while_tab

/*	Define the literal pool			*/

#define	LITABSZ 950
#define	LITMAX	LITABSZ-1

/*	Define the input line			*/

#define	LINESIZE	256
#define	LINEMAX		(LINESIZE-1)
#define	MPMAX		LINEMAX

/*  Output staging buffer size */

#define STAGESIZE	3072
#define STAGELIMIT	(STAGESIZE-1)

/*	Define the macro (define) pool		*/

#define	MACQSIZE	500
#define	MACMAX		MACQSIZE-1

/*	Define statement types (tokens)		*/

#define	STIF		1
#define	STWHILE		2
#define	STRETURN	3
#define	STBREAK		4
#define	STCONT		5
#define	STASM		6
#define	STEXP		7
#define STDO		8
#define STFOR		9
#define STSWITCH	10
#define STCASE		11
#define STDEF		12
#define STGOTO		13
#define STLABEL		14

/* define length of names for assembler */

#define ASMLEN	12
#ifdef SMALL_C
#define SYM_CAST
#define TAG_CAST
#define WQ_CAST
#define SW_CAST
#else
#define SYM_CAST (SYMBOL *)
#define TAG_CAST (TAG_SYMBOL *)
#define WQ_CAST (WHILE_TAB *)
#define SW_CAST (SW_TAB *)
#endif
