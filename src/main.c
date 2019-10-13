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

  // デフォルトはstdin。
  // -i file でそのファイルディスクリプタを扱う。
  // これも最後まで特に開放しないです。
  FILE* infile = stdin;

  // デフォルトはstdout。
  // -o file でそのファイルディスクリプタを扱う。
  // これも最後まで特に開放しないです。
  FILE* outfile = stdout;

  int opt;
  while( (opt = getopt(argc, argv, "di:o:")) != -1 ) {
    switch (opt) {
      // デバッグモード。verboseな感じで標準エラー出力がうるさくなる。
      case 'd': isDebugMode = true; break;
      // 指定されたファイルから読み込む
      case 'i': {
        infile = fopen(optarg, "r");
        if(infile  == NULL) {
          fprintf(stderr, "Can't open input file.");
          exit(EXIT_FAILURE);
        }
      }
      break;
      // 指定されたファイルに書き出す
      case 'o': {
        outfile = fopen(optarg, "w");
        if(outfile == NULL) {
          fprintf(stderr, "Can't open output file.");
          exit(EXIT_FAILURE);
        }
      }
      break;
      default:
        fprintf(stderr, "Usage: %s [-d] [-i infile] [-o outfile]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  // 一定のバッファまで入力文字列を許す
  // それ以上は許さないことで話を単純化する
  char* input;
  input = (char*)malloc(sizeof(char) * INPUT_BUFFER_SIZE);
  fread(input, sizeof(char), INPUT_BUFFER_SIZE, infile);

  // 入力からTokenを作成
  Token* token = tokenize(input, INPUT_BUFFER_SIZE);
  if( isDebugMode ) print_tokens(token);

  // TokenをASTに変換
  AST* ast = parse(token);
  if( isDebugMode ) print_ast(ast, 0);

  // コード生成
  CodeGen* gen = create_codegen(outfile, isDebugMode);
  generate_code(gen, ast);

  return 0;
}
