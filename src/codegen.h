#pragma once

#include "parser.h"

typedef struct {
  char const* output;
  char* current;
  size_t len;
  size_t index;
} CodeGen;

CodeGen* create_codegen(size_t len);
void generate_code(CodeGen* gen, AST* ast);
