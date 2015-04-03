rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.dll to %VIZBENCH%\ff10plugins%config% and %VIZBENCH%\ff10plugins

copy %config%\%project%.dll %VIZBENCH%\ff10plugins%config%\%project%.dll
copy %config%\%project%.dll %VIZBENCH%\ff10plugins\%project%.dll
