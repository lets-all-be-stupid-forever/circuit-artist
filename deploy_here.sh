# Script to creates a release package
# Usage: 
#   (i) Create a folder for the release and go to it.
#   (ii) run bash ../deploy_here.sh
set -e
mkdir bin
cp ../build/Release/* bin/
cp -r ../luasrc/ .
cp -r ../assets/ .
cp ../LuaJIT/src/lua51.* bin/
