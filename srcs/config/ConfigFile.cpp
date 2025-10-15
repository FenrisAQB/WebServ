#include "../../includes/ConfigFile.hpp"

ConfigFile::ConfigFile() {
	_path = "conf/valid/default.conf";
	_serverBlocs.clear();
}

ConfigFile::~ConfigFile() {}

void	ConfigFile::setPath(const char* path) {
	_path = path;
}

void	ConfigFile::duplicateServerName() const {
	std::vector<std::string>	names;

	for (size_t i=0; i<_serverBlocs.size(); i++) {
		names = _serverBlocs[i].getServerNames();
		for (size_t j=i+1; j<_serverBlocs.size(); j++) {
			if (_serverBlocs[j].areServerNamesfound(names))
				Logger::log(Logger::FATAL, "%s: conflicting server names found: \"%s\"", _path, names[0].c_str());
		}
	}
}

void	ConfigFile::parseFile() {
	std::ifstream				file(_path);
	std::string					line;
	std::vector<std::string>	args;

	if (!file.is_open())
		Logger::log(Logger::FATAL, "%s: cannot open file", _path);
	while (std::getline(file, line)) {
		line = trimWhitespace(line, " \t\v\r\n\f");
		if (line.empty() || line[0] == '#')
			continue;
		args = split(line);
		if (args.size() == 2 && args[0] == "server" && args[1] == "{") {
			ServerBloc	server(_path);
			if (server.parseServerBloc(file))
				_serverBlocs.push_back(server);
		}
		else
			Logger::log(Logger::FATAL, "%s: invalid line: \"%s\"", _path, line.c_str());
	}
	if (!_serverBlocs.size())
		Logger::log(Logger::FATAL, "%s: missing server bloc", _path);
	duplicateServerName();
	file.close();
	if (Logger::getLogLevel() >= Logger::INFO) {
		bool	filteredOutput = Logger::getFilteredOutput();
		Logger::setFilteredOutput(false);
		printServerBlocs();
		Logger::setFilteredOutput(filteredOutput);
	}
}

std::vector<ServerBloc>	ConfigFile::getServerBlocs() const {
	return _serverBlocs;
}

void	ConfigFile::printServerBlocs() const {
	std::cout << Logger::getColorStr(Logger::INFO) << "╔═════════ Config File" << std::endl;
	for (size_t i=0; i<_serverBlocs.size(); i++)
		_serverBlocs[i].printServerBloc(1);
	std::cout << Logger::getColorStr(Logger::INFO) << "╚════════════════════════" << Logger::getColorStr(Logger::NONE) << std::endl;
}
