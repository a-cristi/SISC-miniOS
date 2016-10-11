@echo off
del *.pdb /s  > cleanup.log
del *.ilk /s  >> cleanup.log
del int\*.* /s /q  >> cleanup.log
del *.sdf  >> cleanup.log
del *.suo /ah  >> cleanup.log