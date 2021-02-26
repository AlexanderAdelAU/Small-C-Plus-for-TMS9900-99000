/*
 *	STRING FUNCTIONS FOR SMALL C
 * 	BASED ON CORRESPONDING UNIX FUNCTIONS
 */

#include <stdio.h>
//#include <string.h>


strcpy( to, from )
char *to, *from ;
{
	char *temp ;

	temp = to ;
	while( *to++ = *from++ ) ;
	return temp ;
}

