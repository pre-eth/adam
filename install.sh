#!/bin/bash

arch=$(uname -m)
if [[ $arch -ne x86_64 ]]
then
	echo "Architecture not supported. Exiting installation..."
	return 1
fi

echo "Welcome to the installation for ADAM, a cryptographically secure pseudorandom number generator."
echo "Generating seed bytes from Genesis..."

g++ -o gen gen.cc -O3 -march=native
./gen > chunk

echo "INSTALLING..."

mkdir build
cd build
cmake ..
make

echo "Done. Make sure the adam binary is in your path and you're all good to go!"