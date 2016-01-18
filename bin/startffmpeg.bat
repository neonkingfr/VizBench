c:\local\ffmpeg\bin\ffmpeg.exe -r 60 -f rawvideo -pix_fmt bgr24 -s 1024x768 -i - -threads 0 -preset fast -y -r 60 -vf vflip ffmpegtestX.mp4
