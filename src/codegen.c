#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "codegen.h"
#include "parser.h"

CodeGen* create_codegen(FILE* fp, bool debug) {
  CodeGen* g = (CodeGen*)malloc(sizeof(CodeGen));
  g->output = fp;
  g->index = 0;
  g->label_index = 0;
  g->debug = debug;
  return g;
}

static void comment(CodeGen* g, const char* format, ...) {
  if( !g->debug ) return;

  va_list va;
  va_start(va, format);
  vfprintf(g->output, format, va);
  va_end(va);
}

static void gen(CodeGen* g, const char* format, ...) {
  va_list va;
  va_start(va, format);
  vfprintf(g->output, format, va);
  va_end(va);
}

static size_t gen_alloca(CodeGen* g) {
  const size_t mem = ++(g->index);
  gen(g, "  %%%zu = alloca i32, align 4\n", mem);
  return mem;
}

static void gen_named_alloca(CodeGen* g, Token* token) {
  gen(g, "  %%%.*s = alloca i32, align 4\n", token->len, token->buffer + token->pos);
}

static size_t gen_zext(CodeGen* g, const char* from, const char* to, size_t before) {
  const size_t after = ++(g->index);
  gen(g, "  %%%zu = zext %s %%%zu to %s\n", after, from, before, to);
  return after;
}

static void gen_store(CodeGen* g, size_t reg, size_t mem) {
  gen(g, "  store i32 %%%zu, i32* %%%zu, align 4\n", reg, mem);
}

static void gen_store_immediate(CodeGen* g, long long imm, size_t mem) {
  gen(g, "  store i32 %ld, i32* %%%zu\n", imm, mem);
}

static size_t gen_load(CodeGen* g, size_t src) {
  const size_t dst = ++(g->index);
  gen(g, "  %%%zu = load i32, i32* %%%zu, align 4\n", dst, src);
  return dst;
}

static size_t gen_named_load(CodeGen* g, Token* token) {
  const size_t dst = ++(g->index);
  gen(g, "  %%%zu = load i32, i32* %%%.*s, align 4\n", dst, token->len, token->buffer + token->pos);
  return dst;
}

static size_t gen_add(CodeGen* g, size_t lhs, size_t rhs) {
  const size_t reg = ++(g->index);
  gen(g, "  %%%zu = add i32 %%%zu, %%%zu\n", reg, lhs, rhs);
  return reg;
}

static size_t gen_sub(CodeGen* g, size_t lhs, size_t rhs) {
  const size_t reg = ++(g->index);
  gen(g, "  %%%zu = sub i32 %%%zu, %%%zu\n", reg, lhs, rhs);
  return reg;
}

static size_t gen_mul(CodeGen* g, size_t lhs, size_t rhs) {
  const size_t reg = ++(g->index);
  gen(g, "  %%%zu = mul i32 %%%zu, %%%zu\n", reg, lhs, rhs);
  return reg;
}

static size_t gen_sdiv(CodeGen* g, size_t lhs, size_t rhs) {
  const size_t reg = ++(g->index);
  gen(g, "  %%%zu = sdiv i32 %%%zu, %%%zu\n", reg, lhs, rhs);
  return reg;
}

static size_t gen_cmp(CodeGen* g, const char* cmp, size_t lhs, size_t rhs) {
  const size_t reg = ++(g->index);
  gen(g, "  %%%zu = icmp %s i32 %%%zu, %%%zu\n", reg, cmp, lhs, rhs);
  return reg;
}

static void gen_named_store(CodeGen* g, Token* lvar, size_t reg) {
  gen(g, "  store i32 %%%zu, i32* %%%.*s, align 4\n", reg, lvar->len, lvar->buffer + lvar->pos);
}

static size_t gen_func_define_name(CodeGen* g, Token* name) {
  gen(g, "define i32 @%.*s(", name->len, name->buffer + name->pos);
  return g->index;
}

static size_t gen_func_define_arg(CodeGen* g){
  if( g->index != 0 ) gen(g, ", ");
  gen(g, "i32");
  return ++(g->index);
}

static size_t gen_func_start(CodeGen* g) {
  gen(g, ") nounwind {\n");
  return g->index;
}

static size_t gen_func_start_arg(CodeGen* g, size_t arg_reg, Token* name) {
  gen_named_alloca(g, name);
  gen_named_store(g, name, arg_reg);
  return g->index;
}

static size_t gen_func_end(CodeGen* g, size_t result_reg) {
  comment(g, "  ; ------------- Returning result\n");
  gen(g, "  ret i32 %%%zu\n", result_reg);
  gen(g, "}\n");
  return g->index;
}

static size_t gen_call(CodeGen* g, Token* ident, size_t reg) {
  gen(g, "  call i32 (i32) @%.*s(i32 %%%zu)\n", ident->len, ident->buffer + ident->pos, reg);
  return ++(g->index);
}


static size_t gen_block(CodeGen* g, AST* ast) {
  switch( ast->type ) {
    case ST_NUM: {
      comment(g, "  ; ST_NUM\n");
      const size_t mem = gen_alloca(g);
      gen_store_immediate(g, ast->val, mem);
      const size_t reg = gen_load(g, mem);
      return reg;
    }
    break;
    case ST_LET: {
      comment(g, "  ; ST_LET\n");
      gen_named_alloca(g, get_lhs(ast)->token);
      if( get_rhs(ast) ) {
        const size_t num_reg = gen_block(g, get_rhs(ast));
        gen_named_store(g, get_lhs(ast)->token, num_reg);
      }
      const size_t reg = gen_named_load(g, get_lhs(ast)->token);
      return reg;
    }
    break;
    case ST_VAR: {
      comment(g, "  ; ST_VAR\n");
      const size_t reg = gen_named_load(g, ast->token);
      return reg;
    }
    break;
    case ST_CALL: {
      comment(g, "  ; ST_CALL\n");
      Token* ident = ast->token;
      // Currently , Only single argument function is implemented
      // TODO: multiple arguments
      const size_t reg = gen_block(g, get_lhs(ast));
      return gen_call(g, ident, reg);
    }
    break;
    case ST_RETURN: {
      comment(g, "  ; ST_RETURN\n");
      const size_t reg = gen_block(g, get_lhs(ast));
      gen(g, "  ret i32 %%%zu\n", reg);
      g->index++; // ret increment variable index (for label variable)
      return reg;
    }
    break;
    case ST_IF: {
      comment(g, "  ; ST_IF\n");
      AST* cond = ast->children[0];

      const size_t if_true_label = ++g->label_index;
      const size_t if_false_label = ++g->label_index;
      const size_t if_end_label = ++g->label_index;

      // condition
      const size_t cmp_reg = gen_block(g, cond);
      gen(g, "  %%%zu = icmp ne i32 %%%zu, 0\n", ++(g->index), cmp_reg);
      gen(g, "  br i1 %%%zu, label %%label.%zu, label %%label.%zu\n", g->index, if_true_label, if_false_label);

      // when true
      gen(g, "label.%zu:\n", if_true_label);
      AST* when_true = ast->children[1];
      const size_t before_true = g->label_index;
      const size_t if_true_reg = gen_block(g, when_true);
      size_t if_true_end_label = g->label_index;
      if( if_true_end_label == before_true ) if_true_end_label = if_true_label;
      gen(g, "  br label %%label.%zu\n", if_end_label);

      // when false
      gen(g, "label.%zu:\n", if_false_label);
      AST* when_false = ast->children[2];
      const size_t before_false = g->label_index;
      const size_t if_false_reg = gen_block(g, when_false);
      size_t if_false_end_label = g->label_index;
      if( if_false_end_label == before_false ) if_false_end_label = if_false_label;
      gen(g, "  br label %%label.%zu\n", if_end_label);

      gen(g, "label.%zu:\n", if_end_label);
      const size_t result_reg = ++(g->index);
      gen(g, "  %%%zu = phi i32 [ %%%zu, %%label.%zu ], [ %%%zu, %%label.%zu ]\n", result_reg, if_true_reg, if_true_end_label, if_false_reg, if_false_end_label);
      return result_reg;
    }
    break;
    case ST_BLOCK: {
      comment(g, "  ; ST_BLOCK\n");
      size_t result_reg;
      for( AST** current = ast->children; *current; ++current ) {
        result_reg = gen_block(g, *current);
      }
      return result_reg;
    }
    break;
    default:
      // 特にすることない
    break;
  }

  comment(g, "  ; ------------- Calculate LHS\n");
  const size_t lhs_reg = gen_block(g, get_lhs(ast));

  comment(g, "  ; ------------- Calculate RHS\n");
  const size_t rhs_reg = gen_block(g, get_rhs(ast));

  switch( ast->type ) {
  case ST_ADD:
    {
      comment(g, "  ; ------------- Calculate ST_ADD\n");
      return gen_add(g, lhs_reg, rhs_reg);
    }
    break;
  case ST_SUB:
    {
      comment(g, "  ; ------------- Calculate ST_SUB\n");
      return gen_sub(g, lhs_reg, rhs_reg);
    }
    break;
  case ST_MUL:
    {
      comment(g, "  ; ------------- Calculate ST_MUL\n");
      return gen_mul(g, lhs_reg, rhs_reg);
    }
    break;
  case ST_DIV:
    {
      comment(g, "  ; ------------- Calculate ST_DIV\n");
      return gen_sdiv(g, lhs_reg, rhs_reg);
    }
    break;
  case ST_EQUAL:
    {
      comment(g, "  ; ------------- Calculate ST_EQUAL\n");
      const size_t cmp_reg = gen_cmp(g, "eq", lhs_reg, rhs_reg);
      return gen_zext(g, "i1", "i32", cmp_reg);
    }
    break;
  case ST_NOT_EQUAL:
    {
      comment(g, "  ; ------------- Calculate ST_NOT_EQUAL\n");
      const size_t cmp_reg = gen_cmp(g, "ne", lhs_reg, rhs_reg);
      return gen_zext(g, "i1", "i32", cmp_reg);
    }
    break;
  case ST_LT:
    {
      comment(g, "  ; ------------- Calculate ST_LT\n");
      const size_t cmp_reg = gen_cmp(g, "slt", lhs_reg, rhs_reg);
      return gen_zext(g, "i1", "i32", cmp_reg);
    }
    break;
  case ST_LTEQ:
    {
      comment(g, "  ; ------------- Calculate ST_LTEQ\n");
      const size_t cmp_reg = gen_cmp(g, "sle", lhs_reg, rhs_reg);
      return gen_zext(g, "i1", "i32", cmp_reg);
    }
    break;
  case ST_GT:
    {
      comment(g, "  ; ------------- Calculate ST_GT\n");
      const size_t cmp_reg = gen_cmp(g, "sgt", lhs_reg, rhs_reg);
      return gen_zext(g, "i1", "i32", cmp_reg);
    }
    break;
  case ST_GTEQ:
    {
      comment(g, "  ; ------------- Calculate ST_GTEQ\n");
      const size_t cmp_reg = gen_cmp(g, "sge", lhs_reg, rhs_reg);
      return gen_zext(g, "i1", "i32", cmp_reg);
    }
    break;
  default:
    return g->index;
  }

  return g->index;
}

void generate_func(CodeGen* g, AST* func) {
  gen_func_define_name(g, func->token);
  AST* args = get_lhs(func);
  // reset variable index!!
  g->index = 0;
  g->label_index = 0;
  for( AST** arg = args->children; *arg; ++arg ) {
    gen_func_define_arg(g);
  }
  gen_func_start(g);

  comment(g, "; --------- Store args\n");
  // reset variable index!!
  g->index = 0;
  g->label_index = 0;
  // args
  for( AST** arg = args->children; *arg; ++arg ) {
    gen_func_start_arg(g, g->index++, (*arg)->token);
  }
  size_t result_reg = gen_block(g, get_rhs(func));
  gen_func_end(g, result_reg);
}

void generate_header(CodeGen* g) {
  gen(g, "%%FILE = type opaque\n");
  gen(g, "@__stdinp = external global %%FILE*, align 8\n");
  gen(g, "@__stdoutp = external global %%FILE*, align 8\n");
  gen(g, "@__stderrp = external global %%FILE*, align 8\n");
  gen(g, "\n");

  gen(g, "@str = private unnamed_addr constant [4 x i8] c\"%%d\\0A\\00\", align 1\n");
  gen(g, "\n");

  gen(g, "declare i32 @fprintf(%%FILE*, i8*, ...)\n");
  gen(g, "declare i32 @printf(i8*, ...)\n");
  gen(g, "declare i32 @atoi(...)\n");

  gen(g, "define i32 @print(i32) nounwind {\n");
  gen(g, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i64 0, i64 0), i32 %%0)\n");
  gen(g, "  ret i32 %%0\n");
  gen(g, "}\n");
  gen(g, "\n");
}

void generate_code(CodeGen* g, AST* root) {
  generate_header(g);

  size_t result_reg;
  for( AST** current = root->children; *current; ++current ) {
    generate_func(g, *current);
  }
}

