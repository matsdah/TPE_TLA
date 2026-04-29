#include "BisonActions.h"

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: BisonActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	return _shutdownBisonActionsModule;
}

static void _log(const char * fn) { logDebugging(_logger, "%s", fn); }

/* ── Expressions ──────────────────────────────────────────────────── */

Expression * IntegerExpressionSemanticAction(int value) {
	_log(__FUNCTION__);
	return createIntegerExpression(value);
}

Expression * IdentifierExpressionSemanticAction(char * identifier) {
	_log(__FUNCTION__);
	return createIdentifierExpression(identifier);
}

Expression * BinaryExpressionSemanticAction(Expression * left, ArithOp op, Expression * right) {
	_log(__FUNCTION__);
	return createBinaryExpression(left, op, right);
}

/* ── Condition ────────────────────────────────────────────────────── */

Condition * ConditionSemanticAction(Expression * left, ComparisonOp op, Expression * right) {
	_log(__FUNCTION__);
	return createCondition(left, op, right);
}

/* ── Statements ───────────────────────────────────────────────────── */

Statement * DialogueStatementSemanticAction(char * actorId, char * text) {
	_log(__FUNCTION__);
	return createDialogueStatement(actorId, text);
}

Statement * ShowStatementSemanticAction(ResourceDisplayType type, char * resourceId) {
	_log(__FUNCTION__);
	return createShowStatement(type, resourceId);
}

Statement * HideStatementSemanticAction(ResourceDisplayType type, char * resourceId) {
	_log(__FUNCTION__);
	return createHideStatement(type, resourceId);
}

Statement * PlayStatementSemanticAction(ResourceAudioType type, char * resourceId) {
	_log(__FUNCTION__);
	return createPlayStatement(type, resourceId);
}

Statement * StopStatementSemanticAction(ResourceAudioType type, char * resourceId) {
	_log(__FUNCTION__);
	return createStopStatement(type, resourceId);
}

Statement * GotoStatementSemanticAction(char * sceneName) {
	_log(__FUNCTION__);
	return createGotoStatement(sceneName);
}

Statement * SetStatementSemanticAction(char * identifier, AssignOp op, Expression * expr) {
	_log(__FUNCTION__);
	return createSetStatement(identifier, op, expr);
}

Statement * ChoiceStatementSemanticAction(ChoiceOption * options) {
	_log(__FUNCTION__);
	return createChoiceStatement(options);
}

Statement * IfStatementSemanticAction(Condition * cond, StatementList * thenBlock, StatementList * elseBlock) {
	_log(__FUNCTION__);
	return createIfStatement(cond, thenBlock, elseBlock);
}

Statement * EndStatementSemanticAction() {
	_log(__FUNCTION__);
	return createEndStatement();
}

/* ── StatementList ────────────────────────────────────────────────── */

StatementList * EmptyStatementListSemanticAction() {
	_log(__FUNCTION__);
	return createStatementList();
}

StatementList * AppendStatementSemanticAction(StatementList * list, Statement * stmt) {
	_log(__FUNCTION__);
	appendStatement(list, stmt);
	return list;
}

/* ── ChoiceOption ─────────────────────────────────────────────────── */

ChoiceOption * ChoiceOptionSemanticAction(char * text, StatementList * body) {
	_log(__FUNCTION__);
	return createChoiceOption(text, body);
}

ChoiceOption * AppendChoiceOptionSemanticAction(ChoiceOption * list, ChoiceOption * option) {
	_log(__FUNCTION__);
	appendChoiceOption(&list, option);
	return list;
}

/* ── Declarations ─────────────────────────────────────────────────── */

Declaration * CharacterDeclarationSemanticAction(char * displayName, char * identifier, char * color) {
	_log(__FUNCTION__);
	return createCharacterDeclaration(displayName, identifier, color);
}

Declaration * AssetDeclarationSemanticAction(char * identifier, char * filePath) {
	_log(__FUNCTION__);
	return createAssetDeclaration(identifier, filePath);
}

Declaration * SetDeclarationSemanticAction(char * identifier, AssignOp op, Expression * expr) {
	_log(__FUNCTION__);
	return createSetDeclaration(identifier, op, expr);
}

Declaration * SceneDeclarationSemanticAction(char * name, StatementList * body) {
	_log(__FUNCTION__);
	return createSceneDeclaration(name, body);
}

/* ── DeclarationList ──────────────────────────────────────────────── */

DeclarationList * EmptyDeclarationListSemanticAction() {
	_log(__FUNCTION__);
	return createDeclarationList();
}

DeclarationList * AppendDeclarationSemanticAction(DeclarationList * list, Declaration * decl) {
	_log(__FUNCTION__);
	appendDeclaration(list, decl);
	return list;
}

/* ── Program ──────────────────────────────────────────────────────── */

Program * ProgramSemanticAction(DeclarationList * declarations) {
	_log(__FUNCTION__);
	Program * program = createProgram(declarations);
	_compilerState->abstractSyntaxtTree = program;
	return program;
}
