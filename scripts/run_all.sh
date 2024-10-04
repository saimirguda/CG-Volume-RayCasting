#!/bin/bash

TESTS="configs/*.json"; 
echo "Running tests ...";
for i in $TESTS
do 
    mkdir -p output/student/"$(basename "$i" .json)"
    xvfb-run -s "-screen 0 1024x768x24" build/bin/bin/task2 --configfile "$i" --output output/student/"$(basename "$i" .json)"/"$(basename "$i" .json)" || true
    echo "Ran task ${i##*/}";
    sleep 1;
done
