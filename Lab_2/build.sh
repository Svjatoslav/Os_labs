mkdir build
cd build
cmake -S ../ -B ./
make

mv client* ../
mv host* ../

cd ../
rm -r build
