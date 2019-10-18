#pragma once

#include "tokenizer.h"

#define MAX_BLOCK_SIZE 1024

typedef enum {
  ST_ROOT,
  ST_FUNC,
  ST_ARGS,
  ST_BLOCK,
  ST_NUM,
  ST_ADD,
  ST_SUB,
  ST_MUL,
  ST_DIV,
  ST_LET,
  ST_RET,
  ST_ASSIGN,
  ST_VAR,
  ST_EQUAL,
  ST_NOT_EQUAL,
  ST_LT,
  ST_LTEQ,
  ST_GT,
  ST_GTEQ,
} SyntaxType;

typedef struct tAST {
  SyntaxType type;
  Token* token;
  long val;
  struct tAST* children[MAX_BLOCK_SIZE];
} AST;

typedef struct {
  AST* ast;
  Token* root;
  Token* current;
} Parser;

Parser* parse(Token* token);
AST* get_lhs(AST* node);
AST* get_rhs(AST* node);
void print_ast(AST* ast, size_t level);
