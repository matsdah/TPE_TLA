#include "Generator.h"
#include <stdbool.h>

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/* JSON OUTPUT */

static const char * _outputPath = ".build/story.json";

/* MODULE LIFECYCLE */

void _shutdownGeneratorModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: Generator...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeGeneratorModule() {
	_logger = createLogger("Generator");
	return _shutdownGeneratorModule;
}

/* JSON HELPERS */

static void _writeEscaped(FILE * stream, const char * value) {
	for (size_t k = 0; value[k] != '\0'; ++k) {
		switch (value[k]) {
			case '"': fputs("\\\"", stream); break;
			case '\\': fputs("\\\\", stream); break;
			case '\b': fputs("\\b", stream); break;
			case '\f': fputs("\\f", stream); break;
			case '\n': fputs("\\n", stream); break;
			case '\r': fputs("\\r", stream); break;
			case '\t': fputs("\\t", stream); break;
			default:
				if ((unsigned char)value[k] < 0x20) {
					fprintf(stream, "\\u%04x", (unsigned char)value[k]);
				}
				else {
					fputc(value[k], stream);
				}
				break;
		}
	}
}

static void _writeString(FILE * stream, const char * value) {
	fputc('"', stream);
	if (value != NULL) {
		_writeEscaped(stream, value);
	}
	fputc('"', stream);
}

static const char * _assignOpToString(AssignOp op) {
	switch (op) {
		case ASSIGN_OP: return "=";
		case ASSIGN_ADD_OP: return "+=";
		case ASSIGN_SUB_OP: return "-=";
		default: return "=";
	}
}

static const char * _arithOpToString(ArithOp op) {
	switch (op) {
		case ARITH_ADD: return "+";
		case ARITH_SUB: return "-";
		case ARITH_MUL: return "*";
		case ARITH_DIV: return "/";
		default: return "+";
	}
}

static const char * _comparisonOpToString(ComparisonOp op) {
	switch (op) {
		case CMP_LT: return "<";
		case CMP_GT: return ">";
		case CMP_LE: return "<=";
		case CMP_GE: return ">=";
		case CMP_EQ: return "==";
		case CMP_NE: return "!=";
		default: return "==";
	}
}

static const char * _displayTypeToString(ResourceDisplayType type) {
	switch (type) {
		case DISPLAY_BACKGROUND: return "background";
		case DISPLAY_SPRITE: return "sprite";
		default: return "background";
	}
}

static const char * _audioTypeToString(ResourceAudioType type) {
	switch (type) {
		case AUDIO_MUSIC: return "music";
		case AUDIO_SOUND: return "sound";
		default: return "music";
	}
}

/* JSON SERIALIZATION */

static void _writeExpression(FILE * stream, Expression * expression);
static void _writeCondition(FILE * stream, Condition * condition);
static void _writeStatementList(FILE * stream, StatementList * list);

static void _writeExpression(FILE * stream, Expression * expression) {
	if (expression == NULL) {
		fputs("null", stream);
		return;
	}
	if (expression->type == EXPR_INTEGER) {
		fputs("{\"kind\":\"integer\",\"value\":", stream);
		fprintf(stream, "%d", expression->integer);
		fputs("}", stream);
		return;
	}
	if (expression->type == EXPR_IDENTIFIER) {
		fputs("{\"kind\":\"identifier\",\"name\":", stream);
		_writeString(stream, expression->identifier);
		fputs("}", stream);
		return;
	}
	if (expression->type == EXPR_BINARY) {
		fputs("{\"kind\":\"binary\",\"op\":", stream);
		_writeString(stream, _arithOpToString(expression->binary.op));
		fputs(",\"left\":", stream);
		_writeExpression(stream, expression->binary.left);
		fputs(",\"right\":", stream);
		_writeExpression(stream, expression->binary.right);
		fputs("}", stream);
		return;
	}
}

static void _writeCondition(FILE * stream, Condition * condition) {
	if (condition == NULL) {
		fputs("null", stream);
		return;
	}
	fputs("{\"kind\":\"comparison\",\"op\":", stream);
	_writeString(stream, _comparisonOpToString(condition->op));
	fputs(",\"left\":", stream);
	_writeExpression(stream, condition->left);
	fputs(",\"right\":", stream);
	_writeExpression(stream, condition->right);
	fputs("}", stream);
}

static void _writeStatement(FILE * stream, Statement * statement) {
	if (statement == NULL) {
		fputs("null", stream);
		return;
	}
	switch (statement->type) {
		case STMT_DIALOGUE:
			fputs("{\"kind\":\"dialogue\",\"actorId\":", stream);
			_writeString(stream, statement->dialogue->actorId);
			fputs(",\"text\":", stream);
			_writeString(stream, statement->dialogue->text);
			fputs("}", stream);
			break;
		case STMT_SHOW:
			fputs("{\"kind\":\"show\",\"target\":", stream);
			_writeString(stream, _displayTypeToString(statement->show->displayType));
			fputs(",\"resourceId\":", stream);
			_writeString(stream, statement->show->resourceId);
			fputs("}", stream);
			break;
		case STMT_HIDE:
			fputs("{\"kind\":\"hide\",\"target\":", stream);
			_writeString(stream, _displayTypeToString(statement->hide->displayType));
			fputs(",\"resourceId\":", stream);
			_writeString(stream, statement->hide->resourceId);
			fputs("}", stream);
			break;
		case STMT_PLAY:
			fputs("{\"kind\":\"play\",\"target\":", stream);
			_writeString(stream, _audioTypeToString(statement->play->audioType));
			fputs(",\"resourceId\":", stream);
			_writeString(stream, statement->play->resourceId);
			fputs("}", stream);
			break;
		case STMT_STOP:
			fputs("{\"kind\":\"stop\",\"target\":", stream);
			_writeString(stream, _audioTypeToString(statement->stop->audioType));
			fputs(",\"resourceId\":", stream);
			_writeString(stream, statement->stop->resourceId);
			fputs("}", stream);
			break;
		case STMT_GOTO:
			fputs("{\"kind\":\"goto\",\"scene\":", stream);
			_writeString(stream, statement->goto_->sceneName);
			fputs("}", stream);
			break;
		case STMT_SET:
			fputs("{\"kind\":\"set\",\"identifier\":", stream);
			_writeString(stream, statement->set->identifier);
			fputs(",\"op\":", stream);
			_writeString(stream, _assignOpToString(statement->set->op));
			fputs(",\"expression\":", stream);
			_writeExpression(stream, statement->set->expr);
			fputs("}", stream);
			break;
		case STMT_CHOICE:
			fputs("{\"kind\":\"choice\",\"options\":[", stream);
			for (ChoiceOption * option = statement->choice->options; option != NULL; option = option->next) {
				fputs("{\"text\":", stream);
				_writeString(stream, option->text);
				fputs(",\"statements\":", stream);
				_writeStatementList(stream, option->body);
				fputs("}", stream);
				if (option->next != NULL) {
					fputs(",", stream);
				}
			}
			fputs("]}", stream);
			break;
		case STMT_IF:
			fputs("{\"kind\":\"if\",\"condition\":", stream);
			_writeCondition(stream, statement->if_->condition);
			fputs(",\"then\":", stream);
			_writeStatementList(stream, statement->if_->thenBlock);
			fputs(",\"else\":", stream);
			_writeStatementList(stream, statement->if_->elseBlock);
			fputs("}", stream);
			break;
		case STMT_END:
			fputs("{\"kind\":\"end\"}", stream);
			break;
	}
}

static void _writeStatementList(FILE * stream, StatementList * list) {
	fputs("[", stream);
	if (list != NULL) {
		for (Statement * stmt = list->head; stmt != NULL; stmt = stmt->next) {
			_writeStatement(stream, stmt);
			if (stmt->next != NULL) {
				fputs(",", stream);
			}
		}
	}
	fputs("]", stream);
}

static void _writeDeclarations(FILE * stream, DeclarationList * list) {
	fputs("\"actors\":[", stream);
	if (list != NULL) {
		bool first = true;
		for (Declaration * decl = list->head; decl != NULL; decl = decl->next) {
			if (decl->type == DECL_CHARACTER) {
				if (!first) {
					fputs(",", stream);
				}
				first = false;
				fputs("{\"id\":", stream);
				_writeString(stream, decl->character->identifier);
				fputs(",\"name\":", stream);
				_writeString(stream, decl->character->displayName);
				fputs(",\"color\":", stream);
				_writeString(stream, decl->character->color);
				fputs("}", stream);
			}
		}
	}
	fputs("]", stream);

	fputs(",\"assets\":[", stream);
	if (list != NULL) {
		bool first = true;
		for (Declaration * decl = list->head; decl != NULL; decl = decl->next) {
			if (decl->type == DECL_ASSET) {
				if (!first) {
					fputs(",", stream);
				}
				first = false;
				fputs("{\"id\":", stream);
				_writeString(stream, decl->asset->identifier);
				fputs(",\"path\":", stream);
				_writeString(stream, decl->asset->filePath);
				fputs("}", stream);
			}
		}
	}
	fputs("]", stream);

	fputs(",\"declarations\":[", stream);
	if (list != NULL) {
		bool first = true;
		for (Declaration * decl = list->head; decl != NULL; decl = decl->next) {
			if (decl->type == DECL_SET) {
				if (!first) {
					fputs(",", stream);
				}
				first = false;
				fputs("{\"kind\":\"set\",\"identifier\":", stream);
				_writeString(stream, decl->set->identifier);
				fputs(",\"op\":", stream);
				_writeString(stream, _assignOpToString(decl->set->op));
				fputs(",\"expression\":", stream);
				_writeExpression(stream, decl->set->expr);
				fputs("}", stream);
			}
		}
	}
	fputs("]", stream);
}

static void _writeScenes(FILE * stream, DeclarationList * list) {
	fputs("\"scenes\":[", stream);
	if (list != NULL) {
		bool first = true;
		for (Declaration * decl = list->head; decl != NULL; decl = decl->next) {
			if (decl->type == DECL_SCENE) {
				if (!first) {
					fputs(",", stream);
				}
				first = false;
				fputs("{\"name\":", stream);
				_writeString(stream, decl->scene->name);
				fputs(",\"statements\":", stream);
				_writeStatementList(stream, decl->scene->body);
				fputs("}", stream);
			}
		}
	}
	fputs("]", stream);
}

static void _writeProgram(FILE * stream, Program * program, const char * sourcePath) {
	fputs("{", stream);
	fputs("\"meta\":{\"version\":\"1.0\",\"source\":", stream);
	_writeString(stream, sourcePath);
	fputs("},", stream);
	_writeDeclarations(stream, program->declarations);
	fputs(",", stream);
	_writeScenes(stream, program->declarations);
	fputs("}", stream);
}

/* PUBLIC FUNCTIONS */

void executeGenerator(CompilerState * compilerState) {
	if (compilerState == NULL || compilerState->abstractSyntaxtTree == NULL) {
		logError(_logger, "Cannot generate output because AST is missing.");
		return;
	}
	Program * program = (Program *) compilerState->abstractSyntaxtTree;
	const char * sourcePath = compilerState->sourcePath == NULL ? "" : compilerState->sourcePath;
	FILE * stream = fopen(_outputPath, "w");
	if (stream == NULL) {
		logError(_logger, "Cannot open output file: %s", _outputPath);
		return;
	}
	_writeProgram(stream, program, sourcePath == NULL ? "" : sourcePath);
	fputs("\n", stream);
	fclose(stream);
	logDebugging(_logger, "JSON output written to %s", _outputPath);
}
