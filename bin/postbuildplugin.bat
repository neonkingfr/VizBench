rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.dll to %VIZBENCH%\ffglplugins%config% and %VIZBENCH%\ffglplugins

copy %config%\%project%.dll %VIZBENCH%\ffglplugins%config%\%project%.dll
copy %config%\%project%.dll %VIZBENCH%\ffglplugins\%project%.dll
