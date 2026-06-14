#! /bin/bash

set -euxo pipefail

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
OFF='\033[0m'

rm --force --recursive ".build"
rm --force "src/main/c/frontend/lexical-analysis/FlexScanner.c"
rm --force "src/main/c/frontend/lexical-analysis/FlexScanner.h"
rm --force "src/main/c/frontend/syntactic-analysis/BisonParser.c"
rm --force "src/main/c/frontend/syntactic-analysis/BisonParser.h"

RAYLIB_SRC_FLAG=""
if [ -d "/opt/raylib" ]; then
	RAYLIB_SRC_FLAG="-DFETCHCONTENT_SOURCE_DIR_RAYLIB=/opt/raylib"
fi
set +e
cmake -S . -B .build $RAYLIB_SRC_FLAG
CMAKE_EXIT=$?
set -e
if [ $CMAKE_EXIT -ne 0 ]; then
	echo "=== CMake failed. Dumping error logs: ==="
	find .build -name "CMakeError.log" | while read f; do
		echo "--- $f ---"
		cat "$f"
	done
	find .build -name "CMakeOutput.log" | while read f; do
		echo "--- $f ---"
		tail -50 "$f"
	done
	exit $CMAKE_EXIT
fi
echo -e "${GREEN}CMake done.${OFF}"
cd .build
make
cd ..

echo -e "${GREEN}All done.${OFF}"
