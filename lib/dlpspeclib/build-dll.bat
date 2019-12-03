cd /D %~dp0
C:\Qt\Tools\mingw530_32\bin\gcc -c -DTPL_NOLIB -Wall dlpspec.c dlpspec_scan.c dlpspec_calib.c dlpspec_util.c tpl.c win\mmap.c dlpspec_scan_col.c dlpspec_scan_had.c dlpspec_helper.c
C:\Qt\Tools\mingw530_32\bin\gcc -shared -o libdlpspec.dll dlpspec.o dlpspec_scan.o dlpspec_calib.o dlpspec_util.o tpl.o mmap.o dlpspec_scan_had.o dlpspec_scan_col.o dlpspec_helper.o
del *.o
