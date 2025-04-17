/*
 * itou -- convert nbr to unsigned decimal string of width sz
 *	       right adjusted, blank filled ; returns str
 *
 *	      if sz > 0 terminate with null byte
 *	      if sz  =  0 find end of string
 *	      if sz < 0 use last byte for data
 */
#include "stdio.h"
itou(nbr, str, sz)
int nbr;
char str[];
int sz;
{
	int i;

	if (sz > 0)
		str[--sz] = '\0';
	else if (sz < 0)
		sz = -sz;
	else {
		while (str[sz] != '\0')
			++sz;
	}

	i = sz;

	if (nbr == 0) {
		if (i > 0)
			str[--i] = '0';
	} else {
		while (nbr > 0 && i > 0) {
			str[--i] = (nbr % 10) + '0';
			nbr /= 10;
		}
	}

	while (i > 0)
		str[--i] = ' ';

	return str;
}


