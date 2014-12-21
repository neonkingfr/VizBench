@echo off
if x%VIZBENCH% == x echo "VIZBENCH environment variable needs to be set"
if not x%VIZBENCH% == x python %VIZBENCH%\bin\genvizlet.py %1 %2 %3 %4 %5 %6 %7 %8 %9
