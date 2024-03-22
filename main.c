#include "all.h"

int main(int argc, char* argv[]) {
	if (argc <= 1) {
		printf("usage: compiler <source file>\n");
		error();
	}

	FILE* file = fopen(argv[1], "rb");
	if (file == NULL) {
		perror("");
		error();
	}

	fseek(file, 0, SEEK_END);
	u32 file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* file_contents = malloc(file_size + 1);

	fread(file_contents, 1, file_size, file);
	file_contents[file_size] = 0;

	fclose(file);

	Token tokens[MAX_TOKENS];
	u32 num_tokens;
	lex(file_contents, file_size, tokens, &num_tokens);

	// for (u32 i = 0; i < num_tokens; i++) {
	// 	Token token = tokens[i];
	// 	printf("token [%u] len=%u type=%u: '%.*s'\n", i, token.len, token.type, token.len, token.str);
	// }
	
	AST_Node* expr = parse(file_contents, tokens, num_tokens);
	print_node(expr, 0);

	emit(expr, "output.asm");

	free(file_contents);
	return 0;
}

void error() {
	exit(1);
}
