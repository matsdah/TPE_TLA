#include "AbstractSyntaxTree.h"
#include <string.h>

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* ── Helpers ──────────────────────────────────────────────────────── */

static char * _copyString(const char * src) {
	if (src == NULL) return NULL;
	char * copy = (char *) calloc(strlen(src) + 1, sizeof(char));
	strcpy(copy, src);
	return copy;
}

/* ── Expression constructors ──────────────────────────────────────── */

Expression * createIntegerExpression(int value) {
	Expression * e = calloc(1, sizeof(Expression));
	e->type    = EXPR_INTEGER;
	e->integer = value;
	return e;
}

Expression * createIdentifierExpression(char * identifier) {
	Expression * e = calloc(1, sizeof(Expression));
	e->type        = EXPR_IDENTIFIER;
	e->identifier  = _copyString(identifier);
	return e;
}

Expression * createBinaryExpression(Expression * left, ArithOp op, Expression * right) {
	Expression * e  = calloc(1, sizeof(Expression));
	e->type         = EXPR_BINARY;
	e->binary.left  = left;
	e->binary.op    = op;
	e->binary.right = right;
	return e;
}

/* ── Condition ────────────────────────────────────────────────────── */

Condition * createCondition(Expression * left, ComparisonOp op, Expression * right) {
	Condition * c = calloc(1, sizeof(Condition));
	c->left  = left;
	c->op    = op;
	c->right = right;
	return c;
}

/* ── StatementList ────────────────────────────────────────────────── */

StatementList * createStatementList() {
	return (StatementList *) calloc(1, sizeof(StatementList));
}

void appendStatement(StatementList * list, Statement * stmt) {
	if (list->head == NULL) {
		list->head = stmt;
		list->tail = stmt;
	} else {
		list->tail->next = stmt;
		list->tail = stmt;
	}
}

/* ── Statement constructors ───────────────────────────────────────── */

static Statement * _allocStatement(StatementType type) {
	Statement * s = calloc(1, sizeof(Statement));
	s->type = type;
	return s;
}

Statement * createDialogueStatement(char * actorId, char * text) {
	Statement * s         = _allocStatement(STMT_DIALOGUE);
	s->dialogue           = calloc(1, sizeof(DialogueStatement));
	s->dialogue->actorId  = _copyString(actorId);
	s->dialogue->text     = _copyString(text);
	return s;
}

Statement * createShowStatement(ResourceDisplayType type, char * resourceId) {
	Statement * s        = _allocStatement(STMT_SHOW);
	s->show              = calloc(1, sizeof(ShowStatement));
	s->show->displayType = type;
	s->show->resourceId  = _copyString(resourceId);
	return s;
}

Statement * createHideStatement(ResourceDisplayType type, char * resourceId) {
	Statement * s        = _allocStatement(STMT_HIDE);
	s->hide              = calloc(1, sizeof(HideStatement));
	s->hide->displayType = type;
	s->hide->resourceId  = _copyString(resourceId);
	return s;
}

Statement * createPlayStatement(ResourceAudioType type, char * resourceId) {
	Statement * s       = _allocStatement(STMT_PLAY);
	s->play             = calloc(1, sizeof(PlayStatement));
	s->play->audioType  = type;
	s->play->resourceId = _copyString(resourceId);
	return s;
}

Statement * createStopStatement(ResourceAudioType type, char * resourceId) {
	Statement * s       = _allocStatement(STMT_STOP);
	s->stop             = calloc(1, sizeof(StopStatement));
	s->stop->audioType  = type;
	s->stop->resourceId = _copyString(resourceId);
	return s;
}

Statement * createGotoStatement(char * sceneName) {
	Statement * s         = _allocStatement(STMT_GOTO);
	s->goto_              = calloc(1, sizeof(GotoStatement));
	s->goto_->sceneName   = _copyString(sceneName);
	return s;
}

Statement * createSetStatement(char * identifier, AssignOp op, Expression * expr) {
	Statement * s       = _allocStatement(STMT_SET);
	s->set              = calloc(1, sizeof(SetStatement));
	s->set->identifier  = _copyString(identifier);
	s->set->op          = op;
	s->set->expr        = expr;
	return s;
}

Statement * createChoiceStatement(ChoiceOption * options) {
	Statement * s      = _allocStatement(STMT_CHOICE);
	s->choice          = calloc(1, sizeof(ChoiceStatement));
	s->choice->options = options;
	return s;
}

Statement * createIfStatement(Condition * cond, StatementList * thenBlock, StatementList * elseBlock) {
	Statement * s           = _allocStatement(STMT_IF);
	s->if_                  = calloc(1, sizeof(IfStatement));
	s->if_->condition       = cond;
	s->if_->thenBlock       = thenBlock;
	s->if_->elseBlock       = elseBlock;
	return s;
}

Statement * createEndStatement() {
	return _allocStatement(STMT_END);
}

/* ── ChoiceOption ─────────────────────────────────────────────────── */

ChoiceOption * createChoiceOption(char * text, StatementList * body) {
	ChoiceOption * o = calloc(1, sizeof(ChoiceOption));
	o->text = _copyString(text);
	o->body = body;
	return o;
}

void appendChoiceOption(ChoiceOption ** head, ChoiceOption * option) {
	if (*head == NULL) {
		*head = option;
	} else {
		ChoiceOption * cur = *head;
		while (cur->next != NULL) cur = cur->next;
		cur->next = option;
	}
}

/* ── DeclarationList ──────────────────────────────────────────────── */

DeclarationList * createDeclarationList() {
	return (DeclarationList *) calloc(1, sizeof(DeclarationList));
}

void appendDeclaration(DeclarationList * list, Declaration * decl) {
	if (list->head == NULL) {
		list->head = decl;
		list->tail = decl;
	} else {
		list->tail->next = decl;
		list->tail = decl;
	}
}

/* ── Declaration constructors ─────────────────────────────────────── */

Declaration * createCharacterDeclaration(char * displayName, char * identifier, char * color) {
	Declaration * d           = calloc(1, sizeof(Declaration));
	d->type                   = DECL_CHARACTER;
	d->character              = calloc(1, sizeof(CharacterDeclaration));
	d->character->displayName = _copyString(displayName);
	d->character->identifier  = _copyString(identifier);
	d->character->color       = _copyString(color);
	return d;
}

Declaration * createAssetDeclaration(char * identifier, char * filePath) {
	Declaration * d      = calloc(1, sizeof(Declaration));
	d->type              = DECL_ASSET;
	d->asset             = calloc(1, sizeof(AssetDeclaration));
	d->asset->identifier = _copyString(identifier);
	d->asset->filePath   = _copyString(filePath);
	return d;
}

Declaration * createSetDeclaration(char * identifier, AssignOp op, Expression * expr) {
	Declaration * d     = calloc(1, sizeof(Declaration));
	d->type             = DECL_SET;
	d->set              = calloc(1, sizeof(SetStatement));
	d->set->identifier  = _copyString(identifier);
	d->set->op          = op;
	d->set->expr        = expr;
	return d;
}

Declaration * createSceneDeclaration(char * name, StatementList * body) {
	Declaration * d    = calloc(1, sizeof(Declaration));
	d->type            = DECL_SCENE;
	d->scene           = calloc(1, sizeof(SceneDeclaration));
	d->scene->name     = _copyString(name);
	d->scene->body     = body;
	return d;
}

/* ── Program ──────────────────────────────────────────────────────── */

Program * createProgram(DeclarationList * declarations) {
	Program * p     = calloc(1, sizeof(Program));
	p->declarations = declarations;
	return p;
}

/* ── Destructors ──────────────────────────────────────────────────── */

void destroyExpression(Expression * expr) {
	if (expr == NULL) return;
	if (expr->type == EXPR_IDENTIFIER) free(expr->identifier);
	if (expr->type == EXPR_BINARY) {
		destroyExpression(expr->binary.left);
		destroyExpression(expr->binary.right);
	}
	free(expr);
}

void destroyCondition(Condition * cond) {
	if (cond == NULL) return;
	destroyExpression(cond->left);
	destroyExpression(cond->right);
	free(cond);
}

void destroyChoiceOption(ChoiceOption * opt) {
	while (opt != NULL) {
		ChoiceOption * next = opt->next;
		free(opt->text);
		destroyStatementList(opt->body);
		free(opt);
		opt = next;
	}
}

void destroyStatement(Statement * stmt) {
	if (stmt == NULL) return;
	switch (stmt->type) {
		case STMT_DIALOGUE:
			free(stmt->dialogue->actorId);
			free(stmt->dialogue->text);
			free(stmt->dialogue);
			break;
		case STMT_SHOW:
			free(stmt->show->resourceId);
			free(stmt->show);
			break;
		case STMT_HIDE:
			free(stmt->hide->resourceId);
			free(stmt->hide);
			break;
		case STMT_PLAY:
			free(stmt->play->resourceId);
			free(stmt->play);
			break;
		case STMT_STOP:
			free(stmt->stop->resourceId);
			free(stmt->stop);
			break;
		case STMT_GOTO:
			free(stmt->goto_->sceneName);
			free(stmt->goto_);
			break;
		case STMT_SET:
			free(stmt->set->identifier);
			destroyExpression(stmt->set->expr);
			free(stmt->set);
			break;
		case STMT_CHOICE:
			destroyChoiceOption(stmt->choice->options);
			free(stmt->choice);
			break;
		case STMT_IF:
			destroyCondition(stmt->if_->condition);
			destroyStatementList(stmt->if_->thenBlock);
			destroyStatementList(stmt->if_->elseBlock);
			free(stmt->if_);
			break;
		case STMT_END:
			break;
	}
	free(stmt);
}

void destroyStatementList(StatementList * list) {
	if (list == NULL) return;
	Statement * cur = list->head;
	while (cur != NULL) {
		Statement * next = cur->next;
		destroyStatement(cur);
		cur = next;
	}
	free(list);
}

void destroyDeclaration(Declaration * decl) {
	if (decl == NULL) return;
	switch (decl->type) {
		case DECL_CHARACTER:
			free(decl->character->displayName);
			free(decl->character->identifier);
			free(decl->character->color);
			free(decl->character);
			break;
		case DECL_ASSET:
			free(decl->asset->identifier);
			free(decl->asset->filePath);
			free(decl->asset);
			break;
		case DECL_SET:
			free(decl->set->identifier);
			destroyExpression(decl->set->expr);
			free(decl->set);
			break;
		case DECL_SCENE:
			free(decl->scene->name);
			destroyStatementList(decl->scene->body);
			free(decl->scene);
			break;
	}
	free(decl);
}

void destroyDeclarationList(DeclarationList * list) {
	if (list == NULL) return;
	Declaration * cur = list->head;
	while (cur != NULL) {
		Declaration * next = cur->next;
		destroyDeclaration(cur);
		cur = next;
	}
	free(list);
}

void destroyProgram(Program * program) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program == NULL) return;
	destroyDeclarationList(program->declarations);
	free(program);
}
