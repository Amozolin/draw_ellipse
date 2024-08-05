@echo off
if not defined DevEnvDir (call "\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat")  
cl /EHsc /W4 main.cpp user32.lib kernel32.lib d2d1.lib /link /out:program.exe 
