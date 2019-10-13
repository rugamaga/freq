#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "codegen.h"
#include "parser.h"

CodeGen* create_codegen(FILE* fp) {
  CodeGen* gen = (CodeGen*)malloc(sizeof(CodeGen));
  gen->output = fp;
  gen->index = 0;
  return gen;
}

static void code(CodeGen* gen, const char* format, ...) {
  va_list va;
  va_start(va, format);
  vfprintf(gen->output, format, va);
  va_end(va);
}

static size_t generate_alloca(CodeGen* gen) {
  const size_t mem = ++(gen->index);
  code(gen, "  %%%zu = alloca i32, align 4\n", mem);
  return mem;
}

static void generate_store(CodeGen* gen, size_t reg, size_t mem) {
  code(gen, "  store i32 %%%zu, i32* %%%zu, align 4\n", reg, mem);
}

static size_t generate_load(CodeGen* gen, size_t src) {
  const size_t dst = ++(gen->index);
  code(gen, "  %%%zu = load i32, i32* %%%zu, align 4\n", dst, src);
  return dst;
}

static size_t generate_add(CodeGen* gen, size_t lhs, size_t rhs) {
  const size_t reg = ++(gen->index);
  code(gen, "  %%%zu = add i32 %%%zu, %%%zu\n", reg, lhs, rhs);
  return reg;
}

static size_t generate_sub(CodeGen* gen, size_t lhs, size_t rhs) {
  const size_t reg = ++(gen->index);
  code(gen, "  %%%zu = sub i32 %%%zu, %%%zu\n", reg, lhs, rhs);
  return reg;
}

static size_t generate_numeric_process(CodeGen* gen, AST* ast) {
  if (ast->type == ST_NUM) {
    const size_t alloc_index = ++(gen->index);

    code(gen, "  ; Assign ST_NUM\n");
    code(gen, "  %%%zu = alloca i32, align 4\n", alloc_index);
    code(gen, "  store i32 %ld, i32* %%%zu\n", ast->val, alloc_index);

    code(gen, "\n");
    return gen->index;
  }

  code(gen, "  ; ------------- Calculate LHS\n");
  const size_t lhs_index = generate_numeric_process(gen, ast->lhs);

  code(gen, "  ; ------------- Calculate RHS\n");
  const size_t rhs_index = generate_numeric_process(gen, ast->rhs);

  switch( ast->type ) {
  case ST_NUM:
    // 処理済み
    break;
  case ST_ADD:
    {
      code(gen, "  ; ------------- Calculate ST_ADD\n");
      const size_t lhs_reg_index = generate_load(gen, lhs_index);
      const size_t rhs_reg_index = generate_load(gen, rhs_index);
      const size_t add_reg_index = generate_add(gen, lhs_reg_index, rhs_reg_index);
      const size_t res_mem_index = generate_alloca(gen);
      generate_store(gen, add_reg_index, res_mem_index);
    }
    break;
  case ST_SUB:
    {
      code(gen, "  ; ------------- Calculate ST_ADD\n");
      const size_t lhs_reg_index = generate_load(gen, lhs_index);
      const size_t rhs_reg_index = generate_load(gen, rhs_index);
      const size_t sub_reg_index = generate_sub(gen, lhs_reg_index, rhs_reg_index);
      const size_t res_mem_index = generate_alloca(gen);
      generate_store(gen, sub_reg_index, res_mem_index);
    }
    break;
  }

  code(gen, "\n");
  return gen->index;
}

void generate_code(CodeGen* gen, AST* ast) {
  code(gen, "%%FILE = type opaque\n");
  code(gen, "@__stdinp = external global %%FILE*, align 8\n");
  code(gen, "@__stdoutp = external global %%FILE*, align 8\n");
  code(gen, "@__stderrp = external global %%FILE*, align 8\n");
  code(gen, "\n");

  code(gen, "@str = private unnamed_addr constant [4 x i8] c\"%%d\\0A\\00\", align 1\n");
  code(gen, "\n");

  code(gen, "declare i32 @fprintf(%%FILE*, i8*, ...)\n");
  code(gen, "declare i32 @printf(i8*, ...)\n");
  code(gen, "declare i32 @atoi(...)\n");
  code(gen, "\n");

  code(gen, "define i32 @main() nounwind {\n");

  const size_t num_proc_last_index = generate_numeric_process(gen, ast);
  const size_t reg_index = ++(gen->index);

  code(gen, "  ; ------------- Output result\n");
  code(gen, "  %%%zu = load i32, i32* %%%zu, align 4\n", reg_index, num_proc_last_index);
  code(gen, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i64 0, i64 0), i32 %%%zu)\n", reg_index);
  code(gen, "\n");

  code(gen, "  ret i32 %%%zu\n", reg_index);
  code(gen, "}\n");
}

