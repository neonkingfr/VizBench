@echo off

if x%1 == x echo "you need to provide an argument!"
if x%1 == x goto getout:

set VIZCONFIG=config_puddle_%1
set FFFFPIPELINE=spacepuddle_%1

taskkill /f /im igesture.exe
taskkill /f /im ffff.exe
taskkill /f /im PlogueBidule_x64.exe

rem Let debut die gracefully, so it doesn't complain on next startup
taskkill /im debut.exe

rem let debut.exe finish finishing
sleep 1

rem In the current configuration on my "slate" machine, the audio
rem setup is extremely fragile, and in order for Bidule's audio to work,
rem Debug needs to be started first.
start "DEBUT" "c:\program files (x86)\NCH Software\Debut\debut.exe"

rem we want to giv debut time to start before Bidule starts
sleep 10

start igesture.exe -v

start c:\users\tjt\documents\github\vizbench\config\bidule\spacepuddle.bidule

rem If I start ffff.exe right after Bidule, Bidule crashes - possibly/probably
rem because FFFF sends OSC to Bidule, before Bidule is ready.
sleep 5

start c:\users\tjt\documents\github\vizbench\bin\ffff.exe -c %FFFFPIPELINE%

rem Don't put the resize right after the invokation of igesture.exe,
rem because the window won't exist right away.
nircmd win setsize ititle igesture.exe 10 550 1000 200

:getout
