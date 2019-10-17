#pragma once

#include <stdbool.h>

#include "parser.h"

typedef struct {
  FILE* output;
  size_t index;
  bool debug;
} CodeGen;

CodeGen* create_codegen(FILE* output, bool debug);
void generate_code(CodeGen* gen, AST* root);
