#!/bin/bash

cd src
doxygen || exit

read -p "press any key to continue with latex generation"

cd ../documentation/latex
make || exit

rm -f ../documentation.pdf 
cp refman.pdf ../documentation.pdf

make clean

cd ..

[ -f documentation.html ] || ln -s html/index.html documentation.html

cd ../src
echo ""
find . -type f |grep -v Doxyfile |parallel wc -l |awk '{sum+=$1;} END{print "done: build documentation for "sum" LOC.";}'

