BUILD_DIR="build"
BIN_DIR="."
BIN_FILE="lab3"

mkdir $BUILD_DIR
cd $BUILD_DIR
cmake ..
make
cd ..
[ ! -d $BIN_DIR ] && mkdir $BIN_DIR
cp $BUILD_DIR/$BIN_FILE $BIN_DIR
rm -rf $BUILD_DIR