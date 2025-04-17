/*
 * itod -- convert nbr to signed decimal string of width sz
 *	       right adjusted, blank filled ; returns str
 *
 *	      if sz > 0 terminate with null byte
 *	      if sz  =  0 find end of string
 *	      if sz < 0 use last byte for data
 */
#include "stdio.h"
/*
 * itod -- convert nbr to signed decimal string of width sz
 *	       right adjusted, blank filled ; returns str
 *
 *	      if sz > 0 terminate with null byte
 *	      if sz  =  0 find end of string
 *	      if sz < 0 use last byte for data
 */
itod(nbr, str, sz)
	int nbr; char str[]; int sz; {
	char sgn;
	int i;
	int minval;

	minval = 0x8000;  /* -32768 in 16-bit two's complement */

	if (nbr == minval) {
		/* Special case: avoid negating -32768 */
		str[0] = '-';
		str[1] = '3';
		str[2] = '2';
		str[3] = '7';
		str[4] = '6';
		str[5] = '8';
		str[6] = '\0';
		/* Adjust spacing and width if needed */
		if (sz > 0) {
			i = 7;
			while (i < sz)
				str[i++] = ' ';
			str[sz] = '\0';
		}
		return str;
	}

	if (nbr < 0) {
		sgn = '-';
		nbr = -nbr;
	} else {
		sgn = ' ';
	}
	if (sz > 0) {
		str[--sz] = '\0';
	} else if (sz < 0) {
		sz = -sz;
	} else {
		while (str[sz] != '\0') {
			++sz;
		}
	}
	i = sz;
	if (nbr == 0) {
		if (i > 0) {
			str[--i] = '0';
		}
	} else {
		while (nbr && i > 0) {
			str[--i] = (nbr % 10) + '0';
			nbr /= 10;
		}
	}
	if (i > 0 && sgn == '-') {
		str[--i] = sgn;
	}
	while (i > 0) {
		str[--i] = ' ';
	}
	return str;
}
