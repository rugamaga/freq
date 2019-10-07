#include <stdio.h>

#include "codegen.h"
#include "parser.h"

static char* generate_numeric_process(char* output, size_t* index, AST* ast) {
  if (ast->type == ST_NUM) {
    *index += 1;
    output += sprintf(output, "  ; Assign ST_NUM\n");
    output += sprintf(output, "  %%%zu = alloca i32, align 4\n", *index);
    output += sprintf(output, "  store i32 %ld, i32* %%%zu\n", ast->val, *index);

    output += sprintf(output, "\n");
    return output;
  }

  output += sprintf(output, "  ; ------------- Calculate LHS\n");
  output  = generate_numeric_process(output, index, ast->lhs);
  const size_t lhs_index = *index;

  output += sprintf(output, "  ; ------------- Calculate RHS\n");
  output  = generate_numeric_process(output, index, ast->rhs);
  const size_t rhs_index = *index;

  switch( ast->type ) {
  case ST_NUM:
    // 処理済み
    break;
  case ST_ADD:
    {
      output += sprintf(output, "  ; ------------- Calculate ST_ADD\n");

      *index += 1;
      output += sprintf(output, "  %%%zu = load i32, i32* %%%zu, align 4\n", *index, lhs_index);
      const size_t lhs_reg_index = *index;

      *index += 1;
      output += sprintf(output, "  %%%zu = load i32, i32* %%%zu, align 4\n", *index, rhs_index);
      const size_t rhs_reg_index = *index;

      *index += 1;
      output += sprintf(output, "  %%%zu = add i32 %%%zu, %%%zu\n", *index, lhs_reg_index, rhs_reg_index);
      const size_t add_reg_index = *index;

      *index += 1;
      output += sprintf(output, "  %%%zu = alloca i32, align 4\n", *index);
      const size_t res_mem_index = *index;

      output += sprintf(output, "  store i32 %%%zu, i32* %%%zu, align 4\n", add_reg_index, res_mem_index);
    }
    break;
  case ST_SUB:
    {
      output += sprintf(output, "  ; ------------- Calculate ST_ADD\n");

      *index += 1;
      output += sprintf(output, "  %%%zu = load i32, i32* %%%zu, align 4\n", *index, lhs_index);
      const size_t lhs_reg_index = *index;

      *index += 1;
      output += sprintf(output, "  %%%zu = load i32, i32* %%%zu, align 4\n", *index, rhs_index);
      const size_t rhs_reg_index = *index;

      *index += 1;
      output += sprintf(output, "  %%%zu = sub i32 %%%zu, %%%zu\n", *index, lhs_reg_index, rhs_reg_index);
      const size_t sub_reg_index = *index;

      *index += 1;
      output += sprintf(output, "  %%%zu = alloca i32, align 4\n", *index);
      const size_t res_mem_index = *index;

      output += sprintf(output, "  store i32 %%%zu, i32* %%%zu, align 4\n", sub_reg_index, res_mem_index);
    }
    break;
  }

  output += sprintf(output, "\n");
  return output;
}

void generate_code(char* output, size_t len, AST* ast) {
  size_t index = 0;

  output += sprintf(output, "%%FILE = type opaque\n");
  output += sprintf(output, "@__stdinp = external global %%FILE*, align 8\n");
  output += sprintf(output, "@__stdoutp = external global %%FILE*, align 8\n");
  output += sprintf(output, "@__stderrp = external global %%FILE*, align 8\n");
  output += sprintf(output, "\n");

  output += sprintf(output, "@str = private unnamed_addr constant [4 x i8] c\"%%d\\0A\\00\", align 1\n");
  output += sprintf(output, "\n");

  output += sprintf(output, "declare i32 @fprintf(%%FILE*, i8*, ...)\n");
  output += sprintf(output, "declare i32 @printf(i8*, ...)\n");
  output += sprintf(output, "declare i32 @atoi(...)\n");
  output += sprintf(output, "\n");

  output += sprintf(output, "define i32 @main() nounwind {\n");

  output  = generate_numeric_process(output, &index, ast);

  const size_t num_proc_last_index = index;
  index += 1;
  const size_t reg_index = index;

  output += sprintf(output, "  ; ------------- Output result\n");
  output += sprintf(output, "  %%%zu = load i32, i32* %%%zu, align 4\n", reg_index, num_proc_last_index);
  output += sprintf(output, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i64 0, i64 0), i32 %%%zu)\n", reg_index);
  output += sprintf(output, "\n");

  output += sprintf(output, "  ret i32 %%%zu\n", reg_index);
  output += sprintf(output, "}\n");
}

