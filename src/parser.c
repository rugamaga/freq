#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "util.h"

#define MAX_CODE_SIZE 10240

static Parser* create_parser(Token* root) {
  Parser* parser = (Parser*)malloc(sizeof(Parser));
  parser->code = (AST**)malloc(sizeof(AST*) * MAX_CODE_SIZE);
  parser->current = parser->root = root;
  return parser;
}

static Token* consume(Parser* parser, TokenType type) {
  if( !(parser->current) ) return NULL;
  if( parser->current->type != type ) return NULL;
  Token* consumed = parser->current;
  parser->current = parser->current->next;
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

static AST* parse_lvar(Parser* parser) {
  Token* tok;
  if( (tok = consume( parser, TT_IDENT )) )
    return create_ast(ST_VAR, tok, NULL, NULL );
  return NULL;
}

static AST* parse_expr(Parser* parser);
static AST* parse_factor(Parser* parser) {
  Token* tok;
  AST* node;
  if( consume( parser, TT_LEFT_PAREN ) ) {
    AST* node = parse_expr( parser );
    if( consume( parser, TT_RIGHT_PAREN ) ) {
      return node;
    }
    return NULL;
  }

  if( (tok = consume( parser, TT_NUM )) )
    return create_ast( ST_NUM, tok, NULL, NULL );
  if( (tok = consume( parser, TT_IDENT )) )
    return create_ast( ST_VAR, tok, NULL, NULL );
  return NULL;
}

static AST* parse_unary(Parser* parser) {
  Token* tok;
  if( (tok = consume( parser, TT_PLUS )) )
    return parse_unary( parser );

  if( (tok = consume( parser, TT_MINUS )) ) {
    // ここはシンタックスシュガーとして生成されるので
    // 後で解釈されるときのためにダミーのトークンを登録しておく
    Token* dummy = create_token(TT_NUM, "0", 0, 1);
    return create_ast( ST_SUB, tok, create_ast( ST_NUM, dummy, NULL, NULL ), parse_unary( parser ) );
  }

  return parse_factor( parser );
}

static AST* parse_term(Parser* parser) {
  AST* node = parse_unary(parser);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( parser, TT_MUL )) )
      node = create_ast( ST_MUL, tok, node, parse_unary(parser) );
    else if( (tok = consume( parser, TT_DIV )) )
      node = create_ast( ST_DIV, tok, node, parse_unary(parser) );
    else
      return node;
  }
}

static AST* parse_expr(Parser* parser) {
  AST* node = parse_term(parser);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( parser, TT_PLUS )) )
      node = create_ast( ST_ADD, tok, node, parse_term(parser) );
    else if( (tok = consume( parser, TT_MINUS )) )
      node = create_ast( ST_SUB, tok, node, parse_term(parser) );
    else
      return node;
  }
}


static AST* parse_rational(Parser* parser) {
  AST* node = parse_expr(parser);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( parser, TT_LT )) )
      node = create_ast( ST_LT, tok, node, parse_expr(parser) );
    else if( (tok = consume( parser, TT_LTEQ )) )
      node = create_ast( ST_LTEQ, tok, node, parse_expr(parser) );
    else if( (tok = consume( parser, TT_GT )) )
      node = create_ast( ST_GT, tok, node, parse_expr(parser) );
    else if( (tok = consume( parser, TT_GTEQ )) )
      node = create_ast( ST_GTEQ, tok, node, parse_expr(parser) );
    else
      return node;
  }
}

static AST* parse_equality(Parser* parser) {
  AST* node = parse_rational(parser);
  for( ; ; ) {
    Token* tok;
    if( (tok = consume( parser, TT_EQUAL )) )
      node = create_ast( ST_EQUAL, tok, node, parse_rational(parser) );
    else if( (tok = consume( parser, TT_NOT_EQUAL )) )
      node = create_ast( ST_NOT_EQUAL, tok, node, parse_rational(parser) );
    else
      return node;
  }
}

static AST* parse_assign(Parser* parser) {
  AST* node = parse_equality(parser);
  Token* tok;
  if( (tok = consume(parser, TT_ASSIGN)) ) {
    return create_ast( ST_ASSIGN, tok, node, parse_assign(parser) );
  } else {
    return node;
  }
}

static AST* parse_let(Parser* parser) {
  Token* tok;
  if( (tok = consume(parser, TT_LET)) ) {
    AST* lhs = parse_lvar(parser);
    AST* rhs = NULL;
    Token* assign;
    if( (assign = consume(parser, TT_ASSIGN)) )
      rhs = parse_assign(parser);
    return create_ast( ST_LET, tok, lhs, rhs );
  } else {
    return parse_assign(parser);
  }
}

static AST* parse_stmt(Parser* parser) {
  Token* tok;
  if( (tok = consume(parser, TT_LET)) ) {
    AST* lhs = parse_lvar(parser);
    AST* rhs = NULL;
    Token* assign;
    if( (assign = consume(parser, TT_ASSIGN)) )
      rhs = parse_assign(parser);
    return create_ast( ST_LET, tok, lhs, rhs );
  } else if( (tok = consume(parser, TT_RET) ) ){
    AST* node = parse_assign(parser);
    return create_ast( ST_RET, tok, node, NULL );
  } else {
    return parse_assign(parser);
  }
}

Parser* parse(Token* token) {
  Parser* parser = create_parser(token);

  // ROOTから始まる
  if( !consume( parser, TT_ROOT ) )
    return NULL;

  // stmtをすべて読み込む
  size_t i = 0;
  while( !consume(parser, TT_EOF) ) {
    parser->code[i++] = parse_stmt(parser);
    consume(parser, TT_SEMICOLON);
  }

  // NULLで終端しておくことで後続で処理できるようにする
  parser->code[i] = NULL;

  return parser;
}

void print_ast(AST* ast, size_t level) {
  if( ast == NULL ) return;
  indent(level); fprintf(stderr, "SyntaxType: %u (%ld)\n", ast->type, ast->val);
  print_ast(ast->lhs, level + 1);
  print_ast(ast->rhs, level + 1);
}

