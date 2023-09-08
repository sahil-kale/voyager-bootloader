#!/bin/bash

# Find all C, C++, and header files in the current directory
files=$(find . -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp")

# Set a flag to indicate if any files need formatting
need_formatting=0

# Loop through the files and check if they need formatting
for file in $files
do
  # Check if the file needs formatting
  if ! clang-format "$file" | diff "$file" - >/dev/null
  then
    # The file needs formatting, so set the flag and print an error message
    need_formatting=1
    echo "Error: $file needs formatting"
  fi
done

# If any files needed formatting, exit with an error code
if [ "$need_formatting" -eq 1 ]
then
  exit 1
fi
