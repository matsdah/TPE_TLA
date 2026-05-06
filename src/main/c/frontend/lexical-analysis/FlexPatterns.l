%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

extern CompilationStatus UnknownLexemeAction();
extern CompilationStatus pushTokenLabel(TokenLabel label);
extern CompilationStatus pushTokenLabelAndEnterContext(TokenLabel label, FlexContext context);
extern CompilationStatus pushTokenLabelAndExitContext(TokenLabel label);
extern CompilationStatus EOFLexemeAction();

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x IMPORT_EXPRESSION
%x MULTILINE_COMMENT
%x VARIABLE_DEF_CONTEXT


/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace			[[:space:]]+

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}

"{"									{ return pushTokenLabelAndEnterContext(OPEN_BRACE, IMPORT_EXPRESSION) }
<IMPORT_EXPRESSION>[^{}]+			{ return SubexpressionLexemeAction(); }
<IMPORT_EXPRESSION>"}"				{ return pushTokenLabelAndExitContext(CLOSE_BRACE); }
<IMPORT_EXPRESSION>.				{ return UnknownLexemeAction(); }

"/*"								{ return pushTokenLabelAndEnterContext(OPEN_COMMENT, MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/"				{ return pushTokenLabelAndExitContext(CLOSE_COMMENT); }
<MULTILINE_COMMENT>[^*]+			{ return pushTokenLabel(IGNORED); }
<MULTILINE_COMMENT>.				{ return pushTokenLabel(IGNORED); }

"-"									{ return ArithmeticOperatorLexemeAction(SUB); }
"*"									{ return ArithmeticOperatorLexemeAction(MUL); }
"/"									{ return ArithmeticOperatorLexemeAction(DIV); }
"+"									{ return ArithmeticOperatorLexemeAction(ADD); }

"("									{ return ParenthesisLexemeAction(OPEN_PARENTHESIS); }
")"									{ return ParenthesisLexemeAction(CLOSE_PARENTHESIS); }

[[:digit:]]+						{ return IntegerLexemeAction(); }

{whitespace}						{ return IgnoredLexemeAction(); }
.									{ return UnknownLexemeAction(); }




"character"                                             { return CreateVar(VARIABLE_DEF_CONTEXT, CHARACTER ); }
"as"                                                    { return AS}
<CHARACTER_CONTEXT>[a-zA-Z][a-zA-Z0-9_]*                { return CreateID();                }
<CHARACTER_CONTEXT>";"                                  { return ExitCharacter_Context();   }





<<EOF>>								{ return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"
