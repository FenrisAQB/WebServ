#ifndef UTILS_HPP
# define UTILS_HPP

# include <vector>
# include <map>
# include <string>
# include <sstream>
# include <iostream>
# include <iomanip>

std::string					trimWhitespace(std::string &line, std::string whiteSpace);
std::vector<std::string>	split(const std::string &str);
std::vector<std::string>	split(const std::string &str, const std::string sep);
void						printVector(const std::string &name, const std::vector<std::string> &vec, std::ostream *out);
void						printMap(const std::string &name, const std::map<int, std::string> &map, std::ostream *out);
void						printMap(const std::string &name, const std::map<std::string, std::string> &map, std::ostream *out);

#endif