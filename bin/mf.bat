@echo off

if x%1 == x call api VizServer.set_midifile "{\"file\": \"abc.mid\"}"
if not x%1 == x call api VizServer.set_midifile "{\"file\": \"%1\"}"

call api VizServer.play_midifile "{}"
