Installation et exécution étape par étape.

Le ./configure active le débogage, désactive tous les codecs et muxers/demuxers sauf le déc. h264, les demux. avi, mpegts, mpegtsraw et matroska (.mkv) pour compiler plus vite.
Pour tester avec d'autres formats/codecs il faut activer les bons demuxers et décodeurs.

1° Installer la FFmpeg :

git clone https://github.com/jojva/FFmpeg.git

./configure --enable-coverage --enable-debug=3 --enable-extra-warnings --assert-level=2 --disable-optimizations --disable-programs --disable-doc --disable-encoders --disable-decoders --enable-decoder=h264 --disable-muxers --disable-filters --disable-demuxers --enable-demuxer=avi --enable-demuxer=mpegts --enable-demuxer=mpegtsraw --enable-demuxer=matroska

sudo make

sudo make install

2° Compiler :

gcc --coverage -g -o extract extract.c -lswscale -lavutil -lavformat -lavcodec -lavutil -lm -lpthread
