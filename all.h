#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef enum {
	TOKEN_NONE,
	TOKEN_IDENT,
	TOKEN_INT_LIT,
	TOKEN_STR_LIT,
	TOKEN_ADD,
	TOKEN_SUB,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_IS_EQUAL,
	TOKEN_NOT_EQUAL,
	TOKEN_GREATER_THAN,
	TOKEN_LESS_THAN,
	TOKEN_GREATER_THAN_EQUALS,
	TOKEN_LESS_THAN_EQUALS,
	TOKEN_OPEN_PAREN,
	TOKEN_CLOSE_PAREN,
	TOKEN_OPEN_BRACE,
	TOKEN_CLOSE_BRACE,
	TOKEN_SEMICOLON,
	TOKEN_KEYWORD_VAR,
	TOKEN_KEYWORD_FUNC,
	TOKEN_KEYWORD_IF,
	TOKEN_KEYWORD_RETURN,
	TOKEN_KEYWORD_WHILE,
	TOKEN_ASSIGN,
	TOKEN_COMMA,
	TOKEN_EOF,
} Token_Type;

typedef struct {
	Token_Type type;
	const char* str; // not null-terminated!
	u32 len;
} Token;

#define MAX_TOKENS 1024

typedef struct {
	char* program;

	// todo: dynalloc
	Token* tokens;
	u32 num_tokens;

	u32 pos;
} Parse_State;

typedef enum {
	AST_PROGRAM,
	AST_INT_LITERAL,
	AST_STR_LITERAL,
	AST_BIN_OP,
	AST_BLOCK,
	AST_VAR_DECL,
	AST_VAR,
	AST_ASSIGN,
	AST_FUNC_DECL,
	AST_FUNC_CALL,
	AST_IF,
	AST_WHILE,
	AST_RETURN,
} AST_Type;

typedef struct {
	AST_Type type;
} AST_Node;

typedef struct {
	AST_Type type;
	AST_Node** defs;
	u32 defs_capacity;
	u32 num_defs;
} AST_Program;

typedef struct {
	AST_Type type;
	u32 value;
} AST_Number;

typedef struct {
	AST_Type type;
	Token token;
} AST_String;

typedef enum {
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_EQUALS,
	OP_NOT_EQUALS,
	OP_GREATER_THAN,
	OP_LESS_THAN,
} Binary_Operation;

typedef struct {
	AST_Type type;
	Binary_Operation op;
	AST_Node* left;
	AST_Node* right;
} AST_Binary_Op;

typedef struct {
	AST_Type type;
	AST_Node** statements;
	u32 statements_capacity;
	u32 num_statements;
} AST_Block;

typedef struct {
	AST_Type type;
	Token name;
	AST_Node* assign;
} AST_Var_Decl;

typedef struct {
	AST_Type type;
	Token name;
} AST_Var;

typedef struct {
	AST_Type type;
	Token lhs;
	AST_Node* rhs;
} AST_Assign;

#define MAX_ARGS 16

typedef struct {
	AST_Type type;
	Token name;
	Token args[MAX_ARGS];
	u32 num_args;
	AST_Node* body;
} AST_Func_Decl;

typedef struct {
	AST_Type type;
	Token name;
	AST_Node* args[MAX_ARGS];
	u32 num_args;
} AST_Func_Call;

typedef struct {
	AST_Type type;
	AST_Node* condition;
	AST_Node* body;
} AST_Conditional;

typedef struct {
	AST_Type type;
	AST_Node* expr;
} AST_Return;

typedef u32 stack_loc;

typedef struct {
	Token token;
	stack_loc location;
} Variable;

#define MAX_VARS 64
#define MAX_STRING_LITERALS 64

typedef struct {
	Variable vars[MAX_VARS];
	u32 num_vars;
	stack_loc alloc;
} Local_Context;

typedef struct {
	FILE* file;
	Local_Context context;
	u32 label;

	Token string_literals[MAX_STRING_LITERALS];
	u32 num_string_literals;
} Emit_State;

void error();
void lex(const char* input, u32 input_length, Token* tokens, u32* num_tokens);
AST_Node* parse(char* program, Token* tokens, u32 num_tokens);
void emit(AST_Node* root, const char* path);
void print_node(AST_Node* node, int depth);
bool compare_token(Token* token, const char* str);
