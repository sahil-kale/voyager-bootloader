#!/bin/bash

# find all C, C++, and header files in the current directory and subdirectories
files=$(find . -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp")

# Loop through the files and format them with clang-format
for file in $files
do
  clang-format -i "$file"
done
