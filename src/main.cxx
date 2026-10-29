#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

typedef uint_fast64_t WORD;
typedef uint_fast32_t LINE;

#include "token.cxx"
#include "block.cxx"
#include "scope.cxx"

std::string
read_file(const char *path)
{
	FILE *file;
	size_t length;
	char *string;
	std::string ret;

	file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "Could not read file '%s'.\n", path);
		perror(nullptr);
		exit(1);
	}

	fseek(file, 0L, SEEK_END);
	length = ftell(file);
	string = (char*)malloc(length + 1);
	fseek(file, 0L, SEEK_SET);
	length = fread(string, 1, length, file);
	string[length] = 0;
	fclose(file);

	ret = std::string(string);
	free(string);

	return ret;
}

void
write_file(const char *path, std::vector<uint8_t> data)
{
	FILE *file;

	file = fopen(path, "wb");
	if (!file) {
		perror("Could not write file");
		exit(1);
	}
	fwrite(data.data(), 1, data.size(), file);
	fclose(file);
}

#include "program.hxx"
#include "program.cxx"

int
main(int argc, char **argv)
{
	Program program;
	std::vector<uint8_t> output;
	std::string source;

	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <infile> <outfile>"
			<< std::endl;
		exit(1);
	}

	source = read_file(argv[1]);

	program.parse_input(argv[1], source);
	program.execute();
	output = program.finalize();

	write_file(argv[2], output);
	exit(0);
}
