#include "../../includes/utils.hpp"

std::string	trimWhitespace(std::string &line, std::string whiteSpace) {
	std::string::iterator	start = line.begin(), end = line.end() - 1;
	std::string				result;

	while (whiteSpace.find(*start) != std::string::npos)
		start++;
	while (end >= start && whiteSpace.find(*end) != std::string::npos)
		end--;
	result.append(start, end + 1);
	return result;
}

std::vector<std::string>	split(const std::string &str) {
	std::vector<std::string>	result;
	std::stringstream	stream(str);
	std::string			word;

	stream >> word;
	while (!stream.eof()) {
		result.push_back(word);
		stream >> word;
	}
	result.push_back(word);
	return result;
}

std::vector<std::string>	split(const std::string &str, const std::string sep) {
	std::vector<std::string>	result;
	std::string					temp(str);

	for (size_t pos=temp.find(sep); pos!=std::string::npos; pos=temp.find(sep)) {
		result.push_back(temp.substr(0, pos));
		temp.erase(0, pos + sep.length());
	}
	result.push_back(temp);
	return result;
}

void	printVector(const std::string &name, const std::vector<std::string> &vec, std::ostream *out) {
	(*out) << std::left << std::setw(20) << name << ": ";
	for (size_t i=0; i<vec.size(); i++)
		(*out) << "\"" << vec[i] << "\" ";
	(*out) << std::endl;
}

void	printMap(const std::string &name, const std::map<int, std::string> &map, std::ostream *out) {
	std::map<int, std::string>::const_iterator	it = map.begin();

	(*out) << std::left << std::setw(20) << name << ": ";
	while (it != map.end()) {
		(*out) << "{" << it->first << ", \"" << it->second << "\"}  ";
		it++;
	}
	(*out) << std::endl;
}

void	printMap(const std::string &name, const std::map<std::string, std::string> &map, std::ostream *out) {
	std::map<std::string, std::string>::const_iterator	it = map.begin();

	(*out) << std::left << std::setw(20) << name << ": ";
	while (it != map.end()) {
		(*out) << "{\"" << it->first << "\", \"" << it->second << "\"}  ";
		it++;
	}
	(*out) << std::endl;
}
