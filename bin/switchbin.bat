rem @echo off
if x%1 == x echo "Usage: switchbin {Release|Debug}"

if x%1 == xDebug copy %VIZBENCH%\binDebug\* %VIZBENCH%\bin
if x%1 == xDebug copy %VIZBENCH%\binDebug\VizServer.dll %VIZBENCH%\bin
if x%1 == xDebug copy %VIZBENCH%\ffglpluginsDebug\* %VIZBENCH%\ffglplugins

if x%1 == xRelease copy %VIZBENCH%\binRelease\* %VIZBENCH%\bin
if x%1 == xRelease copy %VIZBENCH%\binRelease\VizServer.dll %VIZBENCH%\bin
if x%1 == xRelease copy %VIZBENCH%\ffglpluginsRelease\* %VIZBENCH%\ffglplugins
