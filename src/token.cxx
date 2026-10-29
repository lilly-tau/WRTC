class Token {
public:
	std::string lexeme;
	std::string filename;
	LINE line;

	Token() {}
	Token(std::string filename, std::string lexeme, unsigned line)
	{
		this->filename = filename;
		this->lexeme = lexeme;
		this->line = line;
	}
};

class TokenIterator {
private:
	std::vector<Token>::iterator current, end;
public:
	TokenIterator(std::vector<Token> &tokens)
	{
		this->current = tokens.begin();
		this->end = tokens.end();
	}

	bool
	advance(signed count)
	{
		unsigned i;

		for (i = 0; i < std::abs(count)
		&& this->current != this->end; i++)
			count > 0 ? ++this->current : --this->current;

		return this->current != this->end;
	}

	Token
	final_token(void)
	{
		return *(this->end - 1);
	}

	explicit operator bool()
	{
		return this->current != this->end;
	}

	bool
	operator++()
	{
		return this->advance(1);
	}

	bool
	operator--()
	{
		return this->advance(-1);
	}

	Token *
	operator->()
	{
		return &*this->current;
	}

	Token&
	operator*()
	{
		return *this->current;
	}
};
