
copy dist\*.exe %VIZBENCH%\bin
copy dist\*.pyd %VIZBENCH%\bin
copy dist\*.dll %VIZBENCH%\bin

rem ----------------------------------------------------------------------
rem This section combines the library.zip files from several py2exe builds
set zip="\program files\7-Zip\7z.exe"
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

@if not x%VIZLETS% == x copy %VIZBENCH%\bin\library.zip %VIZLETS%\bin
rem ----------------------------------------------------------------------
