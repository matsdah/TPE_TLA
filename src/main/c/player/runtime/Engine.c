#include "Engine.h"
#include "ExpressionEvaluator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	engine->currentChoices = NULL;
}

static void _stepNext(Engine * engine);

static const char * _resolveAssetPath(Engine * engine, const char * resourceId) {
	for (Asset * a = engine->story->assets; a != NULL; a = a->next) {
		if (strcmp(a->id, resourceId) == 0) return a->path;
	}
	return NULL;
}

/* ── Core stepping logic ────────────────────────────────────────────── */

static void _stepStatement(Engine * engine, Statement * stmt) {
	if (stmt == NULL) return;
	switch (stmt->kind) {
		case STMT_DIALOGUE:
			engine->currentActorId = stmt->dialogue.actorId;
			engine->currentDialogueText = stmt->dialogue.text;
			engine->waitingForInput = true;
			break;

		case STMT_SHOW:
			if (strcmp(stmt->show.target, "background") == 0) {
				free(engine->shownBackground);
				engine->shownBackground = (char *) malloc(strlen(stmt->show.resourceId) + 1);
				strcpy(engine->shownBackground, stmt->show.resourceId);
				if (engine->bgTextureLoaded) {
					UnloadTexture(engine->bgTexture);
					engine->bgTextureLoaded = false;
				}
				const char * path = _resolveAssetPath(engine, stmt->show.resourceId);
				if (path != NULL) {
					engine->bgTexture = LoadTexture(path);
					if (engine->bgTexture.id != 0) {
						engine->bgTextureLoaded = true;
					} else {
						fprintf(stderr, "Engine: failed to load background texture '%s'\n", path);
					}
				} else {
					fprintf(stderr, "Engine: asset '%s' not found for background show\n", stmt->show.resourceId);
				}
			} else {
				free(engine->shownSprite);
				engine->shownSprite = (char *) malloc(strlen(stmt->show.resourceId) + 1);
				strcpy(engine->shownSprite, stmt->show.resourceId);
				if (engine->spriteTextureLoaded) {
					UnloadTexture(engine->spriteTexture);
					engine->spriteTextureLoaded = false;
				}
				const char * path = _resolveAssetPath(engine, stmt->show.resourceId);
				if (path != NULL) {
					engine->spriteTexture = LoadTexture(path);
					if (engine->spriteTexture.id != 0) {
						engine->spriteTextureLoaded = true;
					} else {
						fprintf(stderr, "Engine: failed to load sprite texture '%s'\n", path);
					}
				} else {
					fprintf(stderr, "Engine: asset '%s' not found for sprite show\n", stmt->show.resourceId);
				}
			}
			_stepNext(engine);
			break;

		case STMT_HIDE:
			if (strcmp(stmt->hide.target, "background") == 0) {
				free(engine->shownBackground); engine->shownBackground = NULL;
				if (engine->bgTextureLoaded) {
					UnloadTexture(engine->bgTexture);
					engine->bgTextureLoaded = false;
				}
			} else {
				free(engine->shownSprite); engine->shownSprite = NULL;
				if (engine->spriteTextureLoaded) {
					UnloadTexture(engine->spriteTexture);
					engine->spriteTextureLoaded = false;
				}
			}
			_stepNext(engine);
			break;

		case STMT_PLAY:
			if (strcmp(stmt->play.target, "music") == 0) {
				free(engine->playingMusic);
				engine->playingMusic = (char *) malloc(strlen(stmt->play.resourceId) + 1);
				strcpy(engine->playingMusic, stmt->play.resourceId);
				if (engine->musicPlaying) {
					StopMusicStream(engine->currentMusic);
					UnloadMusicStream(engine->currentMusic);
					engine->musicPlaying = false;
				}
				const char * path = _resolveAssetPath(engine, stmt->play.resourceId);
				if (path != NULL) {
					engine->currentMusic = LoadMusicStream(path);
					if (engine->currentMusic.stream.buffer != NULL) {
						PlayMusicStream(engine->currentMusic);
						engine->musicPlaying = true;
					} else {
						fprintf(stderr, "Engine: failed to load music '%s'\n", path);
					}
				} else {
					fprintf(stderr, "Engine: asset '%s' not found for music play\n", stmt->play.resourceId);
				}
			} else {
				free(engine->playingSound);
				engine->playingSound = (char *) malloc(strlen(stmt->play.resourceId) + 1);
				strcpy(engine->playingSound, stmt->play.resourceId);
				if (engine->soundPlaying) {
					UnloadSound(engine->currentSound);
					engine->soundPlaying = false;
				}
				const char * path = _resolveAssetPath(engine, stmt->play.resourceId);
				if (path != NULL) {
					engine->currentSound = LoadSound(path);
					if (engine->currentSound.stream.buffer != NULL) {
						PlaySound(engine->currentSound);
						engine->soundPlaying = true;
					} else {
						fprintf(stderr, "Engine: failed to load sound '%s'\n", path);
					}
				} else {
					fprintf(stderr, "Engine: asset '%s' not found for sound play\n", stmt->play.resourceId);
				}
			}
			_stepNext(engine);
			break;

		case STMT_STOP:
			if (strcmp(stmt->stop.target, "music") == 0) {
				free(engine->playingMusic); engine->playingMusic = NULL;
				if (engine->musicPlaying) {
					StopMusicStream(engine->currentMusic);
					UnloadMusicStream(engine->currentMusic);
					engine->musicPlaying = false;
				}
			} else {
				free(engine->playingSound); engine->playingSound = NULL;
				if (engine->soundPlaying) {
					UnloadSound(engine->currentSound);
					engine->soundPlaying = false;
				}
			}
			_stepNext(engine);
			break;

		case STMT_GOTO:
		{
			Scene * target = StoryLoader_findScene(engine->story, stmt->goto_.sceneName);
			engine->stackTop = -1;
			if (target != NULL) {
				_pushContext(engine, target->statements);
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
			if (block != NULL) {
				_pushContext(engine, block);
			}
			/* if the block is empty, just step next in current context */
			if (block == NULL || block->head == NULL) {
				_stepNext(engine);
			}
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
