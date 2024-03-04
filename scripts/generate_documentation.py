import subprocess

# First, remove the html directory
subprocess.call(["rm", "-r", "docs"])

# Then, generate the documentation
ret = subprocess.call(["doxygen", "Doxyfile"])
if ret != 0:
    print("doxygen failed")
    exit(ret)