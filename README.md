# SmallCP-for-TMS9900
This implementation of Small C plus by  Cain, Van Zandt, Hendrix, Yorston emits code that will run on a TMS9900/TMS99000 
Single Board Computer with at least 64k Bytes of memory.    This version of Small-C is a major improvement on small C version 2.1
and includes floating point support as well as structures and unions and other standard C language support that is absent from
Small C version 2.1

It is compiled using MingW GCC and therefore operates as a Cross Compiler however the C code is standard C and 
should be able to self compile with the appropriate OS support on the target computer.  The Eclipse project files are included as a zip file that
may help building and testing the compiler in your own environment.

An IOLIB is included that supports CP/M compatible operating system calls and a core set of 40bit floating point library routines.  The core set
includes, FPADD, FPMUL,FPSUB and FPDIV.   Other functions such as trancedentals will be added later.  The TMS9900 Floating Point Library filename is CFLOAT48.A99 and
was ported from a Z80 Floating Point library.

A test programme for the Byte Sieve.c benchmark is included and runs on a 20MHz TMS99105A in 18 seconds.

A sample test programme is 

```
#include "stdio.h"
#include "float.h"

int i;
int fd;
double dd;
main (argc,argv) int argc; char *argv[]; {
	char ch;
	char str[32];

	dd = 2.545/10.0;
	strcpy(str, "2.545/10.0");
	printf("\n1. %s = %6.3f",str,dd);

	dd = 2.545*10.0;
	strcpy(str, "2.545*10.0");
	printf("\n2. %s = %6.3f",str,dd);

	dd = 2.545+10.0;
	strcpy(str, "2.545+10.0");
	printf("\n3. %s = %6.3f",str,dd);

	dd = 2.545-10.0;
	strcpy(str, "2.545-10.0");
	printf("\n4. %s = %6.3f",str,dd);

	puts("\n Testing disc i/o");
	
    	printf("\nOutput argc for main = %d\n",argc);
	    for(i=1;i< argc;i++)
	    {
	        printf("%s%c",argv[i],(i<argc-1) ? ' ' : '\n');
	    }

	fd = fopen("test1","w");
	printf ("Create new file test1 for writing fd = %d\n",fd);
	ch = putc('T',fd);
	printf("One character written...\n");
	fclose(fd);

	fd = fopen("test1","r");
	printf ("Open file test1 for reading fd = %d\n",fd);
	ch = getc(fd);
	printf ("Output char  = %c\n",ch);
	fclose(fd);
	printf("\nEnd of Test\n");
}

```
```
A output of running the above is shown below

1. 2.545/10.0 =   .255
2. 2.545*10.0 = 25.450
3. 2.545+10.0 = 12.545
4. 2.545-10.0 = -7.455

Testing disc i/o
Output argc for main = 1
Create new file test1 for writing fd = 9
One character written...
Open file test1 for reading fd = 9
Output char  = T

End of Test
%
```
