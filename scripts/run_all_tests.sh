#!/bin/bash

mkdir -p build/bin && cd build/bin
cmake ../cmake
make -j
cd ../../

echo "Creating diffs..."
mkdir -p output && mkdir -p output/diffs && mkdir -p output/student
echo "------------------Testing all cases------------------"
TESTS="configs/*.json"; for i in $TESTS; do mkdir -p output/student/"$(basename "$i" .json)"; build/bin/bin/task2 --configfile "$i" --output output/student/"$(basename "$i" .json)"/"$(basename "$i" .json)" || true; echo "Ran task ${i##*/}"; done

for subfolder in "output/student"/*; do
    parent_dir=$(basename "$subfolder")
    mkdir -p output/diffs/$parent_dir
    for file in "$subfolder"/*; do
        compare -compose src $file "output/reference/$parent_dir/${file##*/}" "output/diffs/$parent_dir/${file##*/}"
    done
    echo "Output diffed ${subfolder##*/}"
done