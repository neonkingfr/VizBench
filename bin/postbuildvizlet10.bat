rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.dll to %VIZBENCH%\ffplugins%config% and %VIZBENCH%\ffplugins

copy %config%\%project%.dll %VIZBENCH%\ffplugins%config%\%project%.dll
copy %config%\%project%.dll %VIZBENCH%\ffplugins\%project%.dll
