dir=`pwd`
thisdir=`basename $dir`
if [ "$thisdir" != "recordings" ]
then
	echo "Hey, $0 needs to be called from a directory named 'recordings'"
	exit 1
fi

tag=`ls -lt *.rawvideo 2>/dev/null | head -1 | sed -e 's/.* //' -e 's/.rawvideo//'`
if [ "$tag" == "" ]
then
	echo "There's no *.rawvideo file!?"
	exit 1
fi

echo "Saving audio recording..."

if [ -f $tag.wav ]
then
	echo "Reprocessing $tag.wav ..."
	rm ${tag}_final.mp4
else
	mv spacepuddle.wav $tag.wav
fi

quiet="-hide_banner -loglevel quiet"
quiet=""

echo "Converting rawvideo to mp4..."

../other/ffmpeg/bin/ffmpeg.exe $quiet -r 30 -f rawvideo -pix_fmt bgr24 -s 640x480 -i $tag.rawvideo -threads 0 -preset fast -y -r 30 -vf vflip $tag.mp4

echo "Merging mp4 and audio..."

delayaudio="-1.0"
delayvideo="0.0"

../other/ffmpeg/bin/ffmpeg.exe $quiet -itsoffset $delayaudio -i $tag.wav -itsoffset $delayvideo -r 30 -i $tag.mp4 -c:v libx264 -preset slow -crf 18 ${tag}_final.mp4

echo ""
echo "RESULTS:"
echo ""
ls -l ${tag}*
echo ""

exit 0
