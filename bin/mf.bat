@echo off
if x%1 == x apiffff play_midifile "{\"file\": \"abc.mid\"}"
if not x%1 == x apiffff play_midifile "{\"file\": \"%1\"}"
