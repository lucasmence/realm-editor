@echo off
for /f %%i in ('savefile.exe') do set file=%%i
echo %file% >> ../temp/savefile.data
exit /b