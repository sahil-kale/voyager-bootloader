#!/bin/bash

set -euxo pipefail

make clean
make

# Run the tests
./build/run_tests