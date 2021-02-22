
attrib  -r *.LIB
attrib  -r *.NDX
attrib  -r *.L99
attrib  -r *.L$
attrib  -r *.N$

del iolib99.LIB
del iolib99.NDX
del iolib99.L$
del iolib99.N$
del iolib.r99
smallcp -C -M iolib
R99 iolib SCHCLC
R99 call SCHCLC
R99 cbdos SCHCLC
rem R99 cfloat SCHCLC
R99 cfloatm48 SCHCLC
lib99 -U iolib99 iolib cfloatm48 cbdos call
REM lib99 -U iolib99 iolib cbdos call
REM del iolib.a99
REM del iolib.r99
