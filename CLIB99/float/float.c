#include "float.h"

extern double _ccfloor();
extern double _ccfloat();
extern double _ccpow();
extern double _ccsin();
extern double _cctan();
extern double _cccos();
extern double _ccifix();
extern double _ccfabs();
extern double _cclog();
extern double _ccexp();
/*
 *	fp floor function
 */

floor (x)
double x;
{
	return _ccfloor(x);
}


float (x)
int x;
{
	return _ccfloat(x);
}

ifix (x)
double x;
{
	return _ccifix(x);
}

sin(x)
double x;
{
	return _ccsin(x);
}

cos(x)
double x;
{
	return _cccos(x);
}

tan(x)
double x;
{
	return _cctan(x);
}

fabs(x)
double x;
{
	return _ccfabs(x);
}


log(x)
double x;
{
	return _cclog(x);
}

exp(x)
double x;
{
	return _ccexp(x);
}

pow(x,y)
double x;double y;
{
	return _ccpow(x,y);
}


