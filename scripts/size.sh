#!/bin/bash

args=("$@")
size=$(ls -lh ${args[0]} | grep -oE '[0-9]+[KMB]');
printf "\033[1;32mFinished!\033[m (\033[1m%sB\033[m)\n" "$size"