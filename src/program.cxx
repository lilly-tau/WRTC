std::optional<WORD>
Program::token_value(Token token, Scope &current_scope)
{
	std::optional<WORD> value;
	uintmax_t from_str;
	char *eptr;

	value = current_scope.get_variable(token.lexeme);
	if (value)
		return *value;

	from_str = strtoumax(token.lexeme.c_str(), &eptr, 16);
	if (*eptr != '\0' || token.lexeme[0] == '\0') {
		return std::nullopt;
	}	

	return (WORD)from_str;
}

void
Program::error(bool condition, TokenIterator &at, const char *fmt, ...)
{
	va_list args;

	if (condition) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		if (at)
			fprintf(
				stderr,
				"\nAt: (token=%s) %s:%u\n",
				at->lexeme.c_str(),
				at->filename.c_str(),
				at->line
			);
		else
			fprintf(
				stderr,
				"\nAt: (token=%s) %s:%u\n",
				at.final_token().lexeme.c_str(),
				at.final_token().filename.c_str(),
				at.final_token().line
			);
		va_end(args);
		exit(1);
	}
}

void
Program::execute_block(TokenIterator &it, Scope *scope)
{
	WORD from = 0, to = 0;
	bool from_specified = false;

	++it;
	if (it->lexeme == "FROM") {
		from_specified = true;
		++it;
		from = *this->token_value(*it, *scope);

		++it;
		this->error(
			it->lexeme != "TO",
			it,
			"Expected FROM after TO."
		);

		++it;
		to = *this->token_value(*it, *scope);
		++it;
	}

	if (from_specified) {
		this->output.push_back(Block(from, to));
	} else {
		Block *last;

		if (this->current_block) {
			last = *this->current_block;
		} else {
			last = &this->output.back();
		}

		this->output.push_back(Block(last->to, 0));
	}
	this->current_block = this->output.data()
		+ this->output.size() - 1;
}

void
Program::execute_write(TokenIterator &it, Scope *scope)
{
	std::vector<uint8_t> data;
	std::vector<WORD> values;

	++it;
	this->execute_list(it, scope, values);
	for (WORD v : values)
		data.push_back(v & 0xFF);

	this->error(
		!this->current_block,
		it,
		"No current block to WRITE to."
	);
	(*this->current_block)->append(data);
}

void
Program::execute_move(TokenIterator &it, Scope *scope)
{
	std::string name;
	std::optional<WORD> value;

	++it;
	value = this->token_value(*it, *scope);
	this->error(
		!value,
		it,
		"Could not use value %s.",
		it->lexeme.c_str()
	);

	++it;
	this->error(it->lexeme != "INTO", it, "Expected INTO in MOVE.");

	++it;
	name = it->lexeme;

	++it;

	this->error(
		!scope->set_variable(name, *value),
		it,
		"Could not retrieve variable %s.",
		name.c_str()
	);
}

void
Program::execute_declare(TokenIterator &it, Scope *scope)
{
	std::string name;
	int global = 0, constant = 0;
	std::optional<WORD> value;

	++it;
	if (it->lexeme == "GLOBAL") {
		++it;
		global = 1;
	} else if (it->lexeme == "LOCAL") {
		++it;
		global = 0;
	}

	if (it->lexeme == "CONSTANT") {
		++it;
		constant = 1;
	} else if (it->lexeme == "MUTABLE") {
		++it;
		constant = 0;
	}

	if (it->lexeme == "VARIABLE") {
		++it;
		name = it->lexeme;
		++it;

		if (it->lexeme == "AS") {
			++it;
			value = this->token_value(*it, *scope);
			this->error(
				!value,
				it,
				"Could not get value %s",
				it->lexeme.c_str()
			);
			++it;
		} else
			value = 0;
	
	
		(global ? &this->global_scope : scope)->create_variable(
			name,
			*value,
			constant
		);
	} else if (it->lexeme == "PROCEDURE") {
		if (constant == 0) {
			this->execute_procedure(
				it,	
				global ? &this->global_scope: scope
			);
		} else {
			std::vector<Token> contents;
			std::string filename;
			LINE line;

			++it;
			name = it->lexeme;
			++it;
			this->error(
				it->lexeme != "AS",
				it,
				"Expected AS in DECLARE CONSTANT PROCDURE."
			);
			++it;

			filename = it->filename;
			line = it->line;

			contents.push_back(
				Token(filename, "WRITE", line)
			);
			this->execute_token_list(it, scope, contents);
			contents.push_back(
				Token(filename, "THEN", line)
			);
			(global
				? &this->global_scope
				: scope
			)->create_procedure(name, contents);
		}
	} else this->error(
		true,
		it,
		"Expected PROCEDURE or VARIABLE in DECLARE."
	);
}

void
Program::execute_binary_operation(TokenIterator &it,
Scope *scope, const char *op_name, const char *seperator,
bool direction, std::function<WORD(WORD,WORD)> result)
{
	std::optional<WORD> increment, initial;
	std::string name;

	++it;
	if (!direction) {
		increment = this->token_value(*it, *scope);
		this->error(
			!increment,
			it,
			"Could not use value %s in %s.",
			it->lexeme.c_str(),
			op_name
		);
	} else {
		name = it->lexeme;
		initial = scope->get_variable(name);
		this->error(
			!initial,
			it,
			"Could not get variable %s in %s.",
			name.c_str(),
			op_name
		);
	}

	++it;
	this->error(
		it->lexeme != seperator,
		it,
		"Expected %s in %s.",
		seperator,
		op_name
	);

	++it;
	if (direction) {
		increment = this->token_value(*it, *scope);
		this->error(
			!increment,
			it,
			"Could not use value %s in %s.",
			it->lexeme.c_str(),
			op_name
		);
	} else {
		name = it->lexeme;
		initial = scope->get_variable(name);
		this->error(
			!initial,
			it,
			"Could not get variable %s in %s.",
			name.c_str(),
			op_name
		);
	}

	this->error(
		!scope->set_variable(
			name,
			result(*initial, *increment)
		),
		it,
		"Could not %s variable %s.",
		op_name,
		name.c_str()
	);
	++it;
}

void
Program::execute_namespaced(TokenIterator &it, Scope *scope, bool is_explicit)
{
	if (is_explicit)
		++it;

	if (it->lexeme == "ADD")
		this->execute_binary_operation(
			it,
			scope,
			"ADD",
			"TO",
			false,
			[](WORD a, WORD b){ return a + b; }
		);
	else if (it->lexeme == "SUBTRACT")
		this->execute_binary_operation(
			it,
			scope,
			"SUBTRACT",
			"FROM",
			false,
			[](WORD a, WORD b){ return a - b; }
		);
	else if (it->lexeme == "MULTIPLY")
		this->execute_binary_operation(
			it,
			scope,
			"MULTIPLY",
			"BY",
			true,
			[](WORD a, WORD b){ return a * b; }
		);
	else if (it->lexeme == "DIVIDE")
		this->execute_binary_operation(
			it,
			scope,
			"DIVIDE",
			"BY",
			true,
			[](WORD a, WORD b){ return a / b; }
		);
	else if (it->lexeme == "MODULO")
		this->execute_binary_operation(
			it,
			scope,
			"MODULO",
			"OF",
			false,
			[](WORD a, WORD b){ return a % b; }
		);
	else if (it->lexeme == "AND")
		this->execute_binary_operation(
			it,
			scope,
			"AND",
			"WITH",
			false,
			[](WORD a, WORD b){ return a & b; }
		);
	else if (it->lexeme == "OR")
		this->execute_binary_operation(
			it,
			scope,
			"OR",
			"WITH",
			false,
			[](WORD a, WORD b){ return a | b; }
		);
	else if (it->lexeme == "XOR")
		this->execute_binary_operation(
			it,
			scope,
			"XOR",
			"WITH",
			false,
			[](WORD a, WORD b){ return a | b; }
		);
	else if (it->lexeme == "SHL")
		this->execute_binary_operation(
			it,
			scope,
			"SHL",
			"BY",
			true,
			[](WORD a, WORD b){ return a << b; }
		);
	else if (it->lexeme == "SHR")
		this->execute_binary_operation(
			it,
			scope,
			"SHR",
			"BY",
			true,
			[](WORD a, WORD b){ return a >> b; }
		);
	else if (it->lexeme == "PUSH") {
		std::optional<WORD> value;

		++it;

		value = this->token_value(*it, *scope);
		this->error(
			!value,
			it,
			"Could not PUSH value %s.",
			it->lexeme.c_str()
		);
		this->parameter_stack.push(*value);

		++it;
	} else if (it->lexeme == "POP") {
		this->error(
			this->parameter_stack.size() == 0,
			it,
			"Stack underflow."
		);

		++it;
		if (it->lexeme == "INTO") {
			++it;
			this->error(
				!scope->set_variable(
					it->lexeme,
					this->parameter_stack.top()
				),
				it,
				"Could not POP into variable %s.",
				it->lexeme.c_str()
			);
			++it;
		}
		this->parameter_stack.pop();
	} else
		this->error(
			true,
			it,
			"Namespaced instruction %s does not exist.",
			it->lexeme.c_str()
		);
}

void
Program::execute_procedure(TokenIterator &it, Scope *scope)
{
	std::string name;
	std::vector<Token> contents;

	++it;
	name = it->lexeme;

	++it;
	while (it->lexeme != "END-PROCEDURE") {
		contents.push_back(*it);
		++it;
	}
	++it;

	scope->create_procedure(name, contents);
}

void
Program::execute_parameters(TokenIterator &it,
Scope *scope)
{
	++it;
	while (it->lexeme != ")") {
		this->error(
			this->parameter_stack.size() == 0,
			it,
			"Stack underflow."
		);
		scope->create_variable(
			it->lexeme,
			this->parameter_stack.top(),
			0
		);
		this->parameter_stack.pop();
		++it;
	}
	++it;
}

void
Program::execute_if(TokenIterator &it, Scope *parent)
{
	std::optional<WORD> value;
	std::vector<Token> if_code, else_code;
	int_fast16_t nesting;
	bool condition, in_if = true;
	Scope scope(parent);

	++it;
	value = this->token_value(*it, scope);
	this->error(
		!value,
		it,
		"Could not find value %s.",
		it->lexeme.c_str()
	);

	++it;
	this->error(it->lexeme != "IS", it, "Expected IS in IF.");

	++it;
	if (it->lexeme == "NOT") {
		++it;
		condition = value != 0;
	} else
		condition = value == 0;

	this->error(it->lexeme != "ZERO", it, "Expected ZERO in IF IS.");

	++it;
	for (nesting = 1; nesting != 0; ++it) {
		if (it->lexeme == "IF") {
			nesting += 1;
			continue;
		} else if (it->lexeme == "END-IF") {
			nesting -= 1;
			continue;
		}

		if (it->lexeme == "ELSE" && nesting == 1) {
			in_if = false;
			continue;
		}

		if (in_if)
			if_code.push_back(*it);
		else
			else_code.push_back(*it);
	}

	if (condition && if_code.size() > 0)
		this->execute_scope(if_code, &scope);
	else if (!condition && else_code.size() > 0)
		this->execute_scope(else_code, &scope);
}

void
Program::execute_list(TokenIterator &it, Scope *scope,
std::vector<WORD> &values)
{
	std::optional<WORD> value;

	this->error(!it, it, "Unterminated list.");
	while (it->lexeme != "THEN") {
		value = this->token_value(*it, *scope);
		this->error(
			!value,
			it,
			"Could not get value %s.",
			it->lexeme.c_str()
		);
		values.push_back(*value);
		
		++it;
		this->error(!it, it, "Unterminated list.");
	}
	++it;
}

void
Program::execute_token_list(TokenIterator &it, Scope *scope,
std::vector<Token> &values)
{

	this->error(!it, it, "Unterminated list.");
	while (it->lexeme != "THEN") {
		values.push_back(*it);
		++it;
		this->error(!it, it, "Unterminated list.");
	}
	++it;
}

void
Program::execute_include(TokenIterator &it, Scope *scope)
{
	std::string source;
	std::vector<Token> tokens;

	++it;
	source = read_file(it->lexeme.c_str());
	tokens = this->parse_string(it->lexeme, source);
	++it;

	execute_scope(tokens, scope);
}

void
Program::execute_scope(std::vector<Token> statements, Scope *scope)
{
	TokenIterator it(statements);

	while (it) {
		if (it->lexeme == "BLOCK")
			this->execute_block(it, scope);
		else if (it->lexeme == "END-BLOCK") {
			if ((*this->current_block)->to
			<= (*this->current_block)->from)
				(*this->current_block)->to =
					(*this->current_block)->from
					+ (*this->current_block)->data.size();

			this->current_block = std::nullopt;
			++it;
		} else if (it->lexeme == "WRITE")
			this->execute_write(it, scope);
		else if (it->lexeme == "MOVE")
			this->execute_move(it, scope);
		else if (it->lexeme == "DECLARE")
			this->execute_declare(it, scope);
		else if (it->lexeme == "$")
			this->execute_namespaced(it, scope, true);
		else if (it->lexeme == "(")
			this->execute_parameters(it, scope);
		else if (it->lexeme == "IF")
			this->execute_if(it, scope);
		else if (it->lexeme == "INCLUDE")
			this->execute_include(it, scope);
		else if (it->lexeme == "REMARK") {
			++it;
			++it;
		} else {
			std::optional<std::vector<Token>> proc;
			std::vector<WORD> to_push;
			Scope new_scope(scope);

			proc = scope->get_procedure(it->lexeme);
			this->error(
				!proc,
				it,
				"Unexpected token %s",
				it->lexeme.c_str()
			);
			++it;

			this->execute_list(it, scope, to_push);
			std::reverse(to_push.begin(), to_push.end());
			for (WORD value : to_push) {
				this->parameter_stack.push(value);
			}

			this->execute_scope(*proc, &new_scope);
		}
	}
}

std::vector<Token>
Program::parse_string(std::string filename, std::string source)
{
	std::vector<Token> ret;
	std::string current_token;
	intptr_t index, place;
	size_t current_line;

	current_line = 1;
	index = 0;
	while (index < source.size()) {
		while (index < source.size()
		&& isspace(source[index])) {
			if (source[index] == '\n')
				current_line += 1;
			index += 1;
		}

		place = index;
		if (source[place] == '\"') {
			std::string current_token;

			index = place += 1;
			while (index < source.size()
			&& source[index] != '\"') {
				if (source[index] == '\\')
					index += 1;

				current_token += source[index];
				index += 1;
			}
			index += 1;
			ret.push_back(
				Token(filename, current_token, current_line)
			);
		} else {
			while (index < source.size()
			&& !isspace(source[index]))
				index += 1;

			if (index - place > 0) {
				current_token = source.substr(
					place,
					index - place
				);

				if (source[place] == '\'')
					place += 1;
				else
					std::transform(
						current_token.begin(),
						current_token.end(),
						current_token.begin(),
						::toupper
					);

				ret.push_back(
					Token(
						filename,
						current_token,
						current_line
					)
				);
			}
		}
	}

	return ret;
}

Program::Program(void) : global_scope(nullptr) { }

void
Program::parse_input(std::string filename, std::string source)
{
	this->input = parse_string(filename, source);
}

void
Program::execute(void)
{
	execute_scope(this->input, &this->global_scope);
}

std::vector<uint8_t>
Program::finalize(void)
{
	std::vector<uint8_t> ret;
	uint8_t *ret_init;
	size_t ret_length;

	ret_init = (uint8_t*)malloc(1);
	ret_length = 0;

	for (Block block : this->output) {
		if (block.to < block.from)
			block.to = block.from + block.data.size();

		if (block.to > ret_length) {
			ret_length = block.to;
			ret_init = (uint8_t*)realloc(
				ret_init,
				ret_length
			);
		}
		memset(
			ret_init + block.from,
			0,
			block.to - block.from
		);
		if (block.data.size() <= block.to - block.from) {
			memcpy(
				ret_init + block.from,
				block.data.data(),
				std::min(
					block.data.size(),
					block.to - block.from
				)
			);
		}
	}

	ret = std::vector(ret_init, ret_init + ret_length);
	free(ret_init);
	return ret;
}
