# SmallCP-for-TMS9900
This implementation of Small C plus by  Cain, Van Zandt, Hendrix, Yorston emits code that will run on a TMS9900/TMS99000 
Single Board Computer with at least 64k Bytes of memory.    This version of Small-C is a major improvement on small C version 2.1
and includes floating point support as well as structures and unions and other standard C language support that is absent from
Small C version 2.1

It is compiled using MingW GCC and therefore operates as a Cross Compiler however the C code is standard C and 
should be able to self compile with the appropriate OS support on the target computer.  The Eclipse project files are included as a zip file that
may help building and testing the compiler in your own environment.

An IOLIB is included that supports CP/M compatible operating system calls.

A test programme for the Byte Sieve.c benchmark is included and runs on a 20MHz TMS99105A in 18 seconds.
