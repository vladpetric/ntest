# Build ntest for the local machine

set -eu

if ! [ -e src ]
  then
    echo "missing source directory. Run this from within the ntest directory."
    return 1
fi

echo "---------------"
echo "Compiling ntest"
echo "---------------"

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../src
make
make test
cd ..  # person who ran build.sh expects its working directory here

echo
echo "---------------"
echo "BUILD SUCCEEDED"
echo "---------------"

