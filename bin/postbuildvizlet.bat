rem config is either Release or Debug
set config=%1
set project=%2

echo copying %config%\%project%.dll to ..\..\..\ffglplugins%config% and ..\..\..\ffglplugins

copy %config%\%project%.dll ..\..\..\ffglplugins%config%\%project%.dll
copy %config%\%project%.dll ..\..\..\ffglplugins\%project%.dll
