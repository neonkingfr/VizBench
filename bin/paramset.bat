@echo off
if x%3 == x echo Needs instance, paramname, and value
if not x%1 == x python jsonrpc.py 4448 ffff.ffglparamset "{\"instance\":\"%1\",\"param\":\"%2\",\"val\":%3}"
