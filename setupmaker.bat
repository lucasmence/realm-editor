set smpath=%USERPROFILE%\Desktop\Boltcraft

if not exist %smpath% mkdir %smpath%
if not exist %smpath%\scripts mkdir %smpath%\scripts
if not exist %smpath%\data mkdir %smpath%\data
if not exist %smpath%\data\options mkdir %smpath%\data\options
if not exist %smpath%\templates mkdir %smpath%\templates

copy Release\realm-editor.exe %smpath%\realm-editor.exe
copy realm-editor.png %smpath%\realm-editor.png
copy realm-editor.ico %smpath%\realm-editor.ico
copy data\options\realm-editor.json %smpath%\data\options\realm-editor.json
robocopy scripts %smpath%\scripts /E
robocopy templates %smpath%\templates /E
 
pause