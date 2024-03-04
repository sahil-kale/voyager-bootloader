import subprocess
import pathlib

# make a new directory with -p for cppcheck
subprocess.call(["mkdir", "-p", "cppcheckbuild"])

# find all C++ files in the src directory and .hpp files in the include directory
cpp_files = list(pathlib.Path("src").rglob("*.c"))
hpp_files = list(pathlib.Path("inc").rglob("*.h"))


# Find the directory of the hpp files
hpp_dirs = set([file.parent for file in hpp_files])
# convert the set to a list
hpp_dirs = list(hpp_dirs)
# prepend the -I flag to each directory
hpp_dirs = [f"-I{dir}" for dir in hpp_dirs]

cppcheck_args = ["--enable=all", "--inconclusive", "--force", "--std=c++11", "--language=c++", "--template=gcc", "--verbose", "--error-exitcode=1", "-q", "-DVOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE=1024"]

suppressions = ["unusedFunction", "missingInclude", "functionStatic", "unmatchedSuppression", "unusedStructMember", "preprocessorErrorDirective", "ConfigurationNotChecked"]

# add suppressions to the cppcheck_args
for suppression in suppressions:
    cppcheck_args.append(f"--suppress={suppression}")

cppcheck_cmd = ["cppcheck", *cppcheck_args, *cpp_files, *hpp_files, *hpp_dirs]

ret = subprocess.call(cppcheck_cmd)
if ret != 0:
    print("cppcheck failed")
    exit(ret)
