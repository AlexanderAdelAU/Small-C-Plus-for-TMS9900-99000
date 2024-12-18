# Small C Plus Compiler for the TMS9900/TMS99000 - Version 1.06a
I have implemented new version of the Small C plus by  Cain, Van Zandt, Hendrix, Yorston that is essentially a full K&R version that emits code that will run on a TMS9900/TMS99000 
Single Board Computer with at least 64k Bytes of memory.   This new version of Small-C is a major improvement over other versions of Small C and includes unsigned integers and chars, floating point support, as well as structures and unions and other standard C language support that are absent from earlier versions.  The TMS9900.99000 architecture is 16 bit and so the compiler has had to be modified to extend the addressing to factor that all instructions must lie on and even or word boundary.  So chars for example whilst defined as 8 bit, actually occupy 16 bits of memory.  Byte data and text is of course byte oriented.  It can compile very large applications, for example it compiles and runs the VT100 (https://github.com/TeraTermProject/teraterm) screen editor **ED2** https://github.com/mdlougheed/ed2  without difficulty. 

A screen capture of **ED2** running on a SBC (https://github.com/AlexanderAdelAU/TMS9900-SBC) that was compiled under this version of Small C is shown here:

<div style="text-align:center;">
    <img src="images/Ed2.png" alt="Screen capture" width="600">
</div>


The source for the BDOS, and IO libraries that are required to run ED2 are included on this site and is built using the following commands:
  ```	
   smallcp -C src/ED2
   R99 src/ED2 SCHCLC
   link99  -M -S src/ED2 iolib99.LIB clib99.LIB.
```
The code produced by the Small C compiler can be either compiled to produce standalone code (using the -M) flag and assembled using the A99 assembler to run at an absolute location in memory.  If it needs to be linked to other runtime libraries then it will produce relocatable assembly code (using the -C flag) that can be assembled and linked with other modules using the R99 (Relocatable assembler) assembler in conjuction with the linker loader (Link99).  The relocatable applications use the standard REL format.

It is compiled using MingW GCC and therefore operates as a Cross Compiler however the C code is standard C and 
should be able to self compile with the appropriate OS support on the target computer.  The Eclipse project files are included as a zip file that
may help building and testing the compiler in your own environment.

A test programme for the Byte Sieve.c benchmark is included and runs on a 16MHz TMS99105A in 16 seconds.

## Floating Point Library
The floating point library implementation is a retargetting of  Anders Hejlsberg's Z80 Floating Point library (FloatM48.z80).  The source code is written in assembler and is generally included through linking it with other libraries when building the application.   The method of building and linking along with the source is included in the folder __fplib99__.

An IOLIB is included that supports CP/M compatible (BDOS in this respository) operating system calls and a core set of 40bit floating point library routines.  The core set includes, FPADD, FPMUL,FPSUB, FPDIV, SIN, COS, TAN, POW,LN,EXP, SQRT.   The actual Floating Point Library is included during Linking CLIB99f, where the 'f' indicaates it contains the floating point library( CLIB99 does not). Other functions such as trancedentals will be added later.  

A sample test programme that demonstrates how to calculate a Double Integral using Simpson's Rule is shown below and is compiled using the following script:

```
	copy DoubleIntegralSimpsonsRule.c, cDSR99.c
	smallcp -C cDSR99
	R99 cDSR99 SCHCLC
	link99  -M  -S cDSR99 clib99f.LIB iolib99.LIB
```

The programme performs a double integration on the following mathemtical function"

```
	zd = cos(pi * cos(x) / 2.0) * cos(pi * (1.0 - sin(x) * cos(y)) / 4.0);
	zd = pow(zd, 2.0)/sin(x);
```

This is a very good test of the Floating Point library.

```
/*
 ============================================================================
 Name        : DoubleIntegralSimpsonsRule.c
 Author      : Alex Cameron
 Version     :
 Copyright   : None
 Description : Simpson integration technique for
 	 	 	   evaluating  double integrals
 ============================================================================
 */

#include "float.h"
double fxy[100], fy[12];
double pi;
double f();

main() {

	double llx, lly, ulx, uly, x, y;
	double hx, hy, ef, of, simpson;
	int nosx, nosy, i, j;

	/*
	 *
	============================================================================
	 *
	 *	Simpson integration constants.
	 *		nos -> number of strips
	 *		ul  -> upper limit of integration
	 *		ll  -> lower limit of integration
	 *		h   -> incremental value per strip
	 =============================================================================
	 */
	pi = 3.1415926;
	nosx = 10;
	nosy = 8;

	llx = 0.1e-8;
	lly = 0.1e-8;

	ulx = pi;
	uly = 2.0 * pi;

	hx = (ulx - llx) / nosx;
	hy = (uly - lly) / nosy;

	printf("\n\nDouble integration parameters: \n");
	printf(" Step size hx and hy:  %10.6f,%10.6f\n", hx, hy);
	printf(" Number of steps (x,y): %d,%d\n\n", nosx, nosy);

	/*
	 Calculate all the points within the integration domain
	 */

	for (j = 0; j <= nosy; j++) {
		y = j * hy + lly;
		for (i = 0; i <= nosx; i++) {
			x = i * hx + llx;
			fxy[i+j*nosx] = f(x, y);
		}
	}
	/*
	 ========================================================
	 Now perform a Simpson integration along
	 the x axis and accumulate results using
	 the y axis variable as an index
	 ===========================================================
	 */

	for (j = 0; j <= nosy; j++) {
		of = fxy[1 + j*nosx];
		ef = 0.0;
		for (i = 2; i <= nosx - 2; i += 2) {
			ef += fxy[i + j*nosx];
			of += fxy[i + 1 + j*nosx];
		}
		fy[j] = fxy[0 + j*nosy] + fxy[nosx + j*nosx] + 2.0 * ef + 4.0 * of;

	}

	/*
	 * ========================================================
	 * Lastly perform Simpson integration
	 *  along the y axis.
	 * ========================================================
	 */
	of = fy[1];
	ef = 0.0;
	for (j = 2; j <= nosy - 2; j += 2) {
		ef += fy[j];
		of += fy[j + 1];
	}
	simpson = (hx * hy / 9.0) * (fy[0] + fy[nosy] + 2.0 * ef + 4.0 * of);
	printf("Result = %10.6f\n\n", simpson);
}

/*
 * ==============================================================
 * Enter the function to be integrated here.
 * plot  abs(((cos(pi*cos(x)/2.0)*cos(pi*(1.0-sin(x)*cos(y))/4.0))^2)/sin(x))
 * ===============================================================
 */
double f(x, y)
double x, y; {
double zd;

	zd = cos(pi * cos(x) / 2.0) * cos(pi * (1.0 - sin(x) * cos(y)) / 4.0);
	zd = pow(zd, 2.0)/sin(x);
	return (fabs(zd));
}



```
```
A output of running the above is shown below.

%cDSR99

Double integration parameters: 
 Step size hx and hy:    0.314159,  0.785398
 Number of steps (x,y): 10,8

Result =   3.827845
%
```
