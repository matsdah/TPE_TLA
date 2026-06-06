#ifndef ENGINE_HEADER
#define ENGINE_HEADER

#include "StoryLoader.h"
#include <raylib.h>
#include <stdbool.h>

/* Context stack depth limit */
#define ENGINE_MAX_CONTEXTS 64

typedef struct Engine Engine;

Engine * Engine_create(Story * story);
void Engine_destroy(Engine * engine);

void Engine_update(Engine * engine);
void Engine_advance(Engine * engine);
void Engine_selectChoice(Engine * engine, int optionIndex);

bool Engine_isFinished(const Engine * engine);
bool Engine_isWaitingForInput(const Engine * engine);

const char * Engine_getCurrentDialogueActor(const Engine * engine);
const char * Engine_getCurrentDialogueText(const Engine * engine);

int Engine_getChoiceCount(const Engine * engine);
const char * Engine_getChoiceText(const Engine * engine, int index);

Scene * Engine_getCurrentScene(const Engine * engine);
Story * Engine_getStory(const Engine * engine);

/* Media state */
Texture2D Engine_getBackgroundTexture(const Engine * engine);
Texture2D Engine_getSpriteTexture(const Engine * engine);
void Engine_updateAudio(Engine * engine);

bool Engine_saveState(Engine * engine, const char * savePath);
bool Engine_loadState(Engine * engine, const char * savePath);

#endif
