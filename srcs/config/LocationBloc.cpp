#include "../../includes/LocationBloc.hpp"

LocationBloc::LocationBloc() {
	_path = NULL;
	_validLocation = true;
	_empty = true;
	_route.clear();
	_allowedMethods.clear();
	_return.first = 0;
	_return.second.clear();
	_alias.clear();
	_autoIndex = false;
	_indexes.clear();
	_cgis.clear();
	_uploadPath.clear();
}

LocationBloc::LocationBloc(const char* path) {
	_path = path;
	_validLocation = true;
	_empty = true;
	_route.clear();
	_allowedMethods.clear();
	_return.first = 0;
	_return.second.clear();
	_alias.clear();
	_autoIndex = false;
	_indexes.clear();
	_cgis.clear();
	_uploadPath.clear();
}

LocationBloc::~LocationBloc() {}

LocationBloc	&LocationBloc::operator=(const LocationBloc &rhs) {
	if (this != &rhs) {
		_route = rhs._route;
		_allowedMethods = rhs._allowedMethods;
		_return = rhs._return;
		_alias = rhs._alias;
		_autoIndex = rhs._autoIndex;
		_indexes = rhs._indexes;
		_cgis = rhs._cgis;
		_uploadPath = rhs._uploadPath;
	}
	return *this;
}

void	LocationBloc::setAllowedMethods(std::vector<std::string> &args) {
	_allowedMethods.clear();
	for (size_t i=1; i<args.size(); i++) {
		if (args[i] != "GET" && args[i] != "POST" && args[i] != "DELETE") {
			Logger::log(Logger::ERROR, "%s: invalid method: \"%s\"", _path, args[i].c_str());
			_validLocation = false;
			return;
		}
		if (std::find(_allowedMethods.begin(), _allowedMethods.end(), args[i]) != _allowedMethods.end()) {
			Logger::log(Logger::ERROR, "%s: duplicate method found: \"%s\"", _path, args[i].c_str());
			_validLocation = false;
			return;
		}
		_allowedMethods.push_back(args[i]);
	}
	_empty = false;
}

void	LocationBloc::setReturn(std::vector<std::string> &args) {
	int				redirectCode = 0;
	char*			tmp = NULL;

	if (_return.first)
		return;
	redirectCode = strtol(args[1].c_str(), &tmp, 10);
	if (redirectCode < 300 || redirectCode > 399 || *tmp) {
		Logger::log(Logger::ERROR, "%s: invalid redirect code: \"%s\"", _path, args[1].c_str());
		_validLocation = false;
		return;
	}
	_return = std::make_pair(redirectCode, args[2]);
	_empty = false;
}

void	LocationBloc::setAlias(std::string &arg) {
	_alias = arg;
	_empty = false;
}

void	LocationBloc::setAutoIndex(std::string &arg) {
	if (arg == "on")
		_autoIndex = true;
	else if (arg == "off")
		_autoIndex = false;
	else {
		Logger::log(Logger::ERROR, "%s: invalid value for autoindex: \"%s\"", _path, arg.c_str());
		_validLocation = false;
		return;
	}
	_empty = false;
}

void	LocationBloc::setIndexes(std::vector<std::string> &args) {
	_indexes.clear();
	for (size_t i=1; i<args.size(); i++) {
		if (std::find(_indexes.begin(), _indexes.end(), args[i]) == _indexes.end())
			_indexes.push_back(args[i]);
	}
	_empty = false;
}

void	LocationBloc::addCgi(std::vector<std::string> &args) {
	_cgis[args[1]] = args[2];
	_empty = false;
}

void	LocationBloc::setUploadPath(std::string &arg) {
	_uploadPath = arg;
	_empty = false;
}

void	LocationBloc::parseLocationKey(std::vector<std::string> &args) {
	if (args.size() < 2 || ((args[0] == "return" || args[0] == "cgi") && args.size() < 3)) {
		Logger::log(Logger::ERROR, "%s: missing value for key: \"%s\"", _path, args[0].c_str());
		_validLocation = false;
		return;
	}
	if (args[0] == "allow_methods")
		setAllowedMethods(args);
	else if (args[0] == "return" && args.size() == 3)
		setReturn(args);
	else if (args[0] == "alias" && args.size() == 2)
		setAlias(args[1]);
	else if (args[0] == "autoindex" && args.size() == 2)
		setAutoIndex(args[1]);
	else if (args[0] == "index")
		setIndexes(args);
	else if (args[0] == "cgi" && args.size() == 3)
		addCgi(args);
	else if (args[0] == "upload_path" && args.size() == 2)
		setUploadPath(args[1]);
	else {
		Logger::log(Logger::ERROR, "%s: unrecognized directive: \"%s\"", _path, args[0].c_str());
		_validLocation = false;
		return;
	}
}

bool	LocationBloc::parseLocationBloc(std::ifstream &file, std::string &route) {
	std::string					line;
	std::vector<std::string>	args;
	bool						closedLocation = false;

	_route = route;
	while (std::getline(file, line)) {
		line = trimWhitespace(line, " \t\n\r\f\v");
		if (line.empty() || line[0] == '#')
			continue;
		args = split(line);
		if (args[0] == "}" && args[0].size() == 1) {
			closedLocation = true;
			break;
		}
		parseLocationKey(args);
	}
	if (!closedLocation)
		Logger::log(Logger::FATAL, "%s: missing closing bracket '}'", _path);
	if (_empty && _validLocation) {
		Logger::log(Logger::ERROR, "%s: empty location bloc found", _path);
		_validLocation = false;
	}
	if (!_validLocation) {
		Logger::log(Logger::WARNING, "will try to continue while ignoring current location bloc due to aformentioned error");
		Logger::log(Logger::WARNING, "fix error to ensure expected behaviour");
		return false;
	}
	return true;
}

std::string	LocationBloc::getRoute() const {
	return _route;
}

std::vector<std::string>	LocationBloc::getAllowedMethods() const {
	return _allowedMethods;
}

std::pair<int, std::string>	LocationBloc::getReturn() const {
	return _return;
}

std::string	LocationBloc::getAlias() const {
	return _alias;
}

bool	LocationBloc::getAutoIndex() const {
	return _autoIndex;
}

std::vector<std::string>	LocationBloc::getIndexes() const {
	return _indexes;
}

std::map<std::string, std::string>	LocationBloc::getCgis() const {
	return _cgis;
}

std::string	LocationBloc::getUploadPath() const {
	return _uploadPath;
}

bool	LocationBloc::containsMethod(const std::string &method) const {
	if (_allowedMethods.empty())
		return true;
	for (size_t i=0; i<_allowedMethods.size(); i++) {
		if (_allowedMethods[i] == method)
			return true;
	}
	return false;
}

bool	LocationBloc::hasCgis(std::string &ext) const {
	if (_cgis.find(ext) == _cgis.end())
		return false;
	return true;
}

void	LocationBloc::printLocationBloc(int indent) const {
	std::string	topSection("╔═════════");
	std::string	wall("║ ");
	std::string	wallChar("║ ");
	std::string	botSection("╚════════════════════════");
	std::ostream	*out = &std::cout;

	if (Logger::getLogLevel() >= Logger::DEBUG &&  Logger::getFilteredOutput())
		out = Logger::getDebugFile();
	for (int i=0; i<indent; i++) {
		topSection = wallChar + topSection;
		botSection = wallChar + botSection;
		wall += wallChar;
	}
	(*out) << Logger::getColorStr(Logger::INFO) << topSection << " Location" << std::endl;
	if (!_route.empty()) {
		(*out) << wall;
		(*out) << std::left << std::setw(20) << "_route" << ": \"" << _route << "\"" << std::endl;
	}
	if (!_allowedMethods.empty()) {
		(*out) << wall;
		printVector("_allowedMethods", _allowedMethods, out);
	}
	if (_return.first) {
		(*out) << wall;
		(*out) << std::left << std::setw(20) << "_return" << ": " << _return.first << " \"" << _return.second << "\"" << std::endl;
	}
	if (!_alias.empty()) {
		(*out) << wall;
		(*out) << std::left << std::setw(20) << "_alias" << ": \"" << _alias << "\"" << std::endl;
	}
	(*out) << wall;
	(*out) << std::left << std::setw(20) << "_autoindex" << ": \"" << (_autoIndex ? "true" : "false") << "\"" << std::endl;
	if (!_indexes.empty()) {
		(*out) << wall;
		printVector("_indexes", _indexes, out);
	}
	if(!_cgis.empty()) {
		(*out) << wall;
		printMap("_cgis", _cgis, out);
	}
	if (!_uploadPath.empty()) {
		(*out) << wall;
		(*out) << std::left << std::setw(20) << "_uploadPath" << ": \"" << _uploadPath << "\"" << std::endl;
	}
	(*out) << botSection << Logger::getColorStr(Logger::NONE) << std::endl;
}