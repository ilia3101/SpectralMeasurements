cd raw2dat_src
gcc -c -O3 raw2dat.c
gcc -c -O3 RawReader.c
gcc raw2dat.o RawReader.o libraw_r.a -o raw2dat -lstdc++ -lm -lz
mv raw2dat ../../raw2dat
rm *.o
cd ..

cd process_data_src
gcc process_data.c -o process_data
mv process_data ../../process_data
cd ..