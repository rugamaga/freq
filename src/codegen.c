#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "codegen.h"
#include "parser.h"

CodeGen* create_codegen(FILE* fp) {
  CodeGen* g = (CodeGen*)malloc(sizeof(CodeGen));
  g->output = fp;
  g->index = 0;
  return g;
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

static size_t gen_numeric_process(CodeGen* g, AST* ast) {
  if (ast->type == ST_NUM) {
    gen(g, "  ; Assign ST_NUM\n");
    const size_t alloc_index = gen_alloca(g);
    gen_store_immediate(g, ast->val, alloc_index);
    gen(g, "\n");
    return g->index;
  }

  gen(g, "  ; ------------- Calculate LHS\n");
  const size_t lhs_index = gen_numeric_process(g, ast->lhs);

  gen(g, "  ; ------------- Calculate RHS\n");
  const size_t rhs_index = gen_numeric_process(g, ast->rhs);

  switch( ast->type ) {
  case ST_NUM:
    // 処理済み
    break;
  case ST_ADD:
    {
      gen(g, "  ; ------------- Calculate ST_ADD\n");
      const size_t lhs_reg_index = gen_load(g, lhs_index);
      const size_t rhs_reg_index = gen_load(g, rhs_index);
      const size_t add_reg_index = gen_add(g, lhs_reg_index, rhs_reg_index);
      const size_t res_mem_index = gen_alloca(g);
      gen_store(g, add_reg_index, res_mem_index);
    }
    break;
  case ST_SUB:
    {
      gen(g, "  ; ------------- Calculate ST_ADD\n");
      const size_t lhs_reg_index = gen_load(g, lhs_index);
      const size_t rhs_reg_index = gen_load(g, rhs_index);
      const size_t sub_reg_index = gen_sub(g, lhs_reg_index, rhs_reg_index);
      const size_t res_mem_index = gen_alloca(g);
      gen_store(g, sub_reg_index, res_mem_index);
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

  const size_t num_proc_last_index = gen_numeric_process(g, ast);
  const size_t reg_index = ++(g->index);

  gen(g, "  ; ------------- Output result\n");
  gen(g, "  %%%zu = load i32, i32* %%%zu, align 4\n", reg_index, num_proc_last_index);
  gen(g, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i64 0, i64 0), i32 %%%zu)\n", reg_index);
  gen(g, "\n");

  gen(g, "  ret i32 %%%zu\n", reg_index);
  gen(g, "}\n");
}

