@echo off
::
:: must be called as postbuild.cmd <solutiondir> <targetbin>
::

cd ..\..\soare-tools\
rm soare-iso\boot\soare.bin
cp %2 soare-iso\boot\soare.bin
:: ls soare-iso\boot
:: bash -c "./create-iso.sh"
