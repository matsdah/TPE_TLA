#include "Engine.h"
#include "ExpressionEvaluator.h"
#include "../vendor/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Helpers ─────────────────────────────────────────────────────────── */

static char * _strdup(const char * src) {
	if (src == NULL) return NULL;
	size_t len = strlen(src) + 1;
	char * dst = (char *) malloc(len);
	if (dst != NULL) memcpy(dst, src, len);
	return dst;
}

/* ── Internal context stack ─────────────────────────────────────────── */

typedef struct {
	StatementList * list;
	Statement * current;
} Context;

struct Engine {
	Story * story;
	Context stack[ENGINE_MAX_CONTEXTS];
	int stackTop; /* -1 when empty */
	bool waitingForInput;
	bool finished;
	/* Cached state for rendering */
	const char * currentActorId;
	const char * currentDialogueText;
	char * savedActorId;
	char * savedDialogueText;
	ChoiceOption * currentChoices;
	/* Renderer / audio state */
	char * shownBackground;
	char * shownSprite;
	char * playingMusic;
	char * playingSound;
	Texture2D bgTexture;
	Texture2D spriteTexture;
	Music currentMusic;
	Sound currentSound;
	bool bgTextureLoaded;
	bool spriteTextureLoaded;
	bool musicPlaying;
	bool soundPlaying;
};

/* ── Helpers ────────────────────────────────────────────────────────── */

static Variable * _findVariable(Engine * engine, const char * name) {
	for (Variable * v = engine->story->variables; v != NULL; v = v->next) {
		if (strcmp(v->name, name) == 0) return v;
	}
	return NULL;
}

static void _setVariable(Engine * engine, const char * name, int value) {
	Variable * v = _findVariable(engine, name);
	if (v != NULL) {
		v->value = value;
		return;
	}
	v = (Variable *) calloc(1, sizeof(Variable));
	v->name = (char *) malloc(strlen(name) + 1);
	strcpy(v->name, name);
	v->value = value;
	v->next = engine->story->variables;
	engine->story->variables = v;
}

static void _executeSet(Engine * engine, const char * identifier, const char * op, Expression * expr) {
	int current = 0;
	Variable * v = _findVariable(engine, identifier);
	if (v != NULL) current = v->value;
	int value = ExpressionEvaluator_evaluate(engine->story, expr);
	if (strcmp(op, "=") == 0) _setVariable(engine, identifier, value);
	else if (strcmp(op, "+=") == 0) _setVariable(engine, identifier, current + value);
	else if (strcmp(op, "-=") == 0) _setVariable(engine, identifier, current - value);
}

static void _pushContext(Engine * engine, StatementList * list) {
	if (engine->stackTop + 1 >= ENGINE_MAX_CONTEXTS) {
		fprintf(stderr, "Engine context stack overflow\n");
		engine->finished = true;
		return;
	}
	engine->stackTop++;
	engine->stack[engine->stackTop].list = list;
	engine->stack[engine->stackTop].current = (list != NULL) ? list->head : NULL;
}

static void _popContext(Engine * engine) {
	if (engine->stackTop >= 0) {
		engine->stackTop--;
	}
}

static Context * _topContext(Engine * engine) {
	if (engine->stackTop < 0) return NULL;
	return &engine->stack[engine->stackTop];
}

static void _clearWaiting(Engine * engine) {
	engine->waitingForInput = false;
	engine->currentActorId = NULL;
	engine->currentDialogueText = NULL;
	free(engine->savedActorId); engine->savedActorId = NULL;
	free(engine->savedDialogueText); engine->savedDialogueText = NULL;
	engine->currentChoices = NULL;
}

static void _stepNext(Engine * engine);

static const char * _resolveAssetPath(Engine * engine, const char * resourceId) {
	for (Asset * a = engine->story->assets; a != NULL; a = a->next) {
		if (strcmp(a->id, resourceId) == 0) return a->path;
	}
	return NULL;
}

/* ── Media helpers (reusable by stepStatement and loadState) ─────────── */

static void _applyShowBackground(Engine * engine, const char * resourceId) {
	free(engine->shownBackground);
	engine->shownBackground = _strdup(resourceId);
	if (engine->bgTextureLoaded) {
		UnloadTexture(engine->bgTexture);
		engine->bgTextureLoaded = false;
	}
	if (resourceId != NULL) {
		const char * path = _resolveAssetPath(engine, resourceId);
		if (path != NULL) {
			engine->bgTexture = LoadTexture(path);
			if (engine->bgTexture.id != 0) {
				engine->bgTextureLoaded = true;
			} else {
				fprintf(stderr, "Engine: failed to load background texture '%s'\n", path);
			}
		} else {
			fprintf(stderr, "Engine: asset '%s' not found for background show\n", resourceId);
		}
	}
}

static void _applyShowSprite(Engine * engine, const char * resourceId) {
	free(engine->shownSprite);
	engine->shownSprite = _strdup(resourceId);
	if (engine->spriteTextureLoaded) {
		UnloadTexture(engine->spriteTexture);
		engine->spriteTextureLoaded = false;
	}
	if (resourceId != NULL) {
		const char * path = _resolveAssetPath(engine, resourceId);
		if (path != NULL) {
			engine->spriteTexture = LoadTexture(path);
			if (engine->spriteTexture.id != 0) {
				engine->spriteTextureLoaded = true;
			} else {
				fprintf(stderr, "Engine: failed to load sprite texture '%s'\n", path);
			}
		} else {
			fprintf(stderr, "Engine: asset '%s' not found for sprite show\n", resourceId);
		}
	}
}

static void _applyHideBackground(Engine * engine) {
	free(engine->shownBackground); engine->shownBackground = NULL;
	if (engine->bgTextureLoaded) {
		UnloadTexture(engine->bgTexture);
		engine->bgTextureLoaded = false;
	}
}

static void _applyHideSprite(Engine * engine) {
	free(engine->shownSprite); engine->shownSprite = NULL;
	if (engine->spriteTextureLoaded) {
		UnloadTexture(engine->spriteTexture);
		engine->spriteTextureLoaded = false;
	}
}

static void _applyPlayMusic(Engine * engine, const char * resourceId) {
	free(engine->playingMusic);
	engine->playingMusic = _strdup(resourceId);
	if (engine->musicPlaying) {
		StopMusicStream(engine->currentMusic);
		UnloadMusicStream(engine->currentMusic);
		engine->musicPlaying = false;
	}
	if (resourceId != NULL) {
		const char * path = _resolveAssetPath(engine, resourceId);
		if (path != NULL && IsAudioDeviceReady()) {
			Music loaded = LoadMusicStream(path);
			if (loaded.stream.buffer != NULL) {
				engine->currentMusic = loaded;
				PlayMusicStream(engine->currentMusic);
				engine->musicPlaying = true;
			} else {
				fprintf(stderr, "Engine: failed to load music '%s'\n", path);
				UnloadMusicStream(loaded);
			}
		} else if (path != NULL) {
			fprintf(stderr, "Engine: audio device not available, skipping music '%s'\n", path);
		} else {
			fprintf(stderr, "Engine: asset '%s' not found for music play\n", resourceId);
		}
	}
}

static void _applyPlaySound(Engine * engine, const char * resourceId) {
	free(engine->playingSound);
	engine->playingSound = _strdup(resourceId);
	if (engine->soundPlaying) {
		UnloadSound(engine->currentSound);
		engine->soundPlaying = false;
	}
	if (resourceId != NULL) {
		const char * path = _resolveAssetPath(engine, resourceId);
		if (path != NULL && IsAudioDeviceReady()) {
			Sound loaded = LoadSound(path);
			if (loaded.stream.buffer != NULL) {
				engine->currentSound = loaded;
				PlaySound(engine->currentSound);
				engine->soundPlaying = true;
			} else {
				fprintf(stderr, "Engine: failed to load sound '%s'\n", path);
				UnloadSound(loaded);
			}
		} else if (path != NULL) {
			fprintf(stderr, "Engine: audio device not available, skipping sound '%s'\n", path);
		} else {
			fprintf(stderr, "Engine: asset '%s' not found for sound play\n", resourceId);
		}
	}
}

static void _applyStopMusic(Engine * engine) {
	free(engine->playingMusic); engine->playingMusic = NULL;
	if (engine->musicPlaying) {
		StopMusicStream(engine->currentMusic);
		UnloadMusicStream(engine->currentMusic);
		engine->musicPlaying = false;
	}
}

static void _applyStopSound(Engine * engine) {
	free(engine->playingSound); engine->playingSound = NULL;
	if (engine->soundPlaying) {
		UnloadSound(engine->currentSound);
		engine->soundPlaying = false;
	}
}

/* ── Core stepping logic ────────────────────────────────────────────── */

static void _stepStatement(Engine * engine, Statement * stmt) {
	if (stmt == NULL) return;
	switch (stmt->kind) {
		case STMT_DIALOGUE:
			free(engine->savedActorId); engine->savedActorId = NULL;
			free(engine->savedDialogueText); engine->savedDialogueText = NULL;
			engine->currentActorId = stmt->dialogue.actorId;
			engine->currentDialogueText = stmt->dialogue.text;
			engine->waitingForInput = true;
			break;

		case STMT_SHOW:
			if (strcmp(stmt->show.target, "background") == 0) {
				_applyShowBackground(engine, stmt->show.resourceId);
			} else {
				_applyShowSprite(engine, stmt->show.resourceId);
			}
			_stepNext(engine);
			break;

		case STMT_HIDE:
			if (strcmp(stmt->hide.target, "background") == 0) {
				_applyHideBackground(engine);
			} else {
				_applyHideSprite(engine);
			}
			_stepNext(engine);
			break;

		case STMT_PLAY:
			if (strcmp(stmt->play.target, "music") == 0) {
				_applyPlayMusic(engine, stmt->play.resourceId);
			} else {
				_applyPlaySound(engine, stmt->play.resourceId);
			}
			_stepNext(engine);
			break;

		case STMT_STOP:
			if (strcmp(stmt->stop.target, "music") == 0) {
				_applyStopMusic(engine);
			} else {
				_applyStopSound(engine);
			}
			_stepNext(engine);
			break;

		case STMT_GOTO:
		{
			Scene * target = StoryLoader_findScene(engine->story, stmt->goto_.sceneName);
			engine->stackTop = -1;
			if (target != NULL) {
				_pushContext(engine, target->statements);
				_stepNext(engine);
			} else {
				fprintf(stderr, "Engine: goto target scene '%s' not found\n", stmt->goto_.sceneName);
				engine->finished = true;
			}
			break;
		}

		case STMT_SET:
			_executeSet(engine, stmt->set.identifier, stmt->set.op, stmt->set.expr);
			_stepNext(engine);
			break;

		case STMT_CHOICE:
			engine->currentChoices = stmt->choice.options;
			engine->waitingForInput = true;
			break;

		case STMT_IF:
		{
			bool result = ExpressionEvaluator_test(engine->story, stmt->if_.condition);
			StatementList * block = result ? stmt->if_.thenBlock : stmt->if_.elseBlock;
			/* Only push the block if it actually has statements to run. */
			if (block != NULL && block->head != NULL) {
				_pushContext(engine, block);
			}
			/* Always advance: if a block was pushed this executes its first
			 * statement; otherwise it continues in the current context. */
			_stepNext(engine);
			break;
		}

		case STMT_END:
			engine->stackTop = -1;
			engine->finished = true;
			break;
	}
}

static void _stepNext(Engine * engine) {
	if (engine->finished) return;
	Context * ctx = _topContext(engine);
	if (ctx == NULL) {
		engine->finished = true;
		return;
	}
	if (ctx->current == NULL) {
		_popContext(engine);
		if (engine->stackTop < 0) {
			engine->finished = true;
			return;
		}
		_stepNext(engine);
		return;
	}
	Statement * stmt = ctx->current;
	ctx->current = ctx->current->next;
	_stepStatement(engine, stmt);
}

/* ── Public API ────────────────────────────────────────────────────── */

Engine * Engine_create(Story * story) {
	Engine * engine = (Engine *) calloc(1, sizeof(Engine));
	engine->story = story;
	engine->stackTop = -1;
	engine->waitingForInput = false;
	engine->finished = false;

	/* Run global declarations (top-level sets) */
	if (story->declarations != NULL) {
		for (Statement * stmt = story->declarations->head; stmt != NULL; stmt = stmt->next) {
			if (stmt->kind == STMT_SET) {
				_executeSet(engine, stmt->set.identifier, stmt->set.op, stmt->set.expr);
			}
		}
	}

	/* Push first scene */
	if (story->scenes != NULL && story->scenes->head != NULL) {
		_pushContext(engine, story->scenes->head->statements);
		_stepNext(engine);
	} else {
		engine->finished = true;
	}

	return engine;
}

void Engine_destroy(Engine * engine) {
	if (engine == NULL) return;
	if (engine->bgTextureLoaded) UnloadTexture(engine->bgTexture);
	if (engine->spriteTextureLoaded) UnloadTexture(engine->spriteTexture);
	if (engine->musicPlaying) {
		StopMusicStream(engine->currentMusic);
		UnloadMusicStream(engine->currentMusic);
	}
	if (engine->soundPlaying) {
		UnloadSound(engine->currentSound);
	}
	free(engine->shownBackground);
	free(engine->shownSprite);
	free(engine->playingMusic);
	free(engine->playingSound);
	free(engine->savedActorId);
	free(engine->savedDialogueText);
	free(engine);
}

void Engine_update(Engine * engine) {
	/* In Phase 2 the update is purely event-driven.
	 * Instant effects have already been applied during stepNext.
	 * This hook is reserved for future real-time animations. */
	(void) engine;
}

void Engine_advance(Engine * engine) {
	if (engine == NULL || engine->finished) return;
	if (!engine->waitingForInput) return;
	_clearWaiting(engine);
	_stepNext(engine);
}

void Engine_selectChoice(Engine * engine, int optionIndex) {
	if (engine == NULL || engine->finished) return;
	if (!engine->waitingForInput) return;
	if (engine->currentChoices == NULL) return;
	ChoiceOption * opt = engine->currentChoices;
	int idx = 0;
	while (opt != NULL && idx < optionIndex) {
		opt = opt->next;
		idx++;
	}
	if (opt == NULL) return;
	_clearWaiting(engine);
	if (opt->body != NULL) {
		_pushContext(engine, opt->body);
	}
	_stepNext(engine);
}

bool Engine_isFinished(const Engine * engine) {
	return engine != NULL && engine->finished;
}

bool Engine_isWaitingForInput(const Engine * engine) {
	return engine != NULL && engine->waitingForInput;
}

const char * Engine_getCurrentDialogueActor(const Engine * engine) {
	if (engine == NULL) return NULL;
	return engine->currentActorId;
}

const char * Engine_getCurrentDialogueText(const Engine * engine) {
	if (engine == NULL) return NULL;
	return engine->currentDialogueText;
}

int Engine_getChoiceCount(const Engine * engine) {
	if (engine == NULL || engine->currentChoices == NULL) return 0;
	int count = 0;
	for (ChoiceOption * opt = engine->currentChoices; opt != NULL; opt = opt->next) count++;
	return count;
}

const char * Engine_getChoiceText(const Engine * engine, int index) {
	if (engine == NULL || engine->currentChoices == NULL) return NULL;
	ChoiceOption * opt = engine->currentChoices;
	int idx = 0;
	while (opt != NULL && idx < index) { opt = opt->next; idx++; }
	return opt != NULL ? opt->text : NULL;
}

Scene * Engine_getCurrentScene(const Engine * engine) {
	if (engine == NULL || engine->stackTop < 0) return NULL;
	/* Find which scene owns the current context's statement list */
	const Context * ctx = &engine->stack[engine->stackTop];
	for (Scene * s = engine->story->scenes->head; s != NULL; s = s->next) {
		if (s->statements == ctx->list) return s;
	}
	return NULL;
}

Story * Engine_getStory(const Engine * engine) {
	if (engine == NULL) return NULL;
	return engine->story;
}

/* ── Media getters ─────────────────────────────────────────────────── */

Texture2D Engine_getBackgroundTexture(const Engine * engine) {
	if (engine == NULL || !engine->bgTextureLoaded) return (Texture2D){0};
	return engine->bgTexture;
}

Texture2D Engine_getSpriteTexture(const Engine * engine) {
	if (engine == NULL || !engine->spriteTextureLoaded) return (Texture2D){0};
	return engine->spriteTexture;
}

void Engine_updateAudio(Engine * engine) {
	if (engine != NULL && engine->musicPlaying) {
		UpdateMusicStream(engine->currentMusic);
	}
}

/* ── Save / Load ────────────────────────────────────────────────────── */

bool Engine_saveState(Engine * engine, const char * savePath) {
	if (engine == NULL || savePath == NULL) return false;
	cJSON * root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "storySource", engine->story->source != NULL ? engine->story->source : "");
	cJSON_AddBoolToObject(root, "finished", engine->finished);
	cJSON_AddBoolToObject(root, "waitingForInput", engine->waitingForInput);

	/* variables */
	cJSON * vars = cJSON_CreateArray();
	for (Variable * v = engine->story->variables; v != NULL; v = v->next) {
		cJSON * obj = cJSON_CreateObject();
		cJSON_AddStringToObject(obj, "name", v->name);
		cJSON_AddNumberToObject(obj, "value", v->value);
		cJSON_AddItemToArray(vars, obj);
	}
	cJSON_AddItemToObject(root, "variables", vars);

	/* media */
	cJSON * media = cJSON_CreateObject();
	cJSON_AddStringToObject(media, "shownBackground", engine->shownBackground);
	cJSON_AddStringToObject(media, "shownSprite", engine->shownSprite);
	cJSON_AddStringToObject(media, "playingMusic", engine->playingMusic);
	cJSON_AddStringToObject(media, "playingSound", engine->playingSound);
	cJSON_AddItemToObject(root, "media", media);

	/* stack */
	cJSON * stack = cJSON_CreateArray();
	for (int i = 0; i <= engine->stackTop; i++) {
		cJSON * frame = cJSON_CreateObject();
		char * listPath = StoryLoader_encodeListPath(engine->story, engine->stack[i].list);
		cJSON_AddStringToObject(frame, "listPath", listPath != NULL ? listPath : "");
		free(listPath);
		int stmtIdx = StoryLoader_getStatementIndex(engine->stack[i].list, engine->stack[i].current);
		cJSON_AddNumberToObject(frame, "stmtIndex", stmtIdx);
		cJSON_AddItemToArray(stack, frame);
	}
	cJSON_AddItemToObject(root, "stack", stack);

	/* pending */
	if (engine->waitingForInput) {
		cJSON * pending = cJSON_CreateObject();
		if (engine->currentDialogueText != NULL) {
			cJSON_AddStringToObject(pending, "kind", "dialogue");
			cJSON_AddStringToObject(pending, "actorId", engine->currentActorId);
			cJSON_AddStringToObject(pending, "text", engine->currentDialogueText);
		} else if (engine->currentChoices != NULL) {
			Statement * choiceStmt = NULL;
			for (Scene * sc = engine->story->scenes->head; sc != NULL && choiceStmt == NULL; sc = sc->next) {
				for (Statement * st = sc->statements->head; st != NULL; st = st->next) {
					if (st->kind == STMT_CHOICE && st->choice.options == engine->currentChoices) {
						choiceStmt = st;
						break;
					}
				}
			}
			cJSON_AddStringToObject(pending, "kind", "choice");
			if (choiceStmt != NULL) {
				char * stmtPath = StoryLoader_encodeStatementPath(engine->story, choiceStmt);
				cJSON_AddStringToObject(pending, "statementPath", stmtPath != NULL ? stmtPath : "");
				free(stmtPath);
			} else {
				cJSON_AddStringToObject(pending, "statementPath", "");
			}
		}
		cJSON_AddItemToObject(root, "pending", pending);
	}

	char * json = cJSON_Print(root);
	cJSON_Delete(root);
	if (json == NULL) return false;

	FILE * file = fopen(savePath, "w");
	if (file == NULL) {
		free(json);
		return false;
	}
	fprintf(file, "%s", json);
	fclose(file);
	free(json);
	return true;
}

bool Engine_loadState(Engine * engine, const char * savePath) {
	if (engine == NULL || savePath == NULL) return false;
	FILE * file = fopen(savePath, "rb");
	if (file == NULL) return false;
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char * buffer = (char *) malloc(size + 1);
	if (buffer == NULL) { fclose(file); return false; }
	size_t _read = fread(buffer, 1, size, file);
	(void) _read;
	buffer[size] = '\0';
	fclose(file);

	cJSON * root = cJSON_Parse(buffer);
	free(buffer);
	if (root == NULL) return false;

	const char * savedSource = cJSON_GetStringValue(cJSON_GetObjectItem(root, "storySource"));
	if (savedSource == NULL || engine->story->source == NULL || strcmp(savedSource, engine->story->source) != 0) {
		cJSON_Delete(root);
		return false;
	}

	/* clear existing state */
	_clearWaiting(engine);
	while (engine->story->variables != NULL) {
		Variable * next = engine->story->variables->next;
		free(engine->story->variables->name);
		free(engine->story->variables);
		engine->story->variables = next;
	}
	engine->stackTop = -1;
	engine->finished = false;
	_applyHideBackground(engine);
	_applyHideSprite(engine);
	_applyStopMusic(engine);
	_applyStopSound(engine);

	/* finished */
	cJSON * finishedItem = cJSON_GetObjectItem(root, "finished");
	if (cJSON_IsBool(finishedItem)) {
		engine->finished = cJSON_IsTrue(finishedItem);
	}
	if (engine->finished) {
		cJSON_Delete(root);
		return true;
	}

	/* waitingForInput */
	cJSON * waitingItem = cJSON_GetObjectItem(root, "waitingForInput");
	if (cJSON_IsBool(waitingItem)) {
		engine->waitingForInput = cJSON_IsTrue(waitingItem);
	}

	/* variables */
	cJSON * vars = cJSON_GetObjectItem(root, "variables");
	if (cJSON_IsArray(vars)) {
		cJSON * v = NULL;
		cJSON_ArrayForEach(v, vars) {
			const char * name = cJSON_GetStringValue(cJSON_GetObjectItem(v, "name"));
			cJSON * valObj = cJSON_GetObjectItem(v, "value");
			if (name != NULL && cJSON_IsNumber(valObj)) {
				_setVariable(engine, name, valObj->valueint);
			}
		}
	}

	/* media */
	cJSON * media = cJSON_GetObjectItem(root, "media");
	if (cJSON_IsObject(media)) {
		const char * bg = cJSON_GetStringValue(cJSON_GetObjectItem(media, "shownBackground"));
		const char * spr = cJSON_GetStringValue(cJSON_GetObjectItem(media, "shownSprite"));
		const char * mus = cJSON_GetStringValue(cJSON_GetObjectItem(media, "playingMusic"));
		const char * snd = cJSON_GetStringValue(cJSON_GetObjectItem(media, "playingSound"));
		if (bg != NULL) _applyShowBackground(engine, bg);
		else _applyHideBackground(engine);
		if (spr != NULL) _applyShowSprite(engine, spr);
		else _applyHideSprite(engine);
		if (mus != NULL) _applyPlayMusic(engine, mus);
		else _applyStopMusic(engine);
		if (snd != NULL) _applyPlaySound(engine, snd);
		else _applyStopSound(engine);
	}

	/* stack */
	cJSON * stack = cJSON_GetObjectItem(root, "stack");
	if (cJSON_IsArray(stack)) {
		cJSON * frame = NULL;
		cJSON_ArrayForEach(frame, stack) {
			const char * listPath = cJSON_GetStringValue(cJSON_GetObjectItem(frame, "listPath"));
			cJSON * idxObj = cJSON_GetObjectItem(frame, "stmtIndex");
			if (listPath != NULL && cJSON_IsNumber(idxObj)) {
				StatementList * list = StoryLoader_resolveListPath(engine->story, listPath);
				if (list != NULL) {
					_pushContext(engine, list);
					if (engine->finished) {
						/* stack overflow during push */
						cJSON_Delete(root);
						return false;
					}
					int idx = idxObj->valueint;
					if (idx >= 0) {
						engine->stack[engine->stackTop].current = StoryLoader_getStatementAtIndex(list, idx);
					} else {
						engine->stack[engine->stackTop].current = NULL;
					}
				}
			}
		}
	}

	/* pending */
	cJSON * pending = cJSON_GetObjectItem(root, "pending");
	if (cJSON_IsObject(pending)) {
		const char * kind = cJSON_GetStringValue(cJSON_GetObjectItem(pending, "kind"));
		if (kind != NULL && strcmp(kind, "dialogue") == 0) {
			const char * actorId = cJSON_GetStringValue(cJSON_GetObjectItem(pending, "actorId"));
			const char * text = cJSON_GetStringValue(cJSON_GetObjectItem(pending, "text"));
			if (actorId != NULL && text != NULL) {
				engine->savedActorId = _strdup(actorId);
				engine->savedDialogueText = _strdup(text);
				engine->currentActorId = engine->savedActorId;
				engine->currentDialogueText = engine->savedDialogueText;
			}
		} else if (kind != NULL && strcmp(kind, "choice") == 0) {
			const char * stmtPath = cJSON_GetStringValue(cJSON_GetObjectItem(pending, "statementPath"));
			if (stmtPath != NULL) {
				Statement * stmt = StoryLoader_resolveStatementPath(engine->story, stmtPath);
				if (stmt != NULL && stmt->kind == STMT_CHOICE) {
					engine->currentChoices = stmt->choice.options;
				}
			}
		}
	}

	cJSON_Delete(root);
	return true;
}
