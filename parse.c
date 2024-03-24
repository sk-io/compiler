#include "all.h"

AST_Node* parse_primary();
AST_Node* parse_infix(u32 min_precedence);
AST_Node* parse_expr();
AST_Node* parse_statement();
AST_Node* parse_block();

Parse_State parser = {0};

static u32 get_precedence(Token_Type token_type) {
	switch (token_type) {
		case TOKEN_IS_EQUAL:
		case TOKEN_NOT_EQUAL:
			return 1;
		case TOKEN_LESS_THAN:
		case TOKEN_GREATER_THAN:
		case TOKEN_LESS_THAN_EQUAL:
		case TOKEN_GREATER_THAN_EQUAL:
			return 2;
		case TOKEN_ADD:
		case TOKEN_SUB:
			return 3;
		case TOKEN_MUL:
		case TOKEN_DIV:
			return 4;
		default:
			printf("uhh thats not an operator\n");
			error();
	}
	
	return 0;
}

static bool is_binary_op(Token_Type token_type) {
	switch (token_type) {
		case TOKEN_ADD:
		case TOKEN_SUB:
		case TOKEN_MUL:
		case TOKEN_DIV:
		case TOKEN_IS_EQUAL:
		case TOKEN_NOT_EQUAL:
		case TOKEN_LESS_THAN:
		case TOKEN_GREATER_THAN:
		case TOKEN_LESS_THAN_EQUAL:
		case TOKEN_GREATER_THAN_EQUAL:
			return true;
		default:
			return false;
	}
	return false;
}

static Binary_Operation token_to_binary_op(Token_Type token_type) {
	switch (token_type) {
		case TOKEN_ADD:
			return OP_ADD;
		case TOKEN_SUB:
			return OP_SUB;
		case TOKEN_MUL:
			return OP_MUL;
		case TOKEN_DIV:
			return OP_DIV;
		case TOKEN_IS_EQUAL:
			return OP_EQUALS;
		case TOKEN_NOT_EQUAL:
			return OP_NOT_EQUALS;
		case TOKEN_LESS_THAN:
			return OP_LESS_THAN;
		case TOKEN_GREATER_THAN:
			return OP_GREATER_THAN;
		case TOKEN_LESS_THAN_EQUAL:
			return OP_LESS_THAN_EQUAL;
		case TOKEN_GREATER_THAN_EQUAL:
			return OP_GREATER_THAN_EQUAL;
		default:
			printf("error in token_to_binary_op\n");
			error();
	}

	return 0;
}

static Token eat(Token_Type expected_token_type) {
	if (parser.tokens[parser.pos].type != expected_token_type) {
		printf("error at %u:\n", parser.pos);
		printf("	expected %u, got %u!\n", expected_token_type, parser.tokens[parser.pos].type);
		error();
	}

	return parser.tokens[parser.pos++];
}

static Token peek(u32 offset) {
	if (parser.pos + offset >= parser.num_tokens) {
		Token token = {0};
		return token;
	}
	
	return parser.tokens[parser.pos + offset];
}

AST_Node* parse_func_call() {
	AST_Func_Call* call = malloc(sizeof(AST_Func_Call));
	call->type = AST_FUNC_CALL;
	call->name = eat(TOKEN_IDENT);
	call->num_args = 0;

	eat(TOKEN_OPEN_PAREN);

	if (peek(0).type != TOKEN_CLOSE_PAREN) {
		for (;;) {
			if (call->num_args >= MAX_ARGS) {
				printf("too many args in func call!\n");
				error();
			}
			
			call->args[call->num_args++] = parse_expr();

			if (peek(0).type == TOKEN_CLOSE_PAREN)
				break;
			
			eat(TOKEN_COMMA);
		}
	}

	eat(TOKEN_CLOSE_PAREN);
	return (AST_Node*) call;
}

AST_Node* parse_primary() {
	Token token = peek(0);

    if (token.type == TOKEN_OPEN_PAREN) {
        eat(TOKEN_OPEN_PAREN);
        AST_Node* expr = parse_expr();
        eat(TOKEN_CLOSE_PAREN);
        return expr;
    }

	if (token.type == TOKEN_IDENT && peek(1).type == TOKEN_OPEN_PAREN) {
		return parse_func_call();
	}

	if (token.type == TOKEN_IDENT) {
		AST_Var* var = malloc(sizeof(AST_Var));
		var->type = AST_VAR;
		var->name = eat(TOKEN_IDENT);
		return (AST_Node*) var;
	}

	if (token.type == TOKEN_STR_LIT) {
		AST_String* str = malloc(sizeof(AST_String));
		str->type = AST_STR_LITERAL;
		str->token = eat(TOKEN_STR_LIT);
		return (AST_Node*) str;
	}

	// assume integer literal
	eat(TOKEN_INT_LIT);

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%.*s", token.len, token.str);

	int num = atoi(buffer);

	AST_Number* node = malloc(sizeof(AST_Number));
	node->type = AST_INT_LITERAL;
	node->value = num;
	return (AST_Node*) node;
}

AST_Node* parse_infix(u32 min_precedence) {
	AST_Node* result = parse_primary();

	for (;;) {
		Token op = peek(0);
		if (!is_binary_op(op.type))
			break;

		u32 precedence = get_precedence(op.type);
		if (precedence < min_precedence)
			break;

		eat(op.type);

		// todo: right associativity?
		AST_Node* rhs = parse_infix(precedence + 1);

		AST_Binary_Op* node = malloc(sizeof(AST_Binary_Op));
		node->type = AST_BIN_OP;
		node->left = result;
		node->right = rhs;
		node->op = token_to_binary_op(op.type);
		result = (AST_Node*) node;
	}

	return result;
}

AST_Node* parse_expr() {
	return parse_infix(0);
}

AST_Node* parse_statement() {
	if (peek(0).type == TOKEN_OPEN_BRACE) {
		eat(TOKEN_OPEN_BRACE);
		AST_Node* block = parse_block();
		eat(TOKEN_CLOSE_BRACE);
		return block;
	}

	if (peek(0).type == TOKEN_KEYWORD_VAR) {
		eat(TOKEN_KEYWORD_VAR);

		AST_Var_Decl* decl = malloc(sizeof(AST_Var_Decl));
		decl->type = AST_VAR_DECL;
		decl->name = eat(TOKEN_IDENT);
		decl->assign = NULL;

		if (peek(0).type == TOKEN_ASSIGN) {
			eat(TOKEN_ASSIGN);

			decl->assign = parse_expr();
		}

		eat(TOKEN_SEMICOLON);
		return (AST_Node*) decl;
	}

	if (peek(0).type == TOKEN_KEYWORD_IF) {
		eat(TOKEN_KEYWORD_IF);

		AST_Conditional* if_stmt = malloc(sizeof(AST_Conditional));
		if_stmt->type = AST_IF;
		
		eat(TOKEN_OPEN_PAREN);
		if_stmt->condition = parse_expr();
		eat(TOKEN_CLOSE_PAREN);

		if_stmt->body = parse_statement();
		return (AST_Node*) if_stmt;
	}

	if (peek(0).type == TOKEN_KEYWORD_WHILE) {
		eat(TOKEN_KEYWORD_WHILE);

		AST_Conditional* while_stmt = malloc(sizeof(AST_Conditional));
		while_stmt->type = AST_WHILE;
		
		eat(TOKEN_OPEN_PAREN);
		while_stmt->condition = parse_expr();
		eat(TOKEN_CLOSE_PAREN);

		while_stmt->body = parse_statement();
		return (AST_Node*) while_stmt;
	}

	if (peek(0).type == TOKEN_KEYWORD_RETURN) {
		eat(TOKEN_KEYWORD_RETURN);

		AST_Return* ret = malloc(sizeof(AST_Return));
		ret->type = AST_RETURN;
		ret->expr = parse_expr();

		eat(TOKEN_SEMICOLON);
		return (AST_Node*) ret;
	}

	// todo: assignment as a binary operator instead?
	if (peek(1).type == TOKEN_ASSIGN) {
		AST_Assign* assign = malloc(sizeof(AST_Assign));
		assign->type = AST_ASSIGN;
		assign->lhs = eat(TOKEN_IDENT);
		eat(TOKEN_ASSIGN);
		assign->rhs = parse_expr();
		eat(TOKEN_SEMICOLON);
		return (AST_Node*) assign;
	}

	AST_Node* expr = parse_expr();
	eat(TOKEN_SEMICOLON);
	return expr;
}

AST_Node* parse_block() {
	AST_Block* block = malloc(sizeof(AST_Block));
	block->type = AST_BLOCK;
	block->num_statements = 0;
	block->statements_capacity = 32;
	block->statements = malloc(block->statements_capacity * sizeof(AST_Node*));

	while (peek(0).type != TOKEN_EOF && peek(0).type != TOKEN_CLOSE_BRACE) {
		AST_Node* statement = parse_statement();
		if (block->num_statements >= block->statements_capacity) {
			block->statements_capacity *= 2;
			block->statements = realloc(block->statements, block->statements_capacity * sizeof(AST_Node*));
		}

		block->statements[block->num_statements++] = statement;
	}

	return (AST_Node*) block;
}

AST_Node* parse_func_decl() {
	eat(TOKEN_KEYWORD_FUNC);

	AST_Func_Decl* decl = malloc(sizeof(AST_Func_Decl));
	decl->type = AST_FUNC_DECL;
	decl->name = eat(TOKEN_IDENT);
	decl->num_args = 0;

	eat(TOKEN_OPEN_PAREN);

	if (peek(0).type != TOKEN_CLOSE_PAREN) {
		for (;;) {
			if (decl->num_args >= MAX_ARGS) {
				printf("too many args in func decl!\n");
				error();
			}
			
			eat(TOKEN_KEYWORD_VAR);
			decl->args[decl->num_args++] = eat(TOKEN_IDENT);

			if (peek(0).type == TOKEN_CLOSE_PAREN)
				break;
			
			eat(TOKEN_COMMA);
		}
	}

	eat(TOKEN_CLOSE_PAREN);

	eat(TOKEN_OPEN_BRACE);
	decl->body = parse_block();
	eat(TOKEN_CLOSE_BRACE);

	return (AST_Node*) decl;
}

AST_Node* parse(char* input_text, Token* tokens, u32 num_tokens) {
	memset(&parser, 0, sizeof(Parse_State));
	parser.program = input_text;
	parser.tokens = tokens;
	parser.num_tokens = num_tokens;

	AST_Program* program = malloc(sizeof(AST_Program));
	program->type = AST_PROGRAM;
	program->num_defs = 0;
	program->defs_capacity = 32;
	program->defs = malloc(program->defs_capacity * sizeof(AST_Node*));

	while (peek(0).type != TOKEN_EOF) {
		AST_Node* def = parse_func_decl();
		if (program->num_defs >= program->defs_capacity) {
			program->defs_capacity *= 2;
			program->defs = realloc(program->defs, program->defs_capacity * sizeof(AST_Node*));
		}

		program->defs[program->num_defs++] = def;
	}

	return (AST_Node*) program;
}
