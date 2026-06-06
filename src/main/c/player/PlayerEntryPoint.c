#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "runtime/StoryLoader.h"
#include "runtime/Engine.h"

static Color _parseColor(const char * hex) {
	Color fallback = YELLOW;
	if (hex == NULL || hex[0] != '#') return fallback;
	int len = (int) strlen(hex);
	if (len != 7) return fallback;
	unsigned int r = 0, g = 0, b = 0;
	if (sscanf(hex + 1, "%2x%2x%2x", &r, &g, &b) == 3) {
		return (Color){ (unsigned char) r, (unsigned char) g, (unsigned char) b, 255 };
	}
	return fallback;
}

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
	InitAudioDevice();
	SetTargetFPS(60);

	Engine * engine = Engine_create(story);
	if (engine == NULL) {
		StoryLoader_destroy(story);
		CloseWindow();
		return 1;
	}

	int saveNotifyTimer = 0;
	const char * saveNotifyText = NULL;
	const char * savePath = ".build/save.json";

	while (!WindowShouldClose() && !Engine_isFinished(engine)) {
		Engine_update(engine);
		Engine_updateAudio(engine);

		/* Input handling */
		if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			if (Engine_isWaitingForInput(engine) && Engine_getChoiceCount(engine) == 0) {
				Engine_advance(engine);
			}
		}

		/* Choice selection via keyboard numbers and mouse */
		int choiceCount = Engine_getChoiceCount(engine);
		if (Engine_isWaitingForInput(engine) && choiceCount > 0) {
			for (int i = 0; i < choiceCount && i < 9; i++) {
				if (IsKeyPressed(KEY_ONE + i)) {
					Engine_selectChoice(engine, i);
					break;
				}
			}
			Vector2 mouse = GetMousePosition();
			int boxY = screenHeight - 40 - choiceCount * 40;
			for (int i = 0; i < choiceCount; i++) {
				Rectangle choiceRect = { 60.0f, (float)(boxY + i * 40), (float)(screenWidth - 120), 32.0f };
				if (CheckCollisionPointRec(mouse, choiceRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
					Engine_selectChoice(engine, i);
					break;
				}
			}
		}

		/* Save / Load */
		if (IsKeyPressed(KEY_F5)) {
			if (Engine_saveState(engine, savePath)) {
				saveNotifyText = "Game saved.";
				saveNotifyTimer = 120;
			} else {
				saveNotifyText = "Save failed.";
				saveNotifyTimer = 120;
			}
		}
		if (IsKeyPressed(KEY_F9)) {
			if (Engine_loadState(engine, savePath)) {
				saveNotifyText = "Game loaded.";
				saveNotifyTimer = 120;
			} else {
				saveNotifyText = "Load failed / mismatch.";
				saveNotifyTimer = 120;
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
		const char * actorId = Engine_getCurrentDialogueActor(engine);
		const char * dialogue = Engine_getCurrentDialogueText(engine);
		if (dialogue != NULL) {
			int boxY = screenHeight - 180;
			DrawRectangle(20, boxY, screenWidth - 40, 160, (Color){0, 0, 0, 200});
			if (actorId != NULL) {
				Actor * actorInfo = StoryLoader_findActor(Engine_getStory(engine), actorId);
				const char * displayName = (actorInfo != NULL && actorInfo->name != NULL) ? actorInfo->name : actorId;
				Color nameColor = (actorInfo != NULL && actorInfo->color != NULL) ? _parseColor(actorInfo->color) : YELLOW;
				DrawText(displayName, 40, boxY + 10, 24, nameColor);
			}
			_drawWrappedText(dialogue, 40, boxY + 45, 20, screenWidth - 80, WHITE);
			DrawText("[SPACE / Click to continue]", 40, boxY + 140, 16, LIGHTGRAY);
		}

		/* Choice menu */
		if (choiceCount > 0) {
			int boxY = screenHeight - 40 - choiceCount * 40;
			Vector2 mouse = GetMousePosition();
			for (int i = 0; i < choiceCount; i++) {
				const char * text = Engine_getChoiceText(engine, i);
				if (text == NULL) continue;
				Rectangle choiceRect = { 60.0f, (float)(boxY + i * 40), (float)(screenWidth - 120), 32.0f };
				bool hovered = CheckCollisionPointRec(mouse, choiceRect);
				Color rectColor = hovered ? SKYBLUE : DARKBLUE;
				Color textColor = hovered ? BLACK : WHITE;
				DrawRectangleRec(choiceRect, rectColor);
				DrawText(TextFormat("%d. %s", i + 1, text), 70, boxY + i * 40 + 6, 20, textColor);
			}
		}

		/* Save/Load notification */
		if (saveNotifyTimer > 0) {
			DrawText(saveNotifyText, 20, 20, 20, GREEN);
			saveNotifyTimer--;
		}

		EndDrawing();
	}

	Engine_destroy(engine);
	CloseAudioDevice();
	CloseWindow();
	StoryLoader_destroy(story);
	return 0;
}
