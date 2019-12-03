cd /D %~dp0
gcc -c -DTPL_NOLIB -Wall dlpspec.c dlpspec_scan.c dlpspec_calib.c dlpspec_util.c tpl.c win\mmap.c dlpspec_scan_col.c dlpspec_scan_had.c dlpspec_helper.c
ar rs libdlpspec.a dlpspec.o dlpspec_scan.o dlpspec_calib.o dlpspec_util.o tpl.o mmap.o dlpspec_scan_had.o dlpspec_scan_col.o dlpspec_helper.o
del *.o
