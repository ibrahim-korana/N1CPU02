echo off
del %1.tar
rmdir %1 /s /q
mkdir %1
cd %1
copy ..\build\bootloader\*.bin .
copy ..\build\partition_table\*.bin .
copy ..\build\*.bin .
cd ..
tar -c -f %1.tar %1
rmdir %1 /s /q
echo on
