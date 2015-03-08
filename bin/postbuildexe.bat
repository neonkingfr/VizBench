rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.exe to %VIZBENCH%\bin%config% and %VIZBENCH%\bin

copy %config%\%project%.exe %VIZBENCH%\bin%config%\%project%.exe
copy %config%\%project%.exe %VIZBENCH%\bin\%project%.exe
