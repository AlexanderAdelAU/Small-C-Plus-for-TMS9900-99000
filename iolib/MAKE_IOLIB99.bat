@echo off
setlocal EnableExtensions

REM ================================================================
REM IOLIB99 strict split build for Small-C/Plus 1.06a
REM
REM   src\       = all source/build input files
REM   toolchain\ = compiler/assembler/library executables
REM   build\     = disposable working directory (recreated every run)
REM   dist\      = final IOLIB99.LIB / IOLIB99.NDX only
REM
REM Nothing is compiled in the project root.
REM ================================================================

set "ROOT=%~dp0"
set "SRC=%ROOT%src"
set "TOOLS=%ROOT%toolchain"
set "BUILD=%ROOT%build"
set "DIST=%ROOT%dist"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"

if not exist "%SRC%\IOCORE.C" goto :missing_source
if not exist "%SRC%\IOSEEK.C" goto :missing_source
if not exist "%TOOLS%\smallcp.exe" goto :missing_tools
if not exist "%TOOLS%\r99.exe" goto :missing_tools
if not exist "%TOOLS%\lib99.exe" goto :missing_tools

REM Start with a completely clean work area so stale generated files
REM can never leak into a new library.
if exist "%BUILD%" rmdir /s /q "%BUILD%"
mkdir "%BUILD%"
if errorlevel 1 goto :fail_root

if not exist "%DIST%" mkdir "%DIST%"
del /f /q "%DIST%\iolib99.LIB" "%DIST%\iolib99.NDX" 2>nul

echo === Stage source into BUILD ===
copy /y "%SRC%\*.*" "%BUILD%\" >nul
if errorlevel 1 goto :fail_root

echo === Stage toolchain into BUILD ===
copy /y "%TOOLS%\*.exe" "%BUILD%\" >nul
if errorlevel 1 goto :fail_root

pushd "%BUILD%"
if errorlevel 1 goto :fail_root

echo.
echo === Compile C modules ===
call :compile iocore
if errorlevel 1 goto :fail_build
call :compile ioopen
if errorlevel 1 goto :fail_build
call :compile ioread
if errorlevel 1 goto :fail_build
call :compile iowrite
if errorlevel 1 goto :fail_build
call :compile ioseek
if errorlevel 1 goto :fail_build

echo.
echo === Assemble modules ===
call :assemble iocore
if errorlevel 1 goto :fail_build
call :assemble ioopen
if errorlevel 1 goto :fail_build
call :assemble ioread
if errorlevel 1 goto :fail_build
call :assemble iowrite
if errorlevel 1 goto :fail_build
call :assemble ioseek
if errorlevel 1 goto :fail_build
call :assemble call
if errorlevel 1 goto :fail_build
call :assemble cbdos
if errorlevel 1 goto :fail_build

echo.
echo === Create library ===
lib99 -U iolib99 iocore ioopen ioread iowrite ioseek cbdos call > lib99.log
set "RC=%ERRORLEVEL%"
type lib99.log
if not "%RC%"=="0" goto :fail_build
if not exist "iolib99.LIB" goto :fail_build
if not exist "iolib99.NDX" goto :fail_build

echo.
echo === Publish to DIST ===
copy /y "iolib99.LIB" "%DIST%\iolib99.LIB" >nul
if errorlevel 1 goto :fail_build
copy /y "iolib99.NDX" "%DIST%\iolib99.NDX" >nul
if errorlevel 1 goto :fail_build

popd

echo.
echo ================================================================
echo IOLIB99 build completed successfully.
echo Output: "%DIST%\iolib99.LIB"
echo Index : "%DIST%\iolib99.NDX"
echo Work  : "%BUILD%"
echo ================================================================
exit /b 0

:compile
smallcp -C -M %1 > %1.compile.log
set "RC=%ERRORLEVEL%"
type %1.compile.log
if not "%RC%"=="0" exit /b 1
"%FINDSTR%" /R /C:"Errors: *0" %1.compile.log >nul
if errorlevel 1 exit /b 1
exit /b 0

:assemble
R99 %1 SCHCLC > %1.assemble.log
set "RC=%ERRORLEVEL%"
type %1.assemble.log
if not "%RC%"=="0" exit /b 1
"%FINDSTR%" /C:"No error(s)." %1.assemble.log >nul
if errorlevel 1 exit /b 1
exit /b 0

:missing_source
echo.
echo *** BUILD FAILED: required source files are missing from "%SRC%" ***
exit /b 1

:missing_tools
echo.
echo *** BUILD FAILED: required toolchain files are missing from "%TOOLS%" ***
exit /b 1

:fail_build
echo.
echo *** IOLIB99 BUILD FAILED - no DIST library will be published ***
del /f /q "%DIST%\iolib99.LIB" "%DIST%\iolib99.NDX" 2>nul
popd
exit /b 1

:fail_root
echo.
echo *** IOLIB99 BUILD FAILED during staging ***
del /f /q "%DIST%\iolib99.LIB" "%DIST%\iolib99.NDX" 2>nul
exit /b 1
