@echo off
SET folder=%~dp0..\
for /f %%i in ('savefile.exe *.json %folder%') do set file=%%i
echo %file% >> ../temp/savefile.data
exit /b