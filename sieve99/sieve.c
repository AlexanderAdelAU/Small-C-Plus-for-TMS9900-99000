/*
 Sieve of Eratosthenes benchmark from BYTE, Sep. '81, pg. 186.
 Compile by:
 A>cc sieve.c -e1000
 All variables have been made external to speed them up; other than
 that one change, the program is identical to the one used in the BYTE
 benchmark....while they got an execution time of 49.5 seconds and a
 size of 3932 bytes using v1.32 of the compiler,
 ********
 v1.45 clocks in at  * 15.2 *  seconds with a size of 3718 bytes!
 ********
 */
#include "stdio.h"

#define TRUE 1
#define FALSE 0
#define SIZE 8190
#define SIZEPL 8191

char flags[SIZEPL];

main(argc, argv) int argc; char *argv[];{

	int i, prime,k,count,iter;
/*
	printf("%d\n",argc);
	for(i=1;i< argc;i++)
	{
		printf("%s%c",argv[i],(i<argc-1) ? ' ' : '\n');
	}
*/
	printf("10 iterations\n");
	for (iter = 1; iter <= 10; iter++) {
		count = 0;
		for (i = 0; i <= SIZE; i++)
		flags[i] = TRUE;
		for (i = 0; i <= SIZE; i++) {
			if (flags[i]) {
				prime = i + i + 3;
				k = i + prime;
				while (k <= SIZE) {
					flags[k] = FALSE;
					k += prime;
				}
				count++;
			}
		}
	}
	printf("\n%d primes.\n",count);
}

