rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.dll to ..\..\..\ffplugins%config% and ..\..\..\ffplugins

copy %config%\%project%.dll ..\..\..\ffplugins%config%\%project%.dll
copy %config%\%project%.dll ..\..\..\ffplugins\%project%.dll
