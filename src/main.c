#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "main.h"
#include "tokenizer.h"
#include "parser.h"
#include "codegen.h"
#include "util.h"

#define INPUT_BUFFER_SIZE (10240)
#define OUTPUT_BUFFER_SIZE (10240)

int main(int argc, char **argv) {
  // 全体的にメモリ解放は頑張る必要がないのでやってないです(D言語方式)

  bool isDebugMode = false;
  int opt;
  while( (opt = getopt(argc, argv, "d")) != -1 ) {
    switch (opt) {
      case 'd': isDebugMode = true; break;
      default:
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  // 一定のバッファまで入力文字列を許す
  // それ以上は許さないことで話を単純化する
  char* input;
  input = (char*)malloc(sizeof(char) * INPUT_BUFFER_SIZE);
  fread(input, sizeof(char), INPUT_BUFFER_SIZE, stdin);

  // 入力からTokenを作成
  Token* token = tokenize(input, INPUT_BUFFER_SIZE);
  if( isDebugMode ) print_tokens(token);

  // TokenをASTに変換
  AST* ast = parse(token);
  if( isDebugMode ) print_ast(ast, 0);

  // 一定のバッファまでコンパイル結果を許す
  // これを超えるLLVM IRサイズは許さないことで話を単純化する
  char* output;
  output = (char*)malloc(sizeof(char) * OUTPUT_BUFFER_SIZE);

  // コード生成
  generate_code(output, OUTPUT_BUFFER_SIZE, ast);

  fprintf(stdout, "%s", output);
  return 0;
}
