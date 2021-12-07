git clone https://github.com/LibRaw/LibRaw.git
cd raw2dat
cd LibRaw
git pull
make -f Makefile.dist -j6
cp lib/libraw_r.a ../libraw_r.a
cd ../../
