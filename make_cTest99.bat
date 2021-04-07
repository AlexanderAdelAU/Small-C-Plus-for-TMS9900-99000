REM Clear Read-Only Attributes
attrib  -r *.COM
attrib  -r *.L99
attrib  -r *.R$
attrib  -r *.D$
REM Delete the old files
del *.COM
del *.R$
del *.D$
del *.L99
smallcp -C cTest99
smallcp -C fptable
R99 cTest99 SCHCLC
link99  -M  -S cTest99 clib99.LIB iolib99.LIB strlib99.LIB fplib99.LIB

