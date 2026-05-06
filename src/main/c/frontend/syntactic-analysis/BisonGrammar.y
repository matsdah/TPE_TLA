%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

/**
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 */
void yyerror(const YYLTYPE * location, const char * message) {}

%}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	/** Terminals. */
	signed int integer;
	char *     string;
	TokenLabel token;

	/** Non-terminals. */
	Program *         program;
	DeclarationList * declarationList;
	Declaration *     declaration;
	StatementList *   statementList;
	Statement *       statement;
	ChoiceOption *    choiceOption;
	Condition *       condition;
	Expression *      expression;
}

/**
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
%destructor { free($$); }                 <string>
%destructor { destroyExpression($$); }    <expression>
%destructor { destroyCondition($$); }     <condition>
%destructor { destroyStatement($$); }     <statement>
%destructor { destroyStatementList($$); } <statementList>
%destructor { destroyChoiceOption($$); }  <choiceOption>
%destructor { destroyDeclaration($$); }   <declaration>
%destructor { destroyDeclarationList($$); } <declarationList>

/** Terminals — literals. */
%token <integer> INTEGER
%token <string>  STRING
%token <string>  IDENTIFIER

/** Terminals — keywords. */
%token <token> CHARACTER
%token <token> AS
%token <token> COLOR
%token <token> ASSET
%token <token> SCENE
%token <token> SET
%token <token> SHOW
%token <token> HIDE
%token <token> BACKGROUND
%token <token> SPRITE
%token <token> PLAY
%token <token> STOP
%token <token> MUSIC
%token <token> SOUND
%token <token> CHOICE
%token <token> IF
%token <token> ELSE
%token <token> GOTO
%token <token> END

/** Terminals — operators. */
%token <token> ASSIGN
%token <token> ASSIGN_ADD
%token <token> ASSIGN_SUB
%token <token> LESS
%token <token> GREATER
%token <token> LESS_EQ
%token <token> GREATER_EQ
%token <token> EQUAL
%token <token> NOT_EQUAL
%token <token> ADD
%token <token> SUB
%token <token> MUL
%token <token> DIV

/** Terminals — punctuation. */
%token <token> OPEN_BRACE
%token <token> CLOSE_BRACE
%token <token> OPEN_PARENTHESIS
%token <token> CLOSE_PARENTHESIS
%token <token> SEMICOLON

/** Terminals — internal. */
%token <token> OPEN_COMMENT
%token <token> CLOSE_COMMENT
%token <token> IGNORED
%token <token> UNKNOWN

/** Non-terminals. */
%type <program>         program
%type <declarationList> declarationList
%type <declaration>     declaration
%type <statementList>   statementList
%type <statement>       statement
%type <statement>       dialogueStatement
%type <statement>       showStatement
%type <statement>       hideStatement
%type <statement>       playStatement
%type <statement>       stopStatement
%type <statement>       gotoStatement
%type <statement>       setStatement
%type <statement>       choiceStatement
%type <statement>       ifStatement
%type <statement>       endStatement
%type <choiceOption>    optionList
%type <choiceOption>    option
%type <condition>       condition
%type <expression>      expression

/**
 * Precedencia y asociatividad (lowest to highest).
 *
 * @see https://en.cppreference.com/w/cpp/language/operator_precedence.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Precedence.html
 */
%right ELSE
%left LESS GREATER LESS_EQ GREATER_EQ EQUAL NOT_EQUAL
%left ADD SUB
%left MUL DIV

%%

// $N -> # args.

program: declarationList			{ $$ = ProgramSemanticAction($1); }
	;

declarationList: %empty			{ $$ = EmptyDeclarationListSemanticAction(); }
	| declarationList declaration	{ $$ = AppendDeclarationSemanticAction($1, $2); }
	;

declaration: CHARACTER STRING AS IDENTIFIER COLOR STRING SEMICOLON	{ $$ = CharacterDeclarationSemanticAction($2, $4, $6); free($2); free($4); free($6); }
	| ASSET IDENTIFIER ASSIGN STRING SEMICOLON						{ $$ = AssetDeclarationSemanticAction($2, $4); free($2); free($4); }
	| SET IDENTIFIER ASSIGN expression SEMICOLON					{ $$ = SetDeclarationSemanticAction($2, ASSIGN_OP, $4); free($2); }
	| SET IDENTIFIER ASSIGN_ADD expression SEMICOLON 				{ $$ = SetDeclarationSemanticAction($2, ASSIGN_ADD_OP, $4); free($2); }
	| SET IDENTIFIER ASSIGN_SUB expression SEMICOLON 				{ $$ = SetDeclarationSemanticAction($2, ASSIGN_SUB_OP, $4); free($2); }
	| SCENE STRING OPEN_BRACE statementList CLOSE_BRACE				{ $$ = SceneDeclarationSemanticAction($2, $4); free($2); }
	;

statementList: %empty								{ $$ = EmptyStatementListSemanticAction(); }
	| statementList statement						{ $$ = AppendStatementSemanticAction($1, $2); }
	;

statement: dialogueStatement	{ $$ = $1; }
	| showStatement				{ $$ = $1; }
	| hideStatement				{ $$ = $1; }
	| playStatement				{ $$ = $1; }
	| stopStatement				{ $$ = $1; }
	| gotoStatement				{ $$ = $1; }
	| setStatement				{ $$ = $1; }
	| choiceStatement			{ $$ = $1; }
	| ifStatement				{ $$ = $1; }
	| endStatement				{ $$ = $1; }
	;

dialogueStatement: IDENTIFIER STRING SEMICOLON			{ $$ = DialogueStatementSemanticAction($1, $2); free($1); free($2); }
	;

showStatement: SHOW BACKGROUND IDENTIFIER SEMICOLON		{ $$ = ShowStatementSemanticAction(DISPLAY_BACKGROUND, $3); free($3); }
	| SHOW SPRITE IDENTIFIER SEMICOLON					{ $$ = ShowStatementSemanticAction(DISPLAY_SPRITE, $3); free($3); }
	;

hideStatement: HIDE BACKGROUND IDENTIFIER SEMICOLON		{ $$ = HideStatementSemanticAction(DISPLAY_BACKGROUND, $3); free($3); }
	| HIDE SPRITE IDENTIFIER SEMICOLON					{ $$ = HideStatementSemanticAction(DISPLAY_SPRITE, $3); free($3); }
	;

playStatement: PLAY MUSIC IDENTIFIER SEMICOLON			{ $$ = PlayStatementSemanticAction(AUDIO_MUSIC, $3); free($3); }
	| PLAY SOUND IDENTIFIER SEMICOLON					{ $$ = PlayStatementSemanticAction(AUDIO_SOUND, $3); free($3); }
	;

stopStatement: STOP MUSIC IDENTIFIER SEMICOLON			{ $$ = StopStatementSemanticAction(AUDIO_MUSIC, $3); free($3); }
	| STOP SOUND IDENTIFIER SEMICOLON					{ $$ = StopStatementSemanticAction(AUDIO_SOUND, $3); free($3); }
	;

gotoStatement: GOTO STRING SEMICOLON					{ $$ = GotoStatementSemanticAction($2); free($2); }
	;

setStatement: SET IDENTIFIER ASSIGN expression SEMICOLON	{ $$ = SetStatementSemanticAction($2, ASSIGN_OP, $4); free($2); }
	| SET IDENTIFIER ASSIGN_ADD expression SEMICOLON 		{ $$ = SetStatementSemanticAction($2, ASSIGN_ADD_OP, $4); free($2); }
	| SET IDENTIFIER ASSIGN_SUB expression SEMICOLON 		{ $$ = SetStatementSemanticAction($2, ASSIGN_SUB_OP, $4); free($2); }
	;

choiceStatement: CHOICE OPEN_BRACE optionList CLOSE_BRACE		{ $$ = ChoiceStatementSemanticAction($3); }
	;

optionList: option			{ $$ = $1; }
	| optionList option		{ $$ = AppendChoiceOptionSemanticAction($1, $2); }
	;

option: STRING OPEN_BRACE statementList CLOSE_BRACE		{ $$ = ChoiceOptionSemanticAction($1, $3); free($1); }
	;

ifStatement: IF OPEN_PARENTHESIS condition CLOSE_PARENTHESIS OPEN_BRACE statementList CLOSE_BRACE										{ $$ = IfStatementSemanticAction($3, $6, NULL); }
	| IF OPEN_PARENTHESIS condition CLOSE_PARENTHESIS OPEN_BRACE statementList CLOSE_BRACE ELSE OPEN_BRACE statementList CLOSE_BRACE	{ $$ = IfStatementSemanticAction($3, $6, $10); }
	;

endStatement: END SEMICOLON			{ $$ = EndStatementSemanticAction(); }
	;

condition: expression[l] LESS expression[r]			{ $$ = ConditionSemanticAction($l, CMP_LT, $r); }
	| expression[l] GREATER expression[r]			{ $$ = ConditionSemanticAction($l, CMP_GT, $r); }
	| expression[l] LESS_EQ expression[r]			{ $$ = ConditionSemanticAction($l, CMP_LE, $r); }
	| expression[l] GREATER_EQ expression[r]		{ $$ = ConditionSemanticAction($l, CMP_GE, $r); }
	| expression[l] EQUAL expression[r]				{ $$ = ConditionSemanticAction($l, CMP_EQ, $r); }
	| expression[l] NOT_EQUAL expression[r]			{ $$ = ConditionSemanticAction($l, CMP_NE, $r); }
	;

expression: expression[l] ADD expression[r]			{ $$ = BinaryExpressionSemanticAction($l, ARITH_ADD, $r); }
	| expression[l] SUB expression[r]				{ $$ = BinaryExpressionSemanticAction($l, ARITH_SUB, $r); }
	| expression[l] MUL expression[r]				{ $$ = BinaryExpressionSemanticAction($l, ARITH_MUL, $r); }
	| expression[l] DIV expression[r]				{ $$ = BinaryExpressionSemanticAction($l, ARITH_DIV, $r); }
	| OPEN_PARENTHESIS expression CLOSE_PARENTHESIS	{ $$ = $2; }
	| INTEGER										{ $$ = IntegerExpressionSemanticAction($1); }
	| IDENTIFIER									{ $$ = IdentifierExpressionSemanticAction($1); free($1); }
	;

%%
