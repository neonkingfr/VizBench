rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.dll to %VIZBENCH%\bin%config% and %VIZBENCH%\bin

copy %config%\%project%.dll %VIZBENCH%\bin%config%\%project%.dll
copy %config%\%project%.dll %VIZBENCH%\bin\%project%.dll
