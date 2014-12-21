@echo off
if x%1 == x echo Needs plugin instance name
if not x%1 == x python jsonrpc.py 4448 ffff.ffglparamlist "{\"instance\":\"%1\"}"
