rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.dll to ..\..\..\bin%config% and ..\..\..\bin

copy %config%\%project%.dll ..\..\..\bin%config%\%project%.dll
copy %config%\%project%.dll ..\..\..\bin\%project%.dll
