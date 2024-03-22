#include "all.h"

stack_loc emit_node(AST_Node* node);

static Emit_State emitter = {0};

static const char* sysv_call_regs[MAX_ARGS] = {
	"rdi", "rsi", "rdx", "rcx", "r8", "r9"
};

static u32 get_required_stack_size(AST_Node* node) {
	switch (node->type) {
		case AST_VAR:
			return 0;
		case AST_INT_LITERAL:
		case AST_STR_LITERAL:
			return 1;
		case AST_FUNC_CALL: {
			AST_Func_Call* call = (AST_Func_Call*) node;
			u32 sum = 1; // 1 for return value
			for (u32 i = 0; i < call->num_args; i++) {
				sum += get_required_stack_size(call->args[i]);
			}
			return sum;
		}
		case AST_BIN_OP: {
			AST_Binary_Op* op = (AST_Binary_Op*) node;
			stack_loc left = get_required_stack_size(op->left);
			stack_loc right = get_required_stack_size(op->right);
			return 1 + left + right;
		}
        case AST_BLOCK: {
			AST_Block* block = (AST_Block*) node;
            u32 sum = 0;
            for (u32 i = 0; i < block->num_statements; i++) {
			    sum += get_required_stack_size(block->statements[i]);
			}
			return sum;
		}
		case AST_FUNC_DECL: {
			AST_Func_Decl* decl = (AST_Func_Decl*) node;
			return decl->num_args + get_required_stack_size(decl->body);
		}
		case AST_RETURN: {
			AST_Return* ret = (AST_Return*) node;
			return get_required_stack_size(ret->expr);
		}
		case AST_WHILE:
		case AST_IF: {
			AST_Conditional* cond = (AST_Conditional*) node;
			return get_required_stack_size(cond->condition) + get_required_stack_size(cond->body);
		}
		case AST_ASSIGN: {
			AST_Assign* assign = (AST_Assign*) node;
			return get_required_stack_size(assign->rhs);
		}
		case AST_VAR_DECL: {
			AST_Var_Decl* var = (AST_Var_Decl*) node;
			u32 sum = 1;
			if (var->assign != NULL) {
				sum += get_required_stack_size(var->assign);
			}
			return sum;
		}
        default:
            printf("get_required_stack_size: unhandled node type %u\n", node->type);
	        error();
	}

	return 0;
}

// fixme: return * 8?
stack_loc allocate_stack() {
	return emitter.context.alloc++;
}

stack_loc emit_number(AST_Number* number) {
	stack_loc location = allocate_stack();
	fprintf(emitter.file, "	; integer literal\n");
	fprintf(emitter.file, "	mov qword [rbp - %u], %u\n", location * 8, number->value);
	return location;
}

stack_loc emit_string(AST_String* str) {
	u32 string_no = emitter.num_string_literals++;
	emitter.string_literals[string_no] = str->token;

	stack_loc location = allocate_stack();
	fprintf(emitter.file, "	; string literal\n");
	fprintf(emitter.file, "	mov qword [rbp - %u], _str%u\n", location * 8, string_no);
	return location;
}

Variable* find_var_by_name(Token* name) {
	Variable* found = NULL;

	for (u32 i = 0; i < emitter.context.num_vars; i++) {
		const Token* token = &emitter.context.vars[i].token;

		if (token->len != name->len)
			continue;

		if (strncmp(token->str, name->str, token->len) == 0) {
			found = &emitter.context.vars[i];
		}
	}

	return found;
}

stack_loc emit_var(AST_Var* var) {
	Variable* found = find_var_by_name(&var->name);

	if (found == NULL) {
		printf("emit_var: variable not found!\n");
		error();
	}

	fprintf(emitter.file, "	; var reference\n");

	return found->location;
}

stack_loc emit_binary_op(AST_Binary_Op* op, stack_loc left, stack_loc right) {
	stack_loc location = allocate_stack();
	fprintf(emitter.file, "	; binary op\n");

	// todo: clean this up

	if (op->op == OP_EQUALS || op->op == OP_NOT_EQUALS) {
		fprintf(emitter.file, "	xor rax, rax\n");
		fprintf(emitter.file, "	mov rcx, qword [rbp - %u]\n", left * 8);
		fprintf(emitter.file, "	cmp rcx, qword [rbp - %u]\n", right * 8);
		if (op->op == OP_EQUALS)
			fprintf(emitter.file, "	sete al\n");
		if (op->op == OP_NOT_EQUALS)
			fprintf(emitter.file, "	setne al\n");
		
		fprintf(emitter.file, "	mov qword [rbp - %u], rax\n", location * 8);
		return location;
	}

	fprintf(emitter.file, "	mov rax, qword [rbp - %u]\n", left * 8);
	switch (op->op) {
		case OP_ADD:
		case OP_SUB:
			fprintf(emitter.file, "	%s rax, qword [rbp - %u]\n", op->op == OP_ADD ? "add" : "sub", right * 8);
			break;
		case OP_MUL:
			fprintf(emitter.file, "	mul qword [rbp - %u]\n", right * 8);
			break;

			// div freezes?
		default:
			printf("emit_binary_op: unhandled operator\n");
			error();
	}
	fprintf(emitter.file, "	mov qword [rbp - %u], rax\n", location * 8);
	return location;
}

void emit_var_decl(AST_Var_Decl* decl) {
	stack_loc location = allocate_stack();

	Variable var = {
		.location = location,
		.token = decl->name,
	};

	// check for duplicate var names
	// todo: do this in the tree instead?
	for (u32 i = 0; i < MAX_VARS; i++) {
		const Token* other = &emitter.context.vars[i].token;
		if (var.token.len == other->len) {
			if (strncmp(var.token.str, other->str, var.token.len) == 0) {
				printf("error: %.*s is already defined\n", var.token.len, var.token.str);
				error();
			}
		}
	}

	emitter.context.vars[emitter.context.num_vars++] = var;

	if (decl->assign != NULL) {
		u32 assign_loc = emit_node(decl->assign);
		fprintf(emitter.file, "	mov rax, qword [rbp - %u]\n", assign_loc * 8);
		fprintf(emitter.file, "	mov qword [rbp - %u], rax\n", location * 8);
	}
}

void emit_assign(AST_Assign* assign, stack_loc rhs_loc) {
	Variable* var = find_var_by_name(&assign->lhs);

	if (var == NULL) {
		printf("emit_assign: variable not found!\n");
		error();
	}

	fprintf(emitter.file, "	; assign\n");
	fprintf(emitter.file, "	mov rax, qword [rbp - %u]\n", rhs_loc * 8);
	fprintf(emitter.file, "	mov qword [rbp - %u], rax\n", var->location * 8);
}

void emit_if(AST_Conditional* if_stmt) {
	stack_loc result_loc = emit_node(if_stmt->condition);
	
	u32 label = emitter.label++;

	fprintf(emitter.file, "	; if statement\n");
	fprintf(emitter.file, "	cmp qword [rbp - %u], 0\n", result_loc * 8);
	fprintf(emitter.file, "	je _label%u\n", label);

	emit_node(if_stmt->body);

	fprintf(emitter.file, "_label%u:\n", label);
}

void emit_while(AST_Conditional* while_stmt) {
	u32 loop_label = emitter.label++;
	u32 exit_label = emitter.label++;

	fprintf(emitter.file, "	; while statement\n");
	fprintf(emitter.file, "_label%u:\n", loop_label);
	stack_loc result_loc = emit_node(while_stmt->condition);
	fprintf(emitter.file, "	cmp qword [rbp - %u], 0\n", result_loc * 8);
	fprintf(emitter.file, "	je _label%u\n", exit_label);
	emit_node(while_stmt->body);
	fprintf(emitter.file, "	jmp _label%u\n", loop_label);
	fprintf(emitter.file, "_label%u:\n", exit_label);
}

void emit_func_decl(AST_Func_Decl* node) {
	// calculate ahead of time, how much stack space this function is gonna need to allocate
	// for variables, arguments and temporary values.
	u32 required_stack_alloc = get_required_stack_size((AST_Node*) node) * 8;

	// align the stack to 16 bytes
	// (sysv amd64 abi requires this)
	if (required_stack_alloc & 0b1111) {
		required_stack_alloc &= ~0b1111;
		required_stack_alloc += 16;
	}

	// reset the context, clear any previous local variables etc.
	memset(&emitter.context, 0, sizeof(Local_Context));
	emitter.context.alloc = 1; // start at ebp - 8

	// function prologue
	fprintf(emitter.file, "global %.*s\n", node->name.len, node->name.str);
	fprintf(emitter.file, "%.*s:\n", node->name.len, node->name.str);
	fprintf(emitter.file, "	push rbp\n");
	fprintf(emitter.file, "	mov rbp, rsp\n");
	fprintf(emitter.file, "	sub rsp, %u\n", required_stack_alloc);

	// put arguments passed in registers into stack space (for now)
	Variable* arg_vars[MAX_ARGS];
	for (u32 i = 0; i < node->num_args; i++) {
		Variable* var = &emitter.context.vars[emitter.context.num_vars++];
		var->location = allocate_stack();
		var->token = node->args[i];
		arg_vars[i] = var;
	}

	// copy arguments from registers into stack
	for (u32 i = 0; i < node->num_args; i++) {
		fprintf(emitter.file, "	mov qword [rbp - %u], %s\n", arg_vars[i]->location * 8, sysv_call_regs[i]);
	}

	if (node->num_args > MAX_ARGS) {
		printf("emit_func_decl error: too many args\n");
		error();
	}

	// emit the function body
	emit_node(node->body);

	// function epilogue
	fprintf(emitter.file, "	mov rsp, rbp\n");
	fprintf(emitter.file, "	pop rbp\n");
	fprintf(emitter.file, "	ret\n");
}

stack_loc emit_func_call(AST_Func_Call* call) {
	// todo: keep track of declared funcs

	stack_loc result_loc = allocate_stack();

	// emit code for evaluating the arguments
	stack_loc locs[MAX_ARGS];
	for (u32 i = 0; i < call->num_args; i++) {
		locs[i] = emit_node(call->args[i]);
	}

	if (call->num_args > MAX_ARGS) {
		printf("emit_func_call error: too many args\n");
		error();
	}

	// copy arguments from stack into registers
	for (u32 i = 0; i < call->num_args; i++) {
		fprintf(emitter.file, "	mov %s, qword [rbp - %u]\n", sysv_call_regs[i], locs[i] * 8);
	}
	
	fprintf(emitter.file, "	call %.*s\n", call->name.len, call->name.str);
	// move return value into temporary
	fprintf(emitter.file, "	mov qword [rbp - %u], rax\n", result_loc * 8);
	return result_loc;
}

void emit_return(AST_Return* ret) {
	stack_loc result_loc = emit_node(ret->expr);

	fprintf(emitter.file, "	; return\n");
	fprintf(emitter.file, "	mov rax, qword [rbp - %u]\n", result_loc * 8);
	fprintf(emitter.file, "	mov rsp, rbp\n");
	fprintf(emitter.file, "	pop rbp\n");
	fprintf(emitter.file, "	ret\n");
}

stack_loc emit_node(AST_Node* node) {
	switch (node->type) {
		case AST_PROGRAM: {
			AST_Program* program = (AST_Program*) node;
			for (u32 i = 0; i < program->num_defs; i++) {
				emit_node(program->defs[i]);
			}
			return 0;
		}
		case AST_INT_LITERAL:
			return emit_number((AST_Number*) node);
		case AST_STR_LITERAL:
			return emit_string((AST_String*) node);
		case AST_BIN_OP: {
			AST_Binary_Op* op = (AST_Binary_Op*) node;
			stack_loc left = emit_node(op->left);
			stack_loc right = emit_node(op->right);
			return emit_binary_op(op, left, right);
		}
        case AST_BLOCK: {
			AST_Block* block = (AST_Block*) node;
            for (u32 i = 0; i < block->num_statements; i++) {
			    emit_node(block->statements[i]);
			}
			return 0;
		}
		case AST_VAR_DECL:
			emit_var_decl((AST_Var_Decl*) node);
			return 0;
		case AST_VAR:
			return emit_var((AST_Var*) node);
		case AST_ASSIGN: {
			AST_Assign* assign = (AST_Assign*) node;
			stack_loc rhs_loc = emit_node(assign->rhs);
			emit_assign((AST_Assign*) assign, rhs_loc);
			return 0;
		}
		case AST_FUNC_DECL: {
			emit_func_decl((AST_Func_Decl*) node);
			return 0;
		}
		case AST_FUNC_CALL: {
			return emit_func_call((AST_Func_Call*) node);
		}
		case AST_IF: {
			emit_if((AST_Conditional*) node);
			return 0;
		}
		case AST_WHILE: {
			emit_while((AST_Conditional*) node);
			return 0;
		}
		case AST_RETURN:
			emit_return((AST_Return*) node);
			return 0;
        default:
        	printf("emit_node: unhandled node type %u\n", node->type);
	        error();
	}

	return 0;
}

void emit(AST_Node* root, const char* path) {
	memset(&emitter, 0, sizeof(Emit_State));

	emitter.file = fopen(path, "w");
	if (emitter.file == NULL) {
		perror("");
		error();
	}

	fprintf(emitter.file, "section .text\n");
	fprintf(emitter.file, "extern exit ; temporary solution\n");
	fprintf(emitter.file, "extern printf ; temporary solution\n");
	emit_node(root);

	// emit all string literals
	for (u32 i = 0; i < emitter.num_string_literals; i++) {
		const Token* token = &emitter.string_literals[i];

		fprintf(emitter.file, "_str%u: db ", i);

		// convert to nasm string
		u32 pos = 1; // skip first "
		while (pos < token->len - 1) {
			if (token->str[pos] == '\\') {
				if (pos + 1 >= token->len - 1 || token->str[pos + 1] != 'n') {
					printf("emit error: invalid escape character");
					error();
				}

				fprintf(emitter.file, "10, ");
				pos += 2;
				continue;
			}

			fprintf(emitter.file, "%u, ", (u32)token->str[pos]);
			pos++;
		}
		fprintf(emitter.file, "0\n");
	}

	fclose(emitter.file);
}
