rem This only copies binaries for DLLs and executables

rem NOTE - the *Release directories are used
copy %VIZBENCH%\ffglpluginsRelease\*.* %VIZLETS%\ffglplugins
copy %VIZBENCH%\binRelease\FFFF.exe %VIZLETS%\bin

copy %VIZBENCH%\bin\jsonrpc.exe %VIZLETS%\bin
copy %VIZBENCH%\bin\oscutil.exe %VIZLETS%\bin
copy %VIZBENCH%\bin\python27.dll %VIZLETS%\bin

copy %VIZBENCH%\bin\pthreadVC2.dll %VIZLETS%\bin
copy %VIZBENCH%\bin\VizDll.dll %VIZLETS%\bin

copy %VIZBENCH%\src\oscutil\dist\*.* %VIZLETS%\bin
copy %VIZBENCH%\src\jsonrpc\dist\*.* %VIZLETS%\bin

rem copy %VIZBENCH%\bin\*.osc %VIZLETS%\bin

rem clean up temporary files
del /s %VIZLETS%\html\*~
del /s %VIZLETS%\bin\*~
del /s %VIZLETS%\config\*~

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
