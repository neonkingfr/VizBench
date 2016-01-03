set config=%1
set project=%2
cd ..\..\..\bin

echo Generating param support for config=%config% project=%project%

c:\python27\python.exe genparams.py Sprite
