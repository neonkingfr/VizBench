rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.exe to ..\..\..\bin%config% and ..\..\..\bin

copy %config%\%project%.exe ..\..\..\bin%config%\%project%.exe
copy %config%\%project%.exe ..\..\..\bin\%project%.exe
