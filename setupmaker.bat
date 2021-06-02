set smpath=%USERPROFILE%\Desktop\Lamina

if not exist %smpath% mkdir %smpath%
if not exist %smpath%\scripts mkdir %smpath%\scripts
if not exist %smpath%\data mkdir %smpath%\data
if not exist %smpath%\data\options mkdir %smpath%\data\options

copy Debug\realm-editor.exe %smpath%\realm-editor.exe
copy realm-editor.png %smpath%\realm-editor.png
copy realm-editor.ico %smpath%\realm-editor.ico
copy data\options\realm-editor.json %smpath%\data\options\realm-editor.json
robocopy scripts %smpath%\scripts /E
 
pause