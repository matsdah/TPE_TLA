#include "SemanticAnalyzer.h"
#include <string.h>

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

void _shutdownSemanticAnalyzerModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: SemanticAnalyzer...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeSemanticAnalyzerModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("SemanticAnalyzer");
	return _shutdownSemanticAnalyzerModule;
}

/* ── Symbol table helpers ─────────────────────────────────────────── */

typedef struct SymbolEntry {
	char * name;
	struct SymbolEntry * next;
} SymbolEntry;

static SymbolEntry * _createSymbolEntry(const char * name) {
	SymbolEntry * entry = (SymbolEntry *) calloc(1, sizeof(SymbolEntry));
	entry->name = (char *) malloc(strlen(name) + 1);
	strcpy(entry->name, name);
	return entry;
}

static void _destroySymbolTable(SymbolEntry * head) {
	while (head != NULL) {
		SymbolEntry * next = head->next;
		free(head->name);
		free(head);
		head = next;
	}
}

static bool _symbolExists(SymbolEntry * head, const char * name) {
	for (SymbolEntry * e = head; e != NULL; e = e->next) {
		if (strcmp(e->name, name) == 0) return true;
	}
	return false;
}

static void _addSymbol(SymbolEntry ** head, const char * name) {
	SymbolEntry * entry = _createSymbolEntry(name);
	entry->next = *head;
	*head = entry;
}

/* ── Forward declarations ─────────────────────────────────────────── */

static bool _collectSetVariables(StatementList * list, SymbolEntry ** variables, SymbolEntry * characters);
static bool _validateStatements(StatementList * list, SymbolEntry * characters, SymbolEntry * assets, SymbolEntry * scenes, SymbolEntry * variables);
static bool _validateExpression(Expression * expr, SymbolEntry * characters, SymbolEntry * variables);

/* ── Collect variables from set statements ────────────────────────── */

static bool _collectSetVariables(StatementList * list, SymbolEntry ** variables, SymbolEntry * characters) {
	if (list == NULL) return true;
	bool ok = true;
	for (Statement * stmt = list->head; stmt != NULL; stmt = stmt->next) {
		if (stmt->type == STMT_SET) {
			const char * id = stmt->set->identifier;
			if (_symbolExists(characters, id)) {
				logError(_logger, "Semantic error: variable '%s' conflicts with existing character declaration.", id);
				ok = false;
			} else if (!_symbolExists(*variables, id)) {
				_addSymbol(variables, id);
			}
		} else if (stmt->type == STMT_CHOICE) {
			for (ChoiceOption * opt = stmt->choice->options; opt != NULL; opt = opt->next) {
				if (!_collectSetVariables(opt->body, variables, characters)) ok = false;
			}
		} else if (stmt->type == STMT_IF) {
			if (!_collectSetVariables(stmt->if_->thenBlock, variables, characters)) ok = false;
			if (!_collectSetVariables(stmt->if_->elseBlock, variables, characters)) ok = false;
		}
	}
	return ok;
}

/* ── Validate expressions ─────────────────────────────────────────── */

static bool _validateExpression(Expression * expr, SymbolEntry * characters, SymbolEntry * variables) {
	if (expr == NULL) return true;
	if (expr->type == EXPR_INTEGER) return true;
	if (expr->type == EXPR_IDENTIFIER) {
		const char * id = expr->identifier;
		if (_symbolExists(characters, id)) {
			logError(_logger, "Semantic error: identifier '%s' is a character, not a variable, and cannot be used in an expression.", id);
			return false;
		}
		if (!_symbolExists(variables, id)) {
			logError(_logger, "Semantic error: undeclared variable '%s' used in expression.", id);
			return false;
		}
		return true;
	}
	if (expr->type == EXPR_BINARY) {
		bool ok = _validateExpression(expr->binary.left, characters, variables);
		if (!_validateExpression(expr->binary.right, characters, variables)) ok = false;
		return ok;
	}
	return true;
}

static bool _validateCondition(Condition * cond, SymbolEntry * characters, SymbolEntry * variables) {
	if (cond == NULL) return true;
	bool ok = _validateExpression(cond->left, characters, variables);
	if (!_validateExpression(cond->right, characters, variables)) ok = false;
	return ok;
}

/* ── Validate statements ─────────────────────────────────────────── */

static bool _validateStatements(StatementList * list, SymbolEntry * characters, SymbolEntry * assets, SymbolEntry * scenes, SymbolEntry * variables) {
	if (list == NULL) return true;
	bool ok = true;
	for (Statement * stmt = list->head; stmt != NULL; stmt = stmt->next) {
		switch (stmt->type) {
			case STMT_DIALOGUE: {
				if (!_symbolExists(characters, stmt->dialogue->actorId)) {
					logError(_logger, "Semantic error: undeclared actor '%s' used in dialogue.", stmt->dialogue->actorId);
					ok = false;
				}
				break;
			}
			case STMT_SHOW: {
				if (!_symbolExists(assets, stmt->show->resourceId)) {
					logError(_logger, "Semantic error: undeclared asset '%s' used in show.", stmt->show->resourceId);
					ok = false;
				}
				break;
			}
			case STMT_HIDE: {
				if (!_symbolExists(assets, stmt->hide->resourceId)) {
					logError(_logger, "Semantic error: undeclared asset '%s' used in hide.", stmt->hide->resourceId);
					ok = false;
				}
				break;
			}
			case STMT_PLAY: {
				if (!_symbolExists(assets, stmt->play->resourceId)) {
					logError(_logger, "Semantic error: undeclared asset '%s' used in play.", stmt->play->resourceId);
					ok = false;
				}
				break;
			}
			case STMT_STOP: {
				if (!_symbolExists(assets, stmt->stop->resourceId)) {
					logError(_logger, "Semantic error: undeclared asset '%s' used in stop.", stmt->stop->resourceId);
					ok = false;
				}
				break;
			}
			case STMT_GOTO: {
				if (!_symbolExists(scenes, stmt->goto_->sceneName)) {
					logError(_logger, "Semantic error: undeclared scene '%s' used in goto.", stmt->goto_->sceneName);
					ok = false;
				}
				break;
			}
			case STMT_SET: {
				if (!_validateExpression(stmt->set->expr, characters, variables)) ok = false;
				break;
			}
			case STMT_CHOICE: {
				for (ChoiceOption * opt = stmt->choice->options; opt != NULL; opt = opt->next) {
					if (!_validateStatements(opt->body, characters, assets, scenes, variables)) ok = false;
				}
				break;
			}
			case STMT_IF: {
				if (!_validateCondition(stmt->if_->condition, characters, variables)) ok = false;
				if (!_validateStatements(stmt->if_->thenBlock, characters, assets, scenes, variables)) ok = false;
				if (!_validateStatements(stmt->if_->elseBlock, characters, assets, scenes, variables)) ok = false;
				break;
			}
			case STMT_END:
				break;
		}
	}
	return ok;
}

/* ── Public API ───────────────────────────────────────────────────── */

CompilationStatus executeSemanticAnalysis(CompilerState * compilerState) {
	if (compilerState == NULL || compilerState->abstractSyntaxtTree == NULL) {
		logError(_logger, "Cannot perform semantic analysis: AST is missing.");
		return FAILED;
	}
	Program * program = (Program *) compilerState->abstractSyntaxtTree;
	DeclarationList * declarations = program->declarations;

	SymbolEntry * characters = NULL;
	SymbolEntry * assets = NULL;
	SymbolEntry * scenes = NULL;
	SymbolEntry * variables = NULL;
	bool ok = true;

	/* Phase 1: collect declarations (characters, assets, scenes, top-level variables) */
	if (declarations != NULL) {
		for (Declaration * decl = declarations->head; decl != NULL; decl = decl->next) {
			switch (decl->type) {
				case DECL_CHARACTER: {
					const char * id = decl->character->identifier;
					if (_symbolExists(characters, id)) {
						logError(_logger, "Semantic error: duplicate character declaration '%s'.", id);
						ok = false;
					} else {
						_addSymbol(&characters, id);
					}
					break;
				}
				case DECL_ASSET: {
					const char * id = decl->asset->identifier;
					if (_symbolExists(assets, id)) {
						logError(_logger, "Semantic error: duplicate asset declaration '%s'.", id);
						ok = false;
					} else {
						_addSymbol(&assets, id);
					}
					break;
				}
				case DECL_SCENE: {
					const char * name = decl->scene->name;
					if (_symbolExists(scenes, name)) {
						logError(_logger, "Semantic error: duplicate scene declaration '%s'.", name);
						ok = false;
					} else {
						_addSymbol(&scenes, name);
					}
					break;
				}
				case DECL_SET: {
					const char * id = decl->set->identifier;
					if (_symbolExists(characters, id)) {
						logError(_logger, "Semantic error: variable '%s' conflicts with existing character declaration.", id);
						ok = false;
					} else if (!_symbolExists(variables, id)) {
						_addSymbol(&variables, id);
					}
					break;
				}
			}
		}
	}

	/* Phase 2: collect variables from inside scenes (including nested choice/if bodies) */
	if (declarations != NULL) {
		for (Declaration * decl = declarations->head; decl != NULL; decl = decl->next) {
			if (decl->type == DECL_SCENE) {
				if (!_collectSetVariables(decl->scene->body, &variables, characters)) ok = false;
			}
		}
	}

	/* Phase 3: validate top-level set expressions */
	if (declarations != NULL) {
		for (Declaration * decl = declarations->head; decl != NULL; decl = decl->next) {
			if (decl->type == DECL_SET) {
				if (!_validateExpression(decl->set->expr, characters, variables)) ok = false;
			}
		}
	}

	/* Phase 4: validate scene bodies */
	if (declarations != NULL) {
		for (Declaration * decl = declarations->head; decl != NULL; decl = decl->next) {
			if (decl->type == DECL_SCENE) {
				if (!_validateStatements(decl->scene->body, characters, assets, scenes, variables)) ok = false;
			}
		}
	}

	_destroySymbolTable(characters);
	_destroySymbolTable(assets);
	_destroySymbolTable(scenes);
	_destroySymbolTable(variables);

	if (ok) {
		logDebugging(_logger, "Semantic analysis succeeded.");
		return SUCCEEDED;
	} else {
		logDebugging(_logger, "Semantic analysis failed.");
		return FAILED;
	}
}
