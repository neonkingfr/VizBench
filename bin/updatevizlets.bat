rem copies binaries and other things from %VIZBENCH% to %VIZLETS%
rem so it can be packaged up for distribution.

@if x%1 == x echo Usage: updatevizlets "{Debug|Release}"
@if x%1 == x goto getout:

set config=%1

copy %VIZBENCH%\ffglplugins%config%\*.* %VIZLETS%\ffglplugins
copy %VIZBENCH%\bin%config%\FFFF.exe %VIZLETS%\bin
copy %VIZBENCH%\bin%config%\VizServer.dll %VIZLETS%\bin

rem get rid of plugins that shouldn't be distributed
del %VIZLETS%\ffglplugins\VizLooper.dll
del %VIZLETS%\ffglplugins\VizletTemplate.dll

rem these are the python-based EXEs, no separate Release or Debug version
copy %VIZBENCH%\bin\jsonrpc.exe %VIZLETS%\bin
copy %VIZBENCH%\bin\oscutil.exe %VIZLETS%\bin

rem other DLLs
copy %VIZBENCH%\bin\python27.dll %VIZLETS%\bin
copy %VIZBENCH%\bin\pthreadVC2.dll %VIZLETS%\bin

copy %VIZBENCH%\src\oscutil\dist\*.* %VIZLETS%\bin
copy %VIZBENCH%\src\jsonrpc\dist\*.* %VIZLETS%\bin

copy %VIZBENCH%\bin\p.bat %VIZLETS%\bin
copy %VIZBENCH%\bin\mf.bat %VIZLETS%\bin
copy %VIZBENCH%\bin\api.bat %VIZLETS%\bin
copy %VIZBENCH%\bin\oscplayback.bat %VIZLETS%\bin
copy %VIZBENCH%\bin\osclisten.bat %VIZLETS%\bin

rem HTML
copy %VIZBENCH%\html\*.html %VIZLETS%\html
copy %VIZBENCH%\html\*.js %VIZLETS%\html

rem Params
copy %VIZBENCH%\config\params\*.json %VIZLETS%\config\params

rem copy %VIZBENCH%\bin\*.osc %VIZLETS%\bin

rem clean up temporary files
del /s %VIZLETS%\html\*~
del /s %VIZLETS%\bin\*~
del /s %VIZLETS%\config\*~
del /s %VIZLETS%\shaders\*~
del /s %VIZLETS%\params\*~

set zip="\program files\7-Zip\7z.exe"
set libraryzip="%VIZBENCH%\bin\library.zip"

rem ----------------------------------------------------------------------
rem This section combines the library.zip files from several py2exe builds
cd %VIZBENCH%\bin
del /s /q library.zip
rmdir /s /q libraryzipdir
mkdir libraryzipdir
cd libraryzipdir
%zip% x %VIZBENCH%\src\oscutil\dist\library.zip >nul
%zip% x -y %VIZBENCH%\src\jsonrpc\dist\library.zip >nul
%zip% a -r ..\library.zip *.* >nul
cd ..
rmdir /s /q libraryzipdir
copy %VIZBENCH%\bin\library.zip %VIZLETS%\bin
rem ----------------------------------------------------------------------

:getout
