cd /cygdrive/c/users/tjt/documents/github/vizbench/recording
rm *.ppm
cd ../bin
set -x
# SAVEFRAMES=one ffff.exe vizexample2 &
FFMPEGFILE=miditest ffff.exe vizexample2 &
pid=$!
echo pid=$pid
sleep 2
mf.bat prelude.mid
# sleep 40
# Don't kill it abruptly - ffff.exe needs to close ffmpeg output properly
# kill $pid
