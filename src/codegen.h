#pragma once

#include "parser.h"

typedef struct {
  FILE* output;
  size_t index;
} CodeGen;

CodeGen* create_codegen(FILE* output);
void generate_code(CodeGen* gen, AST* ast);
