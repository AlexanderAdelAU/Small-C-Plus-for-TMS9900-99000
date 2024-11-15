REM================================
REM Math Library
REM ===============================

REM Clear Read-Only Attributes
attrib  -r *.LIB
attrib  -r *.NDX
attrib  -r *.L$
attrib  -r *.N$
REM Delete the old files
del *.LIB
del *.NDX
del *.L$
del *.N$

REM================================
REM Floating Point Library
REM ===============================
smallcp -C -M float\float
R99 float/float SCHCLC
copy float\*.R99
R99 cfloatm48 SCHCLC



REM Make the library
lib99 -U fplib99 float cfloatM48
rem del *.R99



