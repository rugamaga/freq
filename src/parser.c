#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "util.h"

static Token* consume(Token** current, TokenType type) {
  if( !(*current) ) return NULL;
  if( (*current)->type != type ) return NULL;
  Token* consumed = *current;
  *current = (*current)->next;
  return consumed;
}

static AST* create_ast(SyntaxType type, Token* token, AST* lhs, AST* rhs) {
  AST* node = (AST*)malloc(sizeof(AST));
  node->type = type;
  node->token = token;
  // 数値ノードならtokenをstrtolで解釈する
  if( type == ST_NUM ) {
    const char* sp = token->buffer + token->pos;
    char* ep;
    node->val = strtol(sp, &ep, 10);
  }
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static AST* parse_expr(Token** current);
static AST* parse_factor(Token** current) {
  Token* tok;
  AST* node;
  if( consume( current, TT_LEFT_BRACKET ) ) {
    AST* node = parse_expr( current );
    if( consume( current, TT_RIGHT_BRACKET ) ) {
      return node;
    }
    return NULL;
  }

  if( (tok = consume( current, TT_NUM )) )
    return create_ast( ST_NUM, tok, NULL, NULL );
  return NULL;
}

static AST* parse_unary(Token** current) {
  Token* tok;
  if( (tok = consume( current, TT_PLUS )) )
    return parse_unary( current );

  if( (tok = consume( current, TT_MINUS )) ) {
    // ここはシンタックスシュガーとして生成されるので
    // 後で解釈されるときのためにダミーのトークンを登録しておく
    Token* dummy = create_token(TT_NUM, "0", 0, 1);
    return create_ast( ST_SUB, tok, create_ast( ST_NUM, dummy, NULL, NULL ), parse_unary( current ) );
  }

  return parse_factor( current );
}

static AST* parse_term(Token** current) {
  AST* node = parse_unary(current);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( current, TT_MUL )) )
      node = create_ast( ST_MUL, tok, node, parse_unary(current) );
    else if( (tok = consume( current, TT_DIV )) )
      node = create_ast( ST_DIV, tok, node, parse_unary(current) );
    else
      return node;
  }
}

static AST* parse_expr(Token** current) {
  AST* node = parse_term(current);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( current, TT_PLUS )) )
      node = create_ast( ST_ADD, tok, node, parse_term(current) );
    else if( (tok = consume( current, TT_MINUS )) )
      node = create_ast( ST_SUB, tok, node, parse_term(current) );
    else
      return node;
  }
}

AST* parse(Token* token) {
  // pointerへのpointerを取ることで
  // この後の内部処理を行えるようにする
  Token** current = &token;
  if( !consume( current, TT_ROOT ) )
    return NULL;
  return parse_expr(current);
}

void print_ast(AST* ast, size_t level) {
  if( ast == NULL ) return;
  indent(level); fprintf(stderr, "SyntaxType: %u (%ld)\n", ast->type, ast->val);
  print_ast(ast->lhs, level + 1);
  print_ast(ast->rhs, level + 1);
}

