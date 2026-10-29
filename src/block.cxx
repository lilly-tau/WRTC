/*
	To be included from main.cxx
*/

class Block {
public:
	std::vector<uint8_t> data;
	WORD from;
	WORD to;

	Block(WORD from, WORD to)
	{
		this->from = from;
		this->to = to;
	}

	void
	append(std::vector<uint8_t> data)
	{
		this->data.insert(this->data.end(), data.begin(), data.end());
	}
};
