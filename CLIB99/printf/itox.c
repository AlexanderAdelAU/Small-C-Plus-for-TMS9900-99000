/*
 * itox -- converts nbr to hex string of length sz
 *	       right adjusted and blank filled, returns str
 *
 *	      if sz > 0 terminate with null byte
 *	      if sz  =  0 find end of string
 *	      if sz < 0 use last byte for data
 */
#include "stdio.h"
itox(nbr, str, sz)
    int nbr; char str[]; int sz; {
    int digit, offset, i;

    nbr &= 0xFFFF; /* Mask to 16 bits */

    if (sz > 0) {
        str[--sz] = NULL;
    } else if (sz < 0) {
        sz = -sz;
    } else {
        while (str[sz] != NULL) {
            ++sz;
        }
    }

    i = sz;
    if (nbr == 0) {
        if (i > 0) {
            str[--i] = '0';
        }
    } else {
        int max_digits = 4; // Only allow 4 hex digits
        while (nbr && i > 0 && max_digits--) {
            digit = nbr & 0xF;
            offset = digit < 10 ? '0' : 'A' - 10;
            str[--i] = digit + offset;
            nbr >>= 4;
        }
    }

    while (i > 0) {
        str[--i] = ' ';
    }
    return str;
}
