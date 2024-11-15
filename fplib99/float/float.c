#include "float/float.h"

extern double _ccfloor();
extern double _ccfloat();
extern double _ccpow();
extern double _ccsin();
extern double _cctan();
extern double _ccatan();
extern double _cccos();
extern double _ccifix();
extern double _ccfabs();
extern double _cclog();
extern double _ccexp();
extern double _ccsqrt();
extern double _ccmod();
extern double _cclog10();
/*
 *	These are the Floating Point prototype functions declarations that call the
 *	floating point libary entry points.
 */

double floor (x)
double x;
{
	return _ccfloor(x);
}


double float (x)
int x;
{
	return _ccfloat(x);
}

int ifix (x)
double x;
{
	return _ccifix(x);
}

double sin(x)
double x;
{
	return _ccsin(x);
}

double cos(x)
double x;
{
	return _cccos(x);
}

double tan(x)
double x;
{
	return _cctan(x);
}

double atan(x)
double x;
{
	return _ccatan(x);
}


double fabs(x)
double x;
{
	return _ccfabs(x);
}


double log(x)
double x;
{
	return _cclog(x);
}

double log10(x)
double x;
{
	return _cclog10(x);
}

double exp(x)
double x;
{
	return _ccexp(x);
}

double sqrt(x)
double x;
{
	return _ccsqrt(x);
}

double pow(x,y)
double x;double y;
{
	return _ccpow(x,y);
}

double fmod(x,y)
double x;double y;
{
	return _ccmod(x,y);
}


