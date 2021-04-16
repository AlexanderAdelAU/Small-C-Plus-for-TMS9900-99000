/*
 ============================================================================
 Name        : DounbleIntegralSimpsonsRule.c
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
double f(); double g();

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
			fxy[i+j*nosy] = f(x, y);
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
		of = fxy[1 + j*nosy];
		ef = 0.0;
		for (i = 2; i <= nosx - 2; i += 2) {
			ef += fxy[i + j*nosy];
			of += fxy[i + 1 + j*nosy];
		}
		fy[j] = fxy[0 + j*nosy] + fxy[nosx + j*nosy] + 2.0 * ef + 4.0 * of;

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
f(x, y)
double x, y; {
double zd;

	zd = cos(pi * cos(x) / 2.0) * cos(pi * (1.0 - sin(x) * cos(y)) / 4.0);
	zd = pow(zd, 2.0)/sin(x);
	return (fabs(zd));
}

 g(x,y) double x,y; {
	return (x*y*y);
}
