#include "../../includes/ServerBloc.hpp"

ServerBloc::ServerBloc() {
	_path = NULL;
	_validServer = true;
	_empty = true;
	_listens.clear();
	_serverNames.clear();
	_root.clear();
	_errorPages.clear();
	_maxBodySize = 1048576;
	_locations.clear();
}

ServerBloc::ServerBloc(const char* path) {
	_path = path;
	_validServer = true;
	_empty = true;
	_listens.clear();
	_serverNames.clear();
	_root.clear();
	_errorPages.clear();
	_maxBodySize = 1048576;
	_locations.clear();
}

ServerBloc::~ServerBloc() {}

ServerBloc	&ServerBloc::operator=(const ServerBloc &rhs) {
	if (this != &rhs) {
		_listens = rhs._listens;
		_serverNames = rhs._serverNames;
		_root = rhs._root;
		_errorPages = rhs._errorPages;
		_maxBodySize = rhs._maxBodySize;
		_locations = rhs._locations;
	}
	return *this;
}

void	ServerBloc::addListens(std::string &arg) {
	size_t		colonPos = arg.find(":");

	if (colonPos == 0 || colonPos == arg.size() - 1 || colonPos == std::string::npos) {
			Logger::log(Logger::ERROR, "%s: missing ip or port in listen: \"%s\"", _path, arg.c_str());
			_validServer = false;
			return;
	}
	for (size_t i=0; i<colonPos; i++) {
		if (!isalnum(arg[i]) && arg[i] != '-' && arg[i] != '.') {
			Logger::log(Logger::ERROR, "%s: invalid ip in listen: \"%s\"", _path, arg.c_str());
			_validServer = false;
			return;
		}
	}
	if (colonPos != std::string::npos) {
		char	*tmp = NULL;
		long	port = strtol(arg.substr(colonPos + 1).c_str(), &tmp, 10);
		if (*tmp || !isdigit(arg[0]) || port < 1 || port > 65535) {
			Logger::log(Logger::ERROR, "%s: invalid port in listen: \"%s\"", _path, arg.c_str());
			_validServer = false;
			return;
		}
	}
	_listens[arg] = std::make_pair(arg.substr(0, colonPos), arg.substr(colonPos + 1));
	_empty = false;
}

void	ServerBloc::setServerNames(std::vector<std::string> &args) {
	_serverNames.clear();
	for (size_t i=1; i<args.size(); i++)
		_serverNames.push_back(args[i]);
	_empty = false;
}

void	ServerBloc::setRoot(std::string &arg) {
	_root = arg;
	_empty = false;
}

void	ServerBloc::addErrorPages(std::vector<std::string> &args) {
	size_t			size = args.size();
	int				errorCode = 0;
	char*			tmp = NULL;

	for (size_t i=1; i<size-1; i++) {
		errorCode = strtol(args[i].c_str(), &tmp, 10);
		if (errorCode < 400 || errorCode > 599 || *tmp) {
			Logger::log(Logger::ERROR, "%s: invalid error code: \"%s\"", _path, args[i].c_str());
			_validServer = false;
			return;
		}
		_errorPages[errorCode] = args[size-1];
	}
	_empty = false;
}

void	ServerBloc::setMaxBodySize(std::string &arg) {
	char*	tmp = NULL;
	unsigned long long	size;

	size = strtoull(arg.c_str(), &tmp, 10);
	if (!*(tmp+1) && (*tmp == 'k' || *tmp == 'K') && size <= 51200)
		size *= 1024;
	else if (!*(tmp+1) && (*tmp == 'm' || *tmp == 'M') && size <= 50)
		size *= 1024 * 1024;
	else if (*tmp || arg[0] == '-' || size < 1) {
		Logger::log(Logger::ERROR, "%s: invalid client max body size: \"%s\"", _path, arg.c_str());
		_validServer = false;
		return;
	}
	_maxBodySize = size;
	_empty = false;
}

void	ServerBloc::parseServerKey(std::vector<std::string> &args) {
	if (args.size() < 2 || ((args[0] == "error_page" || args[0] == "location") && args.size() < 3)) {
		Logger::log(Logger::ERROR, "%s: missing value for key: \"%s\"", _path, args[0].c_str());
		_validServer = false;
		return;
	}
	if (args[0] == "listen" && args.size() == 2)
		addListens(args[1]);
	else if (args[0] == "server_name")
		setServerNames(args);
	else if (args[0] == "root" && args.size())
		setRoot(args[1]);
	else if (args[0] == "error_page")
		addErrorPages(args);
	else if (args[0] == "client_max_body_size" && args.size() == 2)
		setMaxBodySize(args[1]);
	else {
		Logger::log(Logger::ERROR, "%s: unrecognized directive or bad usage: \"%s\"", _path, args[0].c_str());
		_validServer = false;
		return;
	}
}

bool	ServerBloc::parseServerBloc(std::ifstream &file) {
	std::string					line;
	std::vector<std::string>	args;
	bool						closedServer = false;

	while (std::getline(file, line)) {
		line = trimWhitespace(line, " \t\n\r\f\v");
		if (line.empty() || line[0] == '#')
			continue;
		args = split(line);
		if (args[0] == "}" && args[0].size() == 1) {
			closedServer = true;
			break;
		}
		if (args.size() == 3 && args[0] == "location" && args[2] == "{") {
			LocationBloc	location(_path);
			if (location.parseLocationBloc(file, args[1])) {
				_locations[args[1]] = location;
				_empty = false;
			}
		}
		else
			parseServerKey(args);
	}
	if (!closedServer)
		Logger::log(Logger::FATAL, "%s: missing server closing bracket '}'", _path);
	if (_empty && _validServer) {
		Logger::log(Logger::ERROR, "%s: empty server bloc found", _path);
		_validServer = false;
	}
	if (!_empty && (_root.empty() || _listens.empty())) {
		Logger::log(Logger::ERROR, "%s: missing root or listen from server", _path);
		_validServer = false;
	}
	if (!_validServer) {
		Logger::log(Logger::WARNING, "will try to continue while ignoring current server bloc due to aformentioned error");
		Logger::log(Logger::WARNING, "fix error to ensure expected behaviour");
		return false;
	}
	return true;
}

std::map<std::string, std::pair<std::string, std::string> >	ServerBloc::getListens() const {
	return _listens;
}

std::vector<std::string>	ServerBloc::getServerNames() const {
	return _serverNames;
}

std::string	ServerBloc::getRoot() const {
	return _root;
}

std::map<int, std::string>	ServerBloc::getErrorPages() const {
	return _errorPages;
}

unsigned long long	ServerBloc::getMaxBodySize() const {
	return _maxBodySize;
}

std::map<std::string, LocationBloc>	ServerBloc::getLocations() const {
	return _locations;
}

bool	ServerBloc::findLocation(const std::string &target, LocationBloc &location) const {
	std::string	match;
	size_t		matchLength = 0;

	for (std::map<std::string, LocationBloc>::const_iterator it=_locations.begin(); it!=_locations.end(); it++) {
		if (target.compare(0, it->first.length(), it->first) == 0) {
			if (it->first.length() > matchLength) {
				match = it->first;
				matchLength = it->first.length();
			}
		}
	}
	if (_locations.find(match) == _locations.end())
		return false;
	location = _locations.find(match)->second;
	return true;
}

bool	ServerBloc::areServerNamesfound(std::vector<std::string> &names) const {
	for (size_t i=0; i<names.size(); i++) {
		for (size_t j=0; j<_serverNames.size(); j++) {
			if (names[i] == _serverNames[j]) {
				names.clear();
				names.push_back(_serverNames[j]);
				return true;
			}
		}
	}
	return false;
}

bool	ServerBloc::containsListen(const std::string &target) const {
	std::map<std::string, std::pair<std::string, std::string> >::const_iterator	it = _listens.begin();

	while (it != _listens.end()) {
		if (it->first == target)
			return true;
		it++;
	}
	return false;
}

bool	ServerBloc::containsServerName(const std::string &target) const {
	for (size_t i=0; i<_serverNames.size(); i++) {
		if (_serverNames[i] == target)
			return true;
	}
	return false;
}

bool	ServerBloc::hasErrorPages(int &code) const {
	if (_errorPages.find(code) == _errorPages.end())
		return false;
	return true;
}

void	ServerBloc::printListens(std::ostream *out) const {
	std::map<std::string, std::pair<std::string, std::string> >::const_iterator	it = _listens.begin();

	(*out) << std::left << std::setw(20) << "_listens" << ": ";
	while (it != _listens.end()) {
		(*out) << "\"" << it->first << "\" ";
		it++;
	}
	(*out) << std::endl;
}

void	ServerBloc::printServerBloc(int indent) const {
	std::string	topSection("╔═════════");
	std::string	wall("║ ");
	std::string	wallChar("║ ");
	std::string	botSection("╚════════════════════════");
	std::ostream	*out = &std::cout;

	if (Logger::getLogLevel() >= Logger::DEBUG && Logger::getFilteredOutput())
		out = Logger::getDebugFile();
	for (int i=0; i<indent; i++) {
		topSection = wallChar + topSection;
		botSection = wallChar + botSection;
		wall += wallChar;
	}
	(*out) << Logger::getColorStr(Logger::INFO) << topSection << " Server" << std::endl;
	if (!_listens.empty()) {
		(*out) << wall;
		printListens(out);
	}
	if (!_serverNames.empty()) {
		(*out) << wall;
		printVector("_serverNames", _serverNames, out);
	}
	if (!_root.empty()) {
		(*out) << wall;
		(*out) << std::left << std::setw(20) << "_root" << ": \"" << _root << "\"" << std::endl;
	}
	if (!_errorPages.empty()) {
		(*out) << wall;
		printMap("_errorPages", _errorPages, out);
	}
	if (_maxBodySize) {
		(*out) << wall;
		(*out) << std::left << std::setw(20) << "_maxBodySize" << ": " << _maxBodySize << std::endl;
	}
	if (!_locations.empty()) {
		for (std::map<std::string, LocationBloc>::const_iterator it=_locations.begin(); it!=_locations.end(); it++)
			it->second.printLocationBloc(indent + 1);
	}
	(*out) << Logger::getColorStr(Logger::INFO) << botSection << Logger::getColorStr(Logger::NONE) << std::endl;
}
