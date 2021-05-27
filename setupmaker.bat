set smpath=%USERPROFILE%\Desktop\Lamina

if not exist %smpath% mkdir %smpath%
if not exist %smpath%\scripts mkdir %smpath%\scripts

copy Debug\realm-editor.exe %smpath%\realm-editor.exe
copy realm-editor.png %smpath%\realm-editor.png
copy realm-editor.ico %smpath%\realm-editor.ico
robocopy scripts %smpath%\scripts /E
 
pause