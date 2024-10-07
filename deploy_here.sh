# Script to creates a release package
# Usage: 
#   (i) Create a folder for the release and go to it.
#   (ii) run bash ../deploy_here.sh
set -e
mkdir bin
cp ../build/Release/* bin/
mkdir luasrc
cp  ../luasrc/*.lua luasrc/
cp -r ../luasrc/component_examples  luasrc/
cp -r ../luasrc/tutorial                luasrc/
cp -r ../luasrc/imgs                luasrc/
cp -r ../luasrc/levels              luasrc/
cp -r ../luasrc/template_scripts    luasrc/
cp -r ../assets/ .
cp ../LuaJIT/src/lua51.* bin/
cp ../build/steam_api64.dll bin/
