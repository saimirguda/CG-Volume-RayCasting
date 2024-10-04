mkdir -p build/bin && cd build/bin
cmake ../cmake
make -j
cd ../../

echo "Running test..."
mkdir -p output && mkdir -p output/diffs && mkdir -p output/student

test="cross"
build/bin/bin/task2 --configfile configs/$test.json --output output/student/$test/$test

mkdir -p output/diffs/$test
for file in "output/student/$test"/*; do
    compare -compose src $file "output/reference/$test/${file##*/}" "output/diffs/$test/${file##*/}"
    echo "Output diffed $file"   
done

