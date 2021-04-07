REM================================
REM Printf Library
REM ===============================
smallcp -C -M printf\itod
R99 printf\itod SCHCLC
smallcp -C -M printf\itou
R99 printf\itou SCHCLC
smallcp -C -M printf\utoi
R99 printf\utoi SCHCLC
smallcp -C -M printf\xtoi
R99 printf\xtoi SCHCLC
REM smallcp -C -M printf\itox
R99 printf\itox SCHCLC
REM smallcp -C -M printf\printf
REM R99 printf\printf SCHCLC
smallcp -C -M printf\printf2
R99 printf\printf2 SCHCLC
copy printf\*.R99
REM================================
REM Floating Point Library
REM ===============================
smallcp -C -M float\float
R99 float/float SCHCLC
copy float\*.R99

REM================================
REM String Library
REM ===============================
 smallcp -C -M strlib\strcpy
 R99 strlib\strcpy SCHCLC
 copy strlib\*.R99

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
REM Make the library
lib99 -U clib99 printf2 itod itou utoi xtoi itox 
lib99 -U strlib99 strcpy
lib99 -U fplib99 float 
del *.R99



