/*
	Included by main.cxx
*/

struct variable {
	WORD value;
	int constant;
};

class Scope {
private:
	std::unordered_map<std::string, struct variable> variables;
	std::unordered_map<std::string, std::vector<Token>> procedures;
	Scope *parent;
public:
	Scope(Scope *parent)
	{
		this->parent = parent;
	}

	void
	create_procedure(std::string key, std::vector<Token> body)
	{
		this->procedures.insert({key, body});
	}

	std::optional<std::vector<Token>>
	get_procedure(std::string key)
	{
		std::unordered_map<
			std::string,
			std::vector<Token>
		>::iterator p;

		p = this->procedures.find(key);
		if (p == this->procedures.end()) {
			if (this->parent != nullptr)
				return this->parent->get_procedure(key);

			return std::nullopt;
		}

		return p->second;
	}

	void
	create_variable(std::string key, WORD value, int constant)
	{
		this->variables.insert({key, {value, constant}});
	}

	std::optional<WORD>
	get_variable(std::string key)
	{
		std::unordered_map<std::string, struct variable>::iterator v;

		v = this->variables.find(key);
		if (v == this->variables.end()) {
			if (this->parent != nullptr)
				return this->parent->get_variable(key);

			return std::nullopt;
		}

		return v->second.value;
	}

	bool
	set_variable(std::string key, WORD value)
	{
		std::unordered_map<std::string, struct variable>::iterator v;

		v = this->variables.find(key);
		if (v == this->variables.end()) {
			if (this->parent != nullptr)
				return this->parent->set_variable(key, value);
			return false;
		}

		if (v->second.constant)
			return false;

		v->second.value = value;
		return true;
	}
};
