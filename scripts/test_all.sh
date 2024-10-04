#!/bin/bash

for subfolder in "output/student"/*; do
    parent_dir=$(basename "$subfolder")
    mkdir -p output/diffs/$parent_dir
    for file in "$subfolder"/*; do
        compare -compose src $file "output/reference/$parent_dir/${file##*/}" "output/diffs/$parent_dir/${file##*/}"
    done
    echo "Output diffed ${subfolder##*/}"
done