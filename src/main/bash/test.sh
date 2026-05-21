#! /bin/bash

set -u

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
RED='\033[0;31m'
OFF='\033[0m'
STATUS=0

echo "Compiler should accept..."
echo ""

for test in $(ls src/test/c/accept/); do
	cat "src/test/c/accept/$test" | ".build/Flex-Bison-Compiler" >/dev/null 2>&1
	RESULT="$?"
	if [ "$RESULT" == "0" ]; then
		echo -e "    $test, ${GREEN}and it does${OFF} (status $RESULT)"
	else
		STATUS=1
		echo -e "    $test, ${RED}but it rejects${OFF} (status $RESULT)"
	fi
done
echo ""

echo "Compiler should reject..."
echo ""

for test in $(ls src/test/c/reject/); do
	cat "src/test/c/reject/$test" | ".build/Flex-Bison-Compiler" >/dev/null 2>&1
	RESULT="$?"
	if [ "$RESULT" != "0" ]; then
		echo -e "    $test, ${GREEN}and it does${OFF} (status $RESULT)"
	else
		STATUS=1
		echo -e "    $test, ${RED}but it accepts${OFF} (status $RESULT)"
	fi
done
echo ""

echo "JSON output should exist..."
echo ""

JSON_TEST_FILE="src/test/c/accept/01-linear-scene"
cat "$JSON_TEST_FILE" | ".build/Flex-Bison-Compiler" "$JSON_TEST_FILE" >/dev/null 2>&1
if [ -s ".build/story.json" ]; then
	echo -e "    story.json, ${GREEN}generated${OFF}"
else
	STATUS=1
	echo -e "    story.json, ${RED}missing${OFF}"
fi
echo ""

echo "All done."
exit $STATUS
