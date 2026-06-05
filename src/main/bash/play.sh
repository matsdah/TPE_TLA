#! /bin/bash

set -euo pipefail

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

STORY_JSON="${1:-.build/story.json}"

if [ ! -f "$STORY_JSON" ]; then
	echo "Error: Story JSON not found at '$STORY_JSON'."
	echo "Run: src/main/bash/run.sh <program-file>"
	exit 1
fi

"./.build/Flex-Bison-Player" "$STORY_JSON"
