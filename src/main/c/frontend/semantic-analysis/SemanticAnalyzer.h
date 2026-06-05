#ifndef SEMANTIC_ANALYZER_HEADER
#define SEMANTIC_ANALYZER_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/ModuleDestructor.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include <stdlib.h>

ModuleDestructor initializeSemanticAnalyzerModule(CompilerState * compilerState);

CompilationStatus executeSemanticAnalysis(CompilerState * compilerState);

#endif
