rem jsonrpc.exe 80 ffff.record "{\"onoff\": %1 }"

oscutil send 3210@127.0.0.1 /Audio_File_Recorder_0/Recording %1
