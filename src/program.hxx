
class Program {
private:
	std::vector<Token> input;
	std::vector<Block> output;
	std::stack<WORD> parameter_stack;
	std::optional<Block *> current_block;
	Scope global_scope;

	std::optional<WORD>
	token_value(Token token, Scope &current_scope);

	void
	error(bool condition, TokenIterator &at, const char *fmt, ...);

	void
	execute_block(TokenIterator &it, Scope *scope);

	void
	execute_write(TokenIterator &it, Scope *scope);

	void
	execute_move(TokenIterator &it, Scope *scope);

	void
	execute_declare(TokenIterator &it, Scope *scope);

	void
	execute_binary_operation(TokenIterator &it, Scope *scope,
	const char *op_name, const char *seperator, bool direction,
	std::function<WORD(WORD,WORD)> result);

	void
	execute_namespaced(TokenIterator &it, Scope *scope, bool is_explicit);

	void
	execute_procedure(TokenIterator &it, Scope *scope);

	void
	execute_parameters(TokenIterator &it, Scope *scope);

	void
	execute_invoke(TokenIterator &it, Scope *scope);

	void
	execute_if(TokenIterator &it, Scope *scope);

	void
	execute_list(TokenIterator &it, Scope *scope,
	std::vector<WORD> &values);

	void
	execute_token_list(TokenIterator &it, Scope *scope,
	std::vector<Token> &values);

	void
	execute_include(TokenIterator &it, Scope *scope);

	void
	execute_scope(std::vector<Token> statements, Scope *parent);

	std::vector<Token>
	parse_string(std::string filename, std::string source);
public:
	Program(void);

	void
	parse_input(std::string filename, std::string);

	void
	execute(void);

	std::vector<uint8_t>
	finalize(void);
};
