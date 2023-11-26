mkdir -p cppcheckbuild

# Find all C, C++, and header files in the current directory
files=$(find . -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp")

# Remove any files located in build/
files=$(echo "$files" | grep -v "build/")

# Store a variable to hold the suppressions
suppressions="--suppress=missingIncludeSystem --suppress=unusedFunction --suppress=missingInclude --suppress=unmatchedSuppression"

# Of the files glob, find only the hpp files
header_files=$(echo "$files" | grep -E "\.h$")

# Find the directories of the hpp files. Prepend the -I flag to each directory
header_files=$(echo "$header_files" | xargs -n1 dirname | sort | uniq | sed -e 's/^/-I/')

# Add compiler args
compiler_args="-DVOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE=64"
base_args="--enable=all --template=gcc --error-exitcode=1 --cppcheck-build-dir=cppcheckbuild -q --inline-suppr"

cppcheck $files $suppressions $base_args $compiler_args $header_files

# Check the exit code of cppcheck
if [ $? -eq 0 ]; then
  echo "Cppcheck was successful"
else
  echo "Cppcheck encountered errors or warnings"
  exit 1
fi