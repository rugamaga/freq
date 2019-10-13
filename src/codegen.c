#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "codegen.h"
#include "parser.h"

CodeGen* create_codegen(FILE* fp, bool debug) {
  CodeGen* g = (CodeGen*)malloc(sizeof(CodeGen));
  g->output = fp;
  g->index = 0;
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

static size_t gen_numeric_process(CodeGen* g, AST* ast) {
  if (ast->type == ST_NUM) {
    comment(g, "  ; Assign ST_NUM\n");
    const size_t mem = gen_alloca(g);
    gen_store_immediate(g, ast->val, mem);
    gen(g, "\n");
    return mem;
  }

  comment(g, "  ; ------------- Calculate LHS\n");
  const size_t lhs = gen_numeric_process(g, ast->lhs);

  comment(g, "  ; ------------- Calculate RHS\n");
  const size_t rhs = gen_numeric_process(g, ast->rhs);

  switch( ast->type ) {
  case ST_NUM:
    // 処理済み
    break;
  case ST_ADD:
    {
      comment(g, "  ; ------------- Calculate ST_ADD\n");
      const size_t lhs_reg = gen_load(g, lhs);
      const size_t rhs_reg = gen_load(g, rhs);
      const size_t add_reg = gen_add(g, lhs_reg, rhs_reg);
      const size_t res_mem = gen_alloca(g);
      gen_store(g, add_reg, res_mem);
    }
    break;
  case ST_SUB:
    {
      comment(g, "  ; ------------- Calculate ST_SUB\n");
      const size_t lhs_reg = gen_load(g, lhs);
      const size_t rhs_reg = gen_load(g, rhs);
      const size_t sub_reg = gen_sub(g, lhs_reg, rhs_reg);
      const size_t res_mem = gen_alloca(g);
      gen_store(g, sub_reg, res_mem);
    }
    break;
  case ST_MUL:
    {
      comment(g, "  ; ------------- Calculate ST_MUL\n");
      const size_t lhs_reg = gen_load(g, lhs);
      const size_t rhs_reg = gen_load(g, rhs);
      const size_t mul_reg = gen_mul(g, lhs_reg, rhs_reg);
      const size_t res_mem = gen_alloca(g);
      gen_store(g, mul_reg, res_mem);
    }
    break;
  case ST_DIV:
    {
      comment(g, "  ; ------------- Calculate ST_DIV\n");
      const size_t lhs_reg = gen_load(g, lhs);
      const size_t rhs_reg = gen_load(g, rhs);
      const size_t div_reg = gen_sdiv(g, lhs_reg, rhs_reg);
      const size_t res_mem = gen_alloca(g);
      gen_store(g, div_reg, res_mem);
    }
    break;
  case ST_EQUAL:
    {
      comment(g, "  ; ------------- Calculate ST_EQUAL\n");
      const size_t lhs_reg = gen_load(g, lhs);
      const size_t rhs_reg = gen_load(g, rhs);
      const size_t cmp_reg = gen_cmp(g, "eq", lhs_reg, rhs_reg);
      const size_t zext_reg = gen_zext(g, "i1", "i32", cmp_reg);
      const size_t res_mem = gen_alloca(g);
      gen_store(g, zext_reg, res_mem);
    }
    break;
  case ST_NOT_EQUAL:
    {
      comment(g, "  ; ------------- Calculate ST_NOT_EQUAL\n");
      const size_t lhs_reg = gen_load(g, lhs);
      const size_t rhs_reg = gen_load(g, rhs);
      const size_t cmp_reg = gen_cmp(g, "ne", lhs_reg, rhs_reg);
      const size_t zext_reg = gen_zext(g, "i1", "i32", cmp_reg);
      const size_t res_mem = gen_alloca(g);
      gen_store(g, zext_reg, res_mem);
    }
    break;
  }

  gen(g, "\n");
  return g->index;
}

void generate_code(CodeGen* g, AST* ast) {
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
  gen(g, "\n");

  gen(g, "define i32 @main() nounwind {\n");

  const size_t num_proc_last = gen_numeric_process(g, ast);

  comment(g, "  ; ------------- Output result\n");
  const size_t result_reg = gen_load(g, num_proc_last);
  gen(g, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i64 0, i64 0), i32 %%%zu)\n", result_reg);
  gen(g, "\n");

  gen(g, "  ret i32 %%%zu\n", result_reg);
  gen(g, "}\n");
}

