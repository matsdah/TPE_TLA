#include "StoryLoader.h"
#include "../vendor/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

static char * _strdup(const char * src) {
	if (src == NULL) return NULL;
	size_t len = strlen(src) + 1;
	char * dst = (char *) malloc(len);
	if (dst != NULL) memcpy(dst, src, len);
	return dst;
}

/* ── JSON helpers ───────────────────────────────────────────────────── */

static const char * _getString(cJSON * obj, const char * key) {
	cJSON * item = cJSON_GetObjectItem(obj, key);
	if (item == NULL || !cJSON_IsString(item)) return NULL;
	return item->valuestring;
}

static int _getInt(cJSON * obj, const char * key) {
	cJSON * item = cJSON_GetObjectItem(obj, key);
	if (item == NULL || !cJSON_IsNumber(item)) return 0;
	return item->valueint;
}

/* ── Parsing sub-structures ─────────────────────────────────────────── */

static Expression * _parseExpression(cJSON * json);
static Condition * _parseCondition(cJSON * json);
static StatementList * _parseStatementList(cJSON * json);

static Expression * _parseExpression(cJSON * json) {
	if (json == NULL) return NULL;
	Expression * expr = (Expression *) calloc(1, sizeof(Expression));
	const char * kind = _getString(json, "kind");
	if (strcmp(kind, "integer") == 0) {
		expr->type = EXPR_INTEGER;
		expr->integer = _getInt(json, "value");
	} else if (strcmp(kind, "identifier") == 0) {
		expr->type = EXPR_IDENTIFIER;
		expr->identifier = _strdup(_getString(json, "name"));
	} else if (strcmp(kind, "binary") == 0) {
		expr->type = EXPR_BINARY;
		expr->binary.left = _parseExpression(cJSON_GetObjectItem(json, "left"));
		const char * op = _getString(json, "op");
		if (strcmp(op, "+") == 0) expr->binary.op = OP_ADD;
		else if (strcmp(op, "-") == 0) expr->binary.op = OP_SUB;
		else if (strcmp(op, "*") == 0) expr->binary.op = OP_MUL;
		else expr->binary.op = OP_DIV;
		expr->binary.right = _parseExpression(cJSON_GetObjectItem(json, "right"));
	}
	return expr;
}

static Condition * _parseCondition(cJSON * json) {
	if (json == NULL) return NULL;
	Condition * cond = (Condition *) calloc(1, sizeof(Condition));
	const char * op = _getString(json, "op");
	if (strcmp(op, "<") == 0) cond->op = CMP_LT;
	else if (strcmp(op, ">") == 0) cond->op = CMP_GT;
	else if (strcmp(op, "<=") == 0) cond->op = CMP_LE;
	else if (strcmp(op, ">=") == 0) cond->op = CMP_GE;
	else if (strcmp(op, "==") == 0) cond->op = CMP_EQ;
	else cond->op = CMP_NE;
	cond->left = _parseExpression(cJSON_GetObjectItem(json, "left"));
	cond->right = _parseExpression(cJSON_GetObjectItem(json, "right"));
	return cond;
}

static Statement * _parseStatement(cJSON * json) {
	if (json == NULL) return NULL;
	Statement * stmt = (Statement *) calloc(1, sizeof(Statement));
	const char * kind = _getString(json, "kind");
	if (strcmp(kind, "dialogue") == 0) {
		stmt->kind = STMT_DIALOGUE;
		stmt->dialogue.actorId = _strdup(_getString(json, "actorId"));
		stmt->dialogue.text = _strdup(_getString(json, "text"));
	} else if (strcmp(kind, "show") == 0) {
		stmt->kind = STMT_SHOW;
		stmt->show.target = _strdup(_getString(json, "target"));
		stmt->show.resourceId = _strdup(_getString(json, "resourceId"));
	} else if (strcmp(kind, "hide") == 0) {
		stmt->kind = STMT_HIDE;
		stmt->hide.target = _strdup(_getString(json, "target"));
		stmt->hide.resourceId = _strdup(_getString(json, "resourceId"));
	} else if (strcmp(kind, "play") == 0) {
		stmt->kind = STMT_PLAY;
		stmt->play.target = _strdup(_getString(json, "target"));
		stmt->play.resourceId = _strdup(_getString(json, "resourceId"));
	} else if (strcmp(kind, "stop") == 0) {
		stmt->kind = STMT_STOP;
		stmt->stop.target = _strdup(_getString(json, "target"));
		stmt->stop.resourceId = _strdup(_getString(json, "resourceId"));
	} else if (strcmp(kind, "goto") == 0) {
		stmt->kind = STMT_GOTO;
		stmt->goto_.sceneName = _strdup(_getString(json, "scene"));
	} else if (strcmp(kind, "set") == 0) {
		stmt->kind = STMT_SET;
		stmt->set.identifier = _strdup(_getString(json, "identifier"));
		stmt->set.op = _strdup(_getString(json, "op"));
		stmt->set.expr = _parseExpression(cJSON_GetObjectItem(json, "expression"));
	} else if (strcmp(kind, "choice") == 0) {
		stmt->kind = STMT_CHOICE;
		cJSON * options = cJSON_GetObjectItem(json, "options");
		ChoiceOption * head = NULL;
		ChoiceOption * tail = NULL;
		cJSON * opt = NULL;
		cJSON_ArrayForEach(opt, options) {
			ChoiceOption * option = (ChoiceOption *) calloc(1, sizeof(ChoiceOption));
			option->text = _strdup(_getString(opt, "text"));
			option->body = _parseStatementList(cJSON_GetObjectItem(opt, "statements"));
			if (head == NULL) { head = tail = option; }
			else { tail->next = option; tail = option; }
		}
		stmt->choice.options = head;
	} else if (strcmp(kind, "if") == 0) {
		stmt->kind = STMT_IF;
		stmt->if_.condition = _parseCondition(cJSON_GetObjectItem(json, "condition"));
		stmt->if_.thenBlock = _parseStatementList(cJSON_GetObjectItem(json, "then"));
		stmt->if_.elseBlock = _parseStatementList(cJSON_GetObjectItem(json, "else"));
	} else if (strcmp(kind, "end") == 0) {
		stmt->kind = STMT_END;
	}
	return stmt;
}

static StatementList * _parseStatementList(cJSON * json) {
	if (json == NULL) return NULL;
	StatementList * list = (StatementList *) calloc(1, sizeof(StatementList));
	cJSON * item = NULL;
	cJSON_ArrayForEach(item, json) {
		Statement * stmt = _parseStatement(item);
		if (list->head == NULL) { list->head = list->tail = stmt; }
		else { list->tail->next = stmt; list->tail = stmt; }
	}
	return list;
}

/* ── Public functions ──────────────────────────────────────────────── */

Story * StoryLoader_load(const char * jsonPath) {
	FILE * file = fopen(jsonPath, "rb");
	if (file == NULL) {
		fprintf(stderr, "Cannot open story JSON: %s\n", jsonPath);
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char * buffer = (char *) malloc(size + 1);
	if (buffer == NULL) { fclose(file); return NULL; }
	size_t _read = fread(buffer, 1, size, file);
	(void) _read;
	buffer[size] = '\0';
	fclose(file);

	cJSON * root = cJSON_Parse(buffer);
	free(buffer);
	if (root == NULL) {
		fprintf(stderr, "Failed to parse story JSON: %s\n", cJSON_GetErrorPtr());
		return NULL;
	}

	Story * story = (Story *) calloc(1, sizeof(Story));

	/* actors */
	cJSON * actors = cJSON_GetObjectItem(root, "actors");
	cJSON * a = NULL;
	cJSON_ArrayForEach(a, actors) {
		Actor * actor = (Actor *) calloc(1, sizeof(Actor));
		actor->id = _strdup(_getString(a, "id"));
		actor->name = _strdup(_getString(a, "name"));
		actor->color = _strdup(_getString(a, "color"));
		if (story->actors == NULL) story->actors = actor;
		else {
			Actor * tail = story->actors;
			while (tail->next != NULL) tail = tail->next;
			tail->next = actor;
		}
	}

	/* assets */
	cJSON * assets = cJSON_GetObjectItem(root, "assets");
	cJSON_ArrayForEach(a, assets) {
		Asset * asset = (Asset *) calloc(1, sizeof(Asset));
		asset->id = _strdup(_getString(a, "id"));
		asset->path = _strdup(_getString(a, "path"));
		if (story->assets == NULL) story->assets = asset;
		else {
			Asset * tail = story->assets;
			while (tail->next != NULL) tail = tail->next;
			tail->next = asset;
		}
	}

	/* declarations (global sets) */
	cJSON * declarations = cJSON_GetObjectItem(root, "declarations");
	story->declarations = _parseStatementList(declarations);

	/* scenes */
	cJSON * scenes = cJSON_GetObjectItem(root, "scenes");
	cJSON * s = NULL;
	cJSON_ArrayForEach(s, scenes) {
		Scene * scene = (Scene *) calloc(1, sizeof(Scene));
		scene->name = _strdup(_getString(s, "name"));
		scene->statements = _parseStatementList(cJSON_GetObjectItem(s, "statements"));
		if (story->scenes == NULL) {
			story->scenes = (SceneList *) calloc(1, sizeof(SceneList));
			story->scenes->head = story->scenes->tail = scene;
		} else {
			story->scenes->tail->next = scene;
			story->scenes->tail = scene;
		}
	}

	cJSON_Delete(root);
	return story;
}

void StoryLoader_destroy(Story * story) {
	if (story == NULL) return;
	while (story->actors != NULL) {
		Actor * next = story->actors->next;
		free(story->actors->id); free(story->actors->name); free(story->actors->color);
		free(story->actors);
		story->actors = next;
	}
	while (story->assets != NULL) {
		Asset * next = story->assets->next;
		free(story->assets->id); free(story->assets->path);
		free(story->assets);
		story->assets = next;
	}
	while (story->variables != NULL) {
		Variable * next = story->variables->next;
		free(story->variables->name);
		free(story->variables);
		story->variables = next;
	}
	StoryLoader_freeStatementList(story->declarations);
	free(story->declarations);
	if (story->scenes != NULL) {
		Scene * scene = story->scenes->head;
		while (scene != NULL) {
			Scene * next = scene->next;
			free(scene->name);
			StoryLoader_freeStatementList(scene->statements);
			free(scene->statements);
			free(scene);
			scene = next;
		}
		free(story->scenes);
	}
	free(story);
}

void StoryLoader_resolveAssetPaths(Story * story, const char * jsonDirectory) {
	if (story == NULL || jsonDirectory == NULL) return;
	for (Asset * a = story->assets; a != NULL; a = a->next) {
		if (a->path == NULL) continue;
		size_t dirLen = strlen(jsonDirectory);
		size_t pathLen = strlen(a->path);
		char * resolved = (char *) malloc(dirLen + 1 + pathLen + 1);
		if (resolved == NULL) continue;
		memcpy(resolved, jsonDirectory, dirLen);
		resolved[dirLen] = '/';
		memcpy(resolved + dirLen + 1, a->path, pathLen + 1);
		free(a->path);
		a->path = resolved;
	}
}

Scene * StoryLoader_findScene(Story * story, const char * name) {
	if (story == NULL || story->scenes == NULL || name == NULL) return NULL;
	for (Scene * s = story->scenes->head; s != NULL; s = s->next) {
		if (strcmp(s->name, name) == 0) return s;
	}
	return NULL;
}

/* ── Clone / free helpers ───────────────────────────────────────────── */

static Expression * _cloneExpression(const Expression * expr) {
	if (expr == NULL) return NULL;
	Expression * copy = (Expression *) calloc(1, sizeof(Expression));
	copy->type = expr->type;
	if (expr->type == EXPR_INTEGER) copy->integer = expr->integer;
	else if (expr->type == EXPR_IDENTIFIER) copy->identifier = _strdup(expr->identifier);
	else if (expr->type == EXPR_BINARY) {
		copy->binary.left = _cloneExpression(expr->binary.left);
		copy->binary.op = expr->binary.op;
		copy->binary.right = _cloneExpression(expr->binary.right);
	}
	return copy;
}

static void _freeExpression(Expression * expr) {
	if (expr == NULL) return;
	if (expr->type == EXPR_IDENTIFIER) free(expr->identifier);
	else if (expr->type == EXPR_BINARY) {
		_freeExpression(expr->binary.left);
		_freeExpression(expr->binary.right);
	}
	free(expr);
}

static StatementList * _cloneStatementList(const StatementList * list);

static Statement * _cloneStatement(const Statement * stmt) {
	if (stmt == NULL) return NULL;
	Statement * copy = (Statement *) calloc(1, sizeof(Statement));
	copy->kind = stmt->kind;
	switch (stmt->kind) {
		case STMT_DIALOGUE:
			copy->dialogue.actorId = _strdup(stmt->dialogue.actorId);
			copy->dialogue.text = _strdup(stmt->dialogue.text);
			break;
		case STMT_SHOW:
			copy->show.target = _strdup(stmt->show.target);
			copy->show.resourceId = _strdup(stmt->show.resourceId);
			break;
		case STMT_HIDE:
			copy->hide.target = _strdup(stmt->hide.target);
			copy->hide.resourceId = _strdup(stmt->hide.resourceId);
			break;
		case STMT_PLAY:
			copy->play.target = _strdup(stmt->play.target);
			copy->play.resourceId = _strdup(stmt->play.resourceId);
			break;
		case STMT_STOP:
			copy->stop.target = _strdup(stmt->stop.target);
			copy->stop.resourceId = _strdup(stmt->stop.resourceId);
			break;
		case STMT_GOTO:
			copy->goto_.sceneName = _strdup(stmt->goto_.sceneName);
			break;
		case STMT_SET:
			copy->set.identifier = _strdup(stmt->set.identifier);
			copy->set.op = _strdup(stmt->set.op);
			copy->set.expr = _cloneExpression(stmt->set.expr);
			break;
		case STMT_CHOICE:
		{
			ChoiceOption * head = NULL;
			ChoiceOption * tail = NULL;
			for (ChoiceOption * opt = stmt->choice.options; opt != NULL; opt = opt->next) {
				ChoiceOption * ocopy = (ChoiceOption *) calloc(1, sizeof(ChoiceOption));
				ocopy->text = _strdup(opt->text);
				ocopy->body = _cloneStatementList(opt->body);
				if (head == NULL) head = tail = ocopy;
				else { tail->next = ocopy; tail = ocopy; }
			}
			copy->choice.options = head;
			break;
		}
		case STMT_IF:
			copy->if_.condition = (Condition *) calloc(1, sizeof(Condition));
			copy->if_.condition->left = _cloneExpression(stmt->if_.condition->left);
			copy->if_.condition->op = stmt->if_.condition->op;
			copy->if_.condition->right = _cloneExpression(stmt->if_.condition->right);
			copy->if_.thenBlock = _cloneStatementList(stmt->if_.thenBlock);
			copy->if_.elseBlock = _cloneStatementList(stmt->if_.elseBlock);
			break;
		case STMT_END:
			break;
	}
	return copy;
}

static StatementList * _cloneStatementList(const StatementList * list) {
	if (list == NULL) return NULL;
	StatementList * copy = (StatementList *) calloc(1, sizeof(StatementList));
	for (Statement * s = list->head; s != NULL; s = s->next) {
		Statement * scopy = _cloneStatement(s);
		if (copy->head == NULL) copy->head = copy->tail = scopy;
		else { copy->tail->next = scopy; copy->tail = scopy; }
	}
	return copy;
}

StatementList * StoryLoader_cloneStatementList(const StatementList * list) {
	return _cloneStatementList(list);
}

static void _freeStatement(Statement * stmt) {
	if (stmt == NULL) return;
	switch (stmt->kind) {
		case STMT_DIALOGUE:
			free(stmt->dialogue.actorId); free(stmt->dialogue.text);
			break;
		case STMT_SHOW:
			free(stmt->show.target); free(stmt->show.resourceId);
			break;
		case STMT_HIDE:
			free(stmt->hide.target); free(stmt->hide.resourceId);
			break;
		case STMT_PLAY:
			free(stmt->play.target); free(stmt->play.resourceId);
			break;
		case STMT_STOP:
			free(stmt->stop.target); free(stmt->stop.resourceId);
			break;
		case STMT_GOTO:
			free(stmt->goto_.sceneName);
			break;
		case STMT_SET:
			free(stmt->set.identifier); free(stmt->set.op);
			_freeExpression(stmt->set.expr);
			break;
		case STMT_CHOICE:
		{
			ChoiceOption * opt = stmt->choice.options;
			while (opt != NULL) {
				ChoiceOption * next = opt->next;
				free(opt->text);
				StoryLoader_freeStatementList(opt->body);
				free(opt->body);
				free(opt);
				opt = next;
			}
			break;
		}
		case STMT_IF:
			_freeExpression(stmt->if_.condition->left);
			_freeExpression(stmt->if_.condition->right);
			free(stmt->if_.condition);
			StoryLoader_freeStatementList(stmt->if_.thenBlock);
			free(stmt->if_.thenBlock);
			StoryLoader_freeStatementList(stmt->if_.elseBlock);
			free(stmt->if_.elseBlock);
			break;
		case STMT_END:
			break;
	}
	free(stmt);
}

void StoryLoader_freeStatementList(StatementList * list) {
	if (list == NULL) return;
	Statement * stmt = list->head;
	while (stmt != NULL) {
		Statement * next = stmt->next;
		_freeStatement(stmt);
		stmt = next;
	}
	list->head = list->tail = NULL;
}
