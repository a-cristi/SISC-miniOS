@echo off
::
:: must be called as postbuild.cmd <solutiondir> <targetbin>
::

:: rem :: Needed only if you use the custom VM -> delete all "rem"
:: 
:: rem set target_mac=00.50.56.2C.D5.76
:: 
:: rem echo POSTBUILD: copying %2 to c:\VM\tftpfolder\%target_mac%.bin
:: rem copy %2 c:\VM\tftpfolder\%target_mac%.bin
:: 
:: echo POSTBUILD: will insert %2 into SOARE2-FLAT.VMDK...
:: echo IMPORTANT: if the Disk is running under VMWARE, you MUST stop it to succeed!
:: 
:: vmware-mount.exe z: D:\SISC\Sem1\PSNA\miniOS\soare-tools\soare2.vmdk
:: copy %2 z:\soare.bin
:: vmware-mount.exe z: /d
