rem jsonrpc.exe 80 ffff.audio "{\"onoff\": %1 }"

oscutil send 3210@127.0.0.1 /play %1
