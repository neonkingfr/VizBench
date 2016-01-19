
taskkill /f /im igesture.exe
taskkill /f /im ffff.exe
taskkill /f /im PlogueBidule_x64.exe

start igesture.exe -v

start c:\users\tjt\documents\github\vizbench\config\bidule\spacepuddle.bidule

rem If I start ffff.exe right after Bidule, Bidule crashes - possibly/probably
rem because FFFF sends OSC to Bidule, before Bidule is ready.
sleep 5

start c:\users\tjt\documents\github\vizbench\bin\ffff.exe -c spacepuddle

rem Don't put the resize right after the invokation of igesture.exe,
rem because the window won't exist right away.
nircmd win setsize ititle igesture.exe 10 550 1000 200
