/*
 *	printf - core routines for printf, sprintf, fprintf
 *	        this is the floating point version
 *
 *	Compile with -m option
 *
 *	R M Yorston 1987
 */

#include "stdio.h"
#include "float.h"

int _printf(), _outc(), ftoa(), ftoe();

/* int _printf(), ftoa(),ftoe(), _outc(); */
/*
 * printf(controlstring, arg, arg, ...)  or
 * sprintf(string, controlstring, arg, arg, ...) or
 * fprintf(file, controlstring, arg, arg, ...) -- formatted print
 *        operates as described by Kernighan & Ritchie
 *        only f,d, x, c, s, and u specs are supported.
 */

char *_String;
int _Count;

printf(args)
	int args; {
	_String = NULL;
	return (_printf(stdout, _argcnt() + &args-1));

}

fprintf(args)
	int args; {
	int *nxtarg;

	_String = NULL;
	nxtarg = _argcnt() + &args;
	return (_printf(*(--nxtarg), --nxtarg));
}

sprintf(args)
	int args; {
	int *nxtarg;

	nxtarg = _argcnt() + &args - 1;
	_String = *nxtarg;
	return (_printf( stdin, --nxtarg));
}
_printf(fd, nxtarg)
int fd ;
int *nxtarg ;
{

	double *pd ;
	int i, width, prec, preclen, len ;
	char c, right, str[64], pad;
	char *sptr, *ctl, *cx ;

	_Count = 0 ;
	ctl = *nxtarg ;

	while ( c = *ctl++ ) {


		if (c != '%' ) {
			_outc(c, fd) ;
			continue ;
		}
		if ( *ctl == '%' ) {
			_outc(*ctl++, fd) ;
			continue ;
		}
		cx = ctl ;
		if ( *cx == '-' ) {
			right = 0 ;
			++cx ;
		}
		else
			right = 1 ;
		if ( *cx == '0' ) {
			pad = '0' ;
			++cx ;
		}
		else
			pad = ' ' ;

		if ( (i=utoi(cx, &width)) >= 0 )
			cx += i ;
		else
			continue  ;

		/* preclen is precision length */

		if (*cx == '.') {
			if ( (preclen=utoi(++cx, &prec)) >= 0 )
				cx += preclen ;
			else
				continue ;
		}
		else
			preclen = 0 ;

		sptr = str ;
		c = *cx++ ;
		i = *(--nxtarg) ;
		switch(c) {
		//putchar(c); /* debug */
			case 'd' :
			case 'i' :
				itod(i, str, 7) ;
				break ;
			case 'x' :
				itox(i, str, 7) ;
				break ;
			case 'c' :
				str[0] = i;
				str[1] = NULL ;
				break ;
			case 's' :
				sptr = i ;
				break ;
			case 'u' :
				itou(i, str, 7) ;
				break ;
			default:
				if ( preclen == 0 )
					prec = 6 ;
				if ( c == 'f' ) {
					nxtarg -= 2 ;
					pd = nxtarg ;
					ftoa( *pd, prec, str ) ;

				}
				else if ( c == 'e' ) {
					nxtarg -= 2 ;
					pd = nxtarg ;
					ftoe( *pd, prec, str ) ;
				}
				else
					continue ;
		}

		ctl = cx ; /* accept conversion spec */

		if ( c != 's' && c != 'c' )
			while ( *sptr == ' ' )
				++sptr ;

		len = -1 ;
		while ( sptr[++len] )
			; /* get length */
		if ( c == 's' && len>prec && preclen>0 )
			len = prec ;

		if (right)
			while ( ((width--)-len) > 0 )
				_outc(pad, fd) ;

		while ( len ) {
			_outc(*sptr++, fd) ;
			--len ;
			--width ;
		}

		while ( ((width--)-len) > 0 )
			_outc(pad, fd) ;
	}
	if (_String != 0) *_String = '\000' ;

	return(_Count) ;
}


/* convert double number to string (f format) */

ftoa(x,f,str)
double x;	/* the number to be converted */
int f;		/* number of digits to follow decimal point */
char *str;	/* output string */
{
	double scale;		/* scale factor */
	int i,				/* copy of f, and # digits before decimal point */
		d;				/* a digit */

	if( x < 0.0 ) {
		*str++ = '-' ;
		x = -x ;
	}
	i = f ;
	scale = 2.0 ;
	while ( i-- )
		scale *= 10.0 ;
	x += 1.0 / scale ;
	/* count places before decimal & scale the number */
	i = 0 ;
	scale = 1.0 ;
	while ( x >= scale ) {
		scale *= 10.0 ;
		i++ ;
	}
	while ( i-- ) {
		/* output digits before decimal */
		scale = floor(0.5 + scale * 0.1 ) ;
		d = ifix( x / scale ) ;
		*str++ = d + '0' ;
		x -= float(d) * scale ;
	}
	if ( f <= 0 ) {
		*str = NULL ;
		return ;
	}
	*str++ = '.' ;
	while ( f-- ) {
		/* output digits after decimal */
		x *= 10.0 ;
		d = ifix(x) ;
		*str++ = d + '0' ;
		x -= float(d) ;
	}
	*str = NULL ;
}

/*	e format conversion			*/

ftoe(x,prec,str)
double x ;		/* number to be converted */
int prec ;		/* # digits after decimal place */
char *str ;		/* output string */
{
	double scale;	/* scale factor */
	int i,			/* counter */
		d,			/* a digit */
		expon;		/* exponent */

	scale = 1.0 ;		/* scale = 10 ** prec */
	i = prec ;
	while ( i-- )
	scale *= 10.0 ;
	if ( x == 0.0 ) {
		expon = 0 ;
		scale *= 10.0 ;
	}
	else {
		expon = prec ;
		if ( x < 0.0 ) {
			*str++ = '-' ;
			x = -x ;
		}
		if ( x > scale ) {
			/* need: scale<x<scale*10 */
			scale *= 10.0 ;
			while ( x >= scale ) {
				x /= 10.0 ;
				++expon ;
			}
		}
		else {
			while ( x < scale ) {
				x *= 10.0 ;
				--expon ;
			}
			scale *= 10.0 ;
		}
		/* at this point, .1*scale <= x < scale */
		x += 0.5 ;			/* round */
		if ( x >= scale ) {
			x /= 10.0 ;
			++expon ;
		}
	}
	i = 0 ;
	while ( i <= prec ) {
		scale = floor( 0.5 + scale * 0.1 ) ;
		/* now, scale <= x < 10*scale */
		d = ifix( x / scale ) ;
		*str++ = d + '0' ;
		x -= float(d) * scale ;
		if ( i++ ) continue ;
		*str++ = '.' ;
	}
	*str++ = 'e' ;
	if ( expon < 0 ) { *str++ = '-' ; expon = -expon ; }
	if(expon>9) *str++ = '0' + expon/10 ;
	*str++ = '0' + expon % 10 ;
	*str = NULL;
}

/*
 * _outc - output a single character
 *         if _String is not null send output to a string instead
 */
_outc(c, fd)
	char c;int fd; {
	if (_String == NULL)
		putc(c, fd);
	else
		*_String++ = c;
	++_Count;
}

