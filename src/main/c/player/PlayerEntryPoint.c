#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "runtime/StoryLoader.h"
#include "runtime/Engine.h"

static void _drawWrappedText(const char * text, int x, int y, int fontSize, int maxWidth, Color color) {
	/* Naive word-wrap for Raylib's DrawText */
	if (text == NULL) return;
	const char * word = text;
	int cursorX = x;
	int cursorY = y;
	while (*word != '\0') {
		const char * end = word;
		while (*end != '\0' && *end != ' ') end++;
		int len = (int)(end - word);
		int wordWidth = MeasureText(word, fontSize);
		if (cursorX + wordWidth > x + maxWidth && cursorX != x) {
			cursorX = x;
			cursorY += fontSize + 4;
		}
		DrawText(word, cursorX, cursorY, fontSize, color);
		cursorX += wordWidth + MeasureText(" ", fontSize);
		if (*end == ' ') end++;
		word = end;
	}
}

const int main(const int length, const char ** arguments) {
	if (length < 2) {
		printf("Usage: Flex-Bison-Player <story.json>\n");
		return 1;
	}

	const char * jsonPath = arguments[1];
	Story * story = StoryLoader_load(jsonPath);
	if (story == NULL) {
		fprintf(stderr, "Failed to load story from %s\n", jsonPath);
		return 1;
	}

	/* Resolve asset paths relative to story.json directory */
	char * pathCopy = (char *) malloc(strlen(jsonPath) + 1);
	strcpy(pathCopy, jsonPath);
	char * dir = dirname(pathCopy);
	StoryLoader_resolveAssetPaths(story, dir);
	free(pathCopy);

	const int screenWidth = 800;
	const int screenHeight = 600;
	InitWindow(screenWidth, screenHeight, "Flex-Bison-Player");
	SetTargetFPS(60);

	Engine * engine = Engine_create(story);
	if (engine == NULL) {
		StoryLoader_destroy(story);
		CloseWindow();
		return 1;
	}

	while (!WindowShouldClose() && !Engine_isFinished(engine)) {
		Engine_update(engine);
		Engine_updateAudio(engine);

		/* Input handling */
		if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			if (Engine_isWaitingForInput(engine) && Engine_getChoiceCount(engine) == 0) {
				Engine_advance(engine);
			}
		}

		/* Choice selection via keyboard numbers */
		if (Engine_isWaitingForInput(engine) && Engine_getChoiceCount(engine) > 0) {
			for (int i = 0; i < Engine_getChoiceCount(engine) && i < 9; i++) {
				if (IsKeyPressed(KEY_ONE + i)) {
					Engine_selectChoice(engine, i);
					break;
				}
			}
		}

		BeginDrawing();
		ClearBackground(BLACK);

		/* Background rendering */
		Texture2D bg = Engine_getBackgroundTexture(engine);
		if (bg.id != 0) {
			DrawTexturePro(bg,
				(Rectangle){0, 0, (float)bg.width, (float)bg.height},
				(Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
				(Vector2){0, 0}, 0.0f, WHITE);
		} else if (Engine_getCurrentScene(engine) != NULL) {
			DrawRectangle(0, 0, screenWidth, screenHeight, DARKGRAY);
		}

		/* Sprite rendering */
		Texture2D spr = Engine_getSpriteTexture(engine);
		if (spr.id != 0) {
			int x = (screenWidth - spr.width) / 2;
			int y = (screenHeight - spr.height) / 2 - 50;
			DrawTexture(spr, x, y, WHITE);
		}

		/* Dialogue box */
		const char * actor = Engine_getCurrentDialogueActor(engine);
		const char * dialogue = Engine_getCurrentDialogueText(engine);
		if (dialogue != NULL) {
			int boxY = screenHeight - 180;
			DrawRectangle(20, boxY, screenWidth - 40, 160, (Color){0, 0, 0, 200});
			if (actor != NULL) {
				DrawText(actor, 40, boxY + 10, 24, YELLOW);
			}
			_drawWrappedText(dialogue, 40, boxY + 45, 20, screenWidth - 80, WHITE);
			DrawText("[SPACE / Click to continue]", 40, boxY + 140, 16, LIGHTGRAY);
		}

		/* Choice menu */
		int choiceCount = Engine_getChoiceCount(engine);
		if (choiceCount > 0) {
			int boxY = screenHeight - 40 - choiceCount * 40;
			for (int i = 0; i < choiceCount; i++) {
				const char * text = Engine_getChoiceText(engine, i);
				if (text == NULL) continue;
				DrawRectangle(60, boxY + i * 40, screenWidth - 120, 32, DARKBLUE);
				DrawText(TextFormat("%d. %s", i + 1, text), 70, boxY + i * 40 + 6, 20, WHITE);
			}
		}

		EndDrawing();
	}

	CloseWindow();
	Engine_destroy(engine);
	StoryLoader_destroy(story);
	return 0;
}
