#!/bin/bash

mkdir -p ../build
cd ../build
cmake ..
make || exit

cp main ../run

echo "#!/bin/bash
cd ../run
./main" > ../src/main

chmod +x ../src/main

cd ../src
find . -type f |grep -v Doxyfile |parallel wc -l |awk '{sum+=$1;} END{print "compiled "sum" LOC";}'
