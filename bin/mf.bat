@echo off
if x%1 == x apiffff VizServer.playmidifile "{\"file\": \"abc.mid\"}"
if not x%1 == x apiffff VizServer.playmidifile "{\"file\": \"%1\"}"
