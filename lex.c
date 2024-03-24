#include "all.h"

static void add_token(Token* tokens, u32* num_tokens, Token* token_to_add) {
	tokens[(*num_tokens)++] = *token_to_add;
	if (*num_tokens >= MAX_TOKENS) {
		printf("too many tokens!\n");
		error();
	}
}

void lex(const char* input, u32 input_length, Token* tokens, u32* num_tokens) {
	u32 pos = 0;
	u32 token_start = 0;
	u32 token_type = 0;
	*num_tokens = 0;

	while (pos < input_length) {
		char ch = input[pos];
		
		// ignore certain chars
		if (isspace(ch)) {
			pos++;
			token_start = pos;
			continue;
		}

		// ignore comments
		if (ch == '/' && input[pos + 1] == '/') {
			while (input[pos] != '\n') {
				pos++;
			}
			token_start = pos;
			continue;
		}

		// determine token type based on first char
		if (isalpha(ch)) {
			token_type = TOKEN_IDENT;
			while (isalnum(input[pos + 1])) {
				pos++;
			}
		} else if (isdigit(ch)) {
			token_type = TOKEN_INT_LIT;
			while (isdigit(input[pos + 1])) {
				pos++;
			}
		} else if (ch == '"') {
			token_type = TOKEN_STR_LIT;
			pos++;
			while (input[pos] != '"') {
				pos++;
			}
		} else if (ch == '+') {
			token_type = TOKEN_ADD;
		} else if (ch == '-') {
			token_type = TOKEN_SUB;
		} else if (ch == '*') {
			token_type = TOKEN_MUL;
		} else if (ch == '/') {
			token_type = TOKEN_DIV;
		} else if (ch == '(') {
			token_type = TOKEN_OPEN_PAREN;
		} else if (ch == ')') {
			token_type = TOKEN_CLOSE_PAREN;
		} else if (ch == ';') {
			token_type = TOKEN_SEMICOLON;
		} else if (ch == '=') {
			if (input[pos + 1] == '=') {
				token_type = TOKEN_IS_EQUAL;
				pos++;
			} else {
				token_type = TOKEN_ASSIGN;
			}
		} else if (ch == '{') {
			token_type = TOKEN_OPEN_BRACE;
		} else if (ch == '}') {
			token_type = TOKEN_CLOSE_BRACE;
		} else if (ch == ',') {
			token_type = TOKEN_COMMA;
		} else if (ch == '<') {
			token_type = TOKEN_LESS_THAN;
			if (input[pos + 1] == '=') {
				token_type = TOKEN_LESS_THAN_EQUAL;
				pos++;
			}
		} else if (ch == '>') {
			token_type = TOKEN_GREATER_THAN;
			if (input[pos + 1] == '=') {
				token_type = TOKEN_GREATER_THAN_EQUAL;
				pos++;
			}
		} else if (ch == '!') {
			if (input[pos + 1] == '=') {
				token_type = TOKEN_NOT_EQUAL;
				pos++;
			}
		} else {
			printf("unknown token type at %u: %u\n", pos, (u32)ch);
			error();
		}

		pos++;

		Token token = {
			.type = token_type,
			.str = input + token_start,
			.len = pos - token_start,
		};
		
		if (token_type == TOKEN_IDENT) {
			if (compare_token(&token, "int")) { // fixme: more types
				token.type = TOKEN_KEYWORD_VAR;
			} else if (compare_token(&token, "func")) {
				token.type = TOKEN_KEYWORD_FUNC;
			} else if (compare_token(&token, "if")) {
				token.type = TOKEN_KEYWORD_IF;
			} else if (compare_token(&token, "return")) {
				token.type = TOKEN_KEYWORD_RETURN;
			} else if (compare_token(&token, "while")) {
				token.type = TOKEN_KEYWORD_WHILE;
			}
		}

		add_token(tokens, num_tokens, &token);

		token_start = pos;
	}

	// add safety eof
	// fixme: is this necessary?
	Token token = {
		.type = TOKEN_EOF,
	};
	add_token(tokens, num_tokens, &token);
}
