#include "all.h"

void print_node(AST_Node* node, int depth) {
	for (int i = 0; i < depth * 2; i++) {
		putchar(' ');
	}

	switch (node->type) {
		case AST_PROGRAM: {
			AST_Program* program = (AST_Program*) node;
			printf("AST_PROGRAM\n");
			for (u32 i = 0; i < program->num_defs; i++) {
				print_node(program->defs[i], depth + 1);
			}
			break;
		}
		case AST_INT_LITERAL:
			printf("AST_INT_LITERAL: %d\n", ((AST_Number*)node)->value);
			break;
		case AST_STR_LITERAL: {
			AST_String* str = (AST_String*) node;
			printf("AST_STR_LITERAL: %.*s\n", str->token.len, str->token.str);
			break;
		}
		case AST_BIN_OP: {
			AST_Binary_Op* op = (AST_Binary_Op*) node;
			printf("AST_BIN_OP: %u\n", op->op);
			print_node(op->left, depth + 1);
			print_node(op->right, depth + 1);
			break;
		}
		case AST_BLOCK: {
			AST_Block* block = (AST_Block*) node;
			printf("AST_BLOCK\n");
			for (u32 i = 0; i < block->num_statements; i++) {
				print_node(block->statements[i], depth + 1);
			}
			break;
		}
		case AST_VAR_DECL: {
			AST_Var_Decl* decl = (AST_Var_Decl*) node;
			printf("AST_VAR_DECL: '%.*s'\n", decl->name.len, decl->name.str);
			if (decl->assign != NULL) {
				print_node(decl->assign, depth + 1);
			}
			break;
		}
		case AST_VAR: {
			AST_Var* var = (AST_Var*) node;
			printf("AST_VAR: '%.*s'\n", var->name.len, var->name.str);
			break;
		}
		case AST_ASSIGN: {
			AST_Assign* assign = (AST_Assign*) node;
			printf("AST_ASSIGN: '%.*s'\n", assign->lhs.len, assign->lhs.str);
			print_node(assign->rhs, depth + 1);
			break;
		}
		case AST_FUNC_DECL: {
			AST_Func_Decl* decl = (AST_Func_Decl*) node;
			printf("AST_FUNC_DECL: '%.*s'\n", decl->name.len, decl->name.str);
			print_node(decl->body, depth + 1);
			break;
		}
		case AST_IF: {
			AST_Conditional* if_stmt = (AST_Conditional*) node;
			printf("AST_IF\n");
			print_node(if_stmt->condition, depth + 1);
			print_node(if_stmt->body, depth + 1);
			break;
		}
        case AST_WHILE: {
			AST_Conditional* while_stmt = (AST_Conditional*) node;
			printf("AST_WHILE\n");
			print_node(while_stmt->condition, depth + 1);
			print_node(while_stmt->body, depth + 1);
			break;
		}
		case AST_FUNC_CALL: {
			AST_Func_Call* call = (AST_Func_Call*) node;
			printf("AST_FUNC_CALL\n");
			for (u32 i = 0; i < call->num_args; i++) {
				print_node(call->args[i], depth + 1);
			}
			break;
		}
		case AST_RETURN: {
			AST_Return* ret = (AST_Return*) node;
			printf("AST_RETURN\n");
			print_node(ret->expr, depth + 1);
			break;
		}
		default:
			printf("print_node error: unhandled type %u\n", node->type);
			error();
	}
}

// todo: free_node?

bool compare_token(Token* token, const char* str) {
	for (u32 i = 0; i < token->len; i++) {
		if (str[i] == '\0')
			return false;
		if (token->str[i] != str[i])
			return false;
	}

	if (str[token->len] != '\0')
		return false;

	return true;
}
