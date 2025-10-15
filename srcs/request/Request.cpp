#include "../../includes/Request.hpp"

Request::Request() {
	_state = INIT;
	_status = REQUEST_DEFAULT_STATUS;
	_isChunked = false;
	_contentLength = -1;
	_isLocation = false;
}

Request::Request(std::string &connectionIp) {
	_state = INIT;
	_connectionIp = connectionIp;
	_status = REQUEST_DEFAULT_STATUS;
	_isChunked = false;
	_contentLength = -1;
	_isLocation = false;
}

Request::~Request() {}

/*
** --------------------------------- Getters -------------------------------------
*/

int	Request::getState() const {return _state;}

int	Request::getStatus() const {return _status;}

std::string	Request::getMethod() const {return _method;}

std::string	Request::getTarget() const {return _target;}

std::string	Request::getQuery() const {return _query;}

std::string	Request::getHost() const {return _host;}

std::map<std::string, std::string>	Request::getHeaders() const {return _headers;}

std::string	Request::findHeaderContent(const std::string &header) const {
	std::map<std::string, std::string>::const_iterator	it = _headers.find(header);

	if (it == _headers.end())
		return std::string();
	return it->second;
}

std::string	Request::getBody() const {return _body;}

ServerBloc	Request::getServer() const {return _server;}

bool	Request::getIsLocation() const {return _isLocation;}

LocationBloc	Request::getLocation() const {return _location;}

std::string Request::getNewline() const {return _newline; }

/*
** --------------------------------- Setters -------------------------------------
*/

void	Request::setFirstErrorStatus(int status, std::string debugMSG) {
	if (_status == REQUEST_DEFAULT_STATUS) {
		_status = status;
		_debugMSG = debugMSG;
	}
}

void	Request::setState(e_state state) {
	_state = state;
}

void	Request::setStatus(int status) {
	_status = status;
}

/*
** --------------------------------- Parsing -------------------------------------
*/

void	Request::findDefaultServer(const std::vector<ServerBloc> &servers) {
	for (size_t i=0; i<servers.size(); i++) {
		if (servers[i].containsListen(_connectionIp)) {
			_server = servers[i];
			Logger::log(Logger::DEBUG, "Found Default server for ip: %s. With index: %d", _connectionIp.c_str(), i);
			break;
		}
	}
}

bool	Request::findNewlineFormat() {
	if (_buffer.find("\r\n") != std::string::npos)
		_newline = "\r\n";
	else if (_buffer.find("\n") != std::string::npos)
		_newline = "\n";
	else
		return false;
	return true;
}

void	Request::extractQuery() {
	size_t	queryPos = _target.find('?');

	if (queryPos == std::string::npos)
		return;
	_query = _target.substr(queryPos + 1);
	_target.erase(queryPos);
}

void	Request::storeRequestLine(const size_t &nlPos) {
	std::string					line = _buffer.substr(0, nlPos);
	std::vector<std::string>	args = split(line);

	_buffer.erase(0, nlPos + _newline.size());
	if (args.size() == 3) {
		_method = args[0];
		_target = args[1];
		extractQuery();
		if (args[2] != "HTTP/1.1")
			setFirstErrorStatus(HTTP_VERSION_NOT_SUPPORTED, "Unsupported HTTP version");
	}
	else
		setFirstErrorStatus(BAD_REQUEST, "Wrong number of arguments on first line of request");
}

bool	Request::invalidCharsInHeader(const std::string &key) const {
	for (std::string::const_iterator it=key.begin(); it!=key.end(); ++it) {
		if (*it <= 32 || *it >= 127)
			return true;
	}
	return false;
}

void	Request::findServerAndLocation(const std::vector<ServerBloc> &servers) {
	std::vector<std::string>	splitHost = split(_host, ":");

	if (splitHost.size() != 2 || splitHost[1].empty()) {
		setFirstErrorStatus(BAD_REQUEST, "Invalid host header");
		return;
	}
	if (_connectionIp.substr(_connectionIp.find(":") + 1) != splitHost[1]) {
		setFirstErrorStatus(BAD_REQUEST, "Mismatched port in connection address and host header");
		return;
	}
	if (_connectionIp != _host) {
		for (size_t i=0; i<servers.size(); i++) {
			if (servers[i].containsListen(_connectionIp) && servers[i].containsServerName(splitHost[0])) {
				_server = servers[i];
				Logger::log(Logger::DEBUG, "Found targeted server for hostname: %s. Or ip: %s. With index: %d", splitHost[0].c_str(), _host.c_str(), i);
				break;
			}
			if (i + 1 == servers.size())
				Logger::log(Logger::WARNING, "unknown server name: \"%s\". Using default server for host ip", splitHost[0].c_str());
		}
	}
	if (_server.findLocation(_target, _location))
		_isLocation = true;
}

void	Request::storeHostHeader(std::string &value, const std::vector<ServerBloc> &servers) {
	size_t		colonPos = value.find(":");

	if (!_host.empty()){
		setFirstErrorStatus(BAD_REQUEST, "Duplicate host header");
		return;
	}
	if (colonPos == 0 || colonPos == value.size() - 1 || colonPos == std::string::npos)
		setFirstErrorStatus(BAD_REQUEST, "missing ip or port in host header");
	for (size_t i=0; i<colonPos; i++) {
		if (!isalnum(value[i]) && value[i] != '-' && value[i] != '.') {
			setFirstErrorStatus(BAD_REQUEST, "Invalid ip in host header");
			break;
		}
	}
	if (colonPos != std::string::npos) {
		char	*tmp = NULL;
		long	port = strtol(value.substr(colonPos + 1).c_str(), &tmp, 10);
		if (*tmp || !isdigit(value.substr(colonPos + 1)[0]) || port < 1 || port > 65535)
			setFirstErrorStatus(BAD_REQUEST, "Invalid port in host header");
	}
	_host = value;
	findServerAndLocation(servers);
}

void	Request::storeParsingUsedHeader(const std::string &key, std::string &value) {
	if (key == "transfer-encoding" && value == "chunked")
		_isChunked = true;
	else if (key == "content-length") {
		char		*tmp = NULL;
		long long	length = strtoll(value.c_str(), &tmp, 10);
		if (*tmp || !isdigit(value[0]) || length < 0)
			setFirstErrorStatus(BAD_REQUEST, "Invalid size in content-length header");
		_contentLength = length;
	}
}

void	Request::storeHeader(const size_t &nlPos, const std::vector<ServerBloc> &servers) {
	std::string	line = _buffer.substr(0, nlPos + _newline.size());
	size_t		colonPos = line.find(":");
	std::string	key, value;

	_buffer.erase(0, nlPos + _newline.size());
	if (line == _newline) {
		_state = BODY;
		return;
	}
	line = line.substr(0, nlPos);
	if (colonPos == std::string::npos) {
		setFirstErrorStatus(BAD_REQUEST, "Invalid header format (missing colon)");
		return;
	}
	key = line.substr(0, colonPos);
	value = line.substr(colonPos + 1);
	value = trimWhitespace(value, " \t");
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
	if (_headers.find(key) != _headers.end() || (key == "host" && !_host.empty()) || invalidCharsInHeader(key))
		setFirstErrorStatus(BAD_REQUEST, "Duplicate or invalid header");
	else if (key == "host")
		storeHostHeader(value, servers);
	else
		_headers[key] = value;
	if (key == "transfer-encoding" || key == "content-length")
		storeParsingUsedHeader(key, value);
}

void	Request::parseChunked() {
	size_t	nlSize = _newline.size();
	long	chunkSize;

	while (true) {
		std::string	chunk = _buffer;
		size_t	pos = chunk.find(_newline);
		if (pos == std::string::npos)
			return;
		char	*ptr = NULL;
		chunkSize = strtol(chunk.substr(0, pos).c_str(), &ptr, 16);
		if (*ptr || chunkSize < 0) {
			setFirstErrorStatus(BAD_REQUEST, "Invalid chunk size");
			_state = VERIFICATION;
			return;
		}
		chunk.erase(0, pos + nlSize);
		if (chunk.size() < chunkSize + nlSize)
			return;
		if (chunk.substr(chunkSize, nlSize) != _newline) {
			setFirstErrorStatus(BAD_REQUEST, "Expected newline not found in chunk");
			_state = VERIFICATION;
			return;
		}
		if (chunkSize == 0) {
			_state = VERIFICATION;
			return;
		}
		_body.append(chunk.substr(0, chunkSize));
		_buffer.erase(0, pos + chunkSize + 2 * nlSize);
	}
}

void	Request::storeBody() {
	if (!_isChunked) {
		_body.append(_buffer);
		_buffer.clear();
		if (static_cast<long long>(_body.size()) >= _contentLength)
			_state = VERIFICATION;
	}
	else
		parseChunked();
}

void	Request::parse(const std::string &buffer, const std::vector<ServerBloc> &servers) {
	size_t	nlPos;
	_buffer.append(buffer);
	Logger::log(Logger::DEBUG, "Parsing request with buffer: \n~~<%s>~~", _buffer.c_str());
	while (true) {
		if (_state == INIT) {
			findDefaultServer(servers);
			if (!findNewlineFormat())
				return;
			_state = REQUEST_LINE;
		}
		if (_state == REQUEST_LINE) {
			nlPos = _buffer.find(_newline);
			if (nlPos == std::string::npos)
				return;
			storeRequestLine(nlPos);
			_state = HEADERS;
		}
		if (_state == HEADERS) {
			nlPos = _buffer.find(_newline);
			if (nlPos == std::string::npos)
				return;
			storeHeader(nlPos, servers);
		}
		else if (_state == BODY) {
			if (_isChunked && _contentLength != -1) {
				setFirstErrorStatus(BAD_REQUEST, "Both transfer-encoding and content-length header present");
				_state = VERIFICATION;
				continue;
			}
			storeBody();
			if (_state == BODY)
				return;
		}
		if (_state == VERIFICATION)
			requestValid();
		if (_state == READY) {
			Logger::log(Logger::DEBUG, "Request \"%s\" on host \"%s\" is ready with status \"%d\"", _method.c_str(), _host.c_str(), _status);
			if (Logger::getLogLevel() >= Logger::DEBUG && Logger::getFilteredOutput())
				printRequest();
			return;
		}
	}
}

/*
** --------------------------------- Checks -------------------------------------
*/

void	Request::requestValid() {
	if (_status == REQUEST_DEFAULT_STATUS) {
		if (_host.empty())
			setFirstErrorStatus(BAD_REQUEST, "Missing host header");
		else if ((_method != "GET" && _method != "POST" && _method != "DELETE" && _method != "BREW") || (_method != "BREW" && _isLocation && !_location.containsMethod(_method)))
			setFirstErrorStatus(METHOD_NOT_ALLOWED, "Self explanatory");
		else if (_method == "POST" && _contentLength == -1 && !_isChunked)
			setFirstErrorStatus(LENGTH_REQUIRED, "Missing length for POST request");
		else if ((!_body.empty() && _contentLength == -1 && !_isChunked) || (_body.empty() && (_contentLength != -1 || _isChunked)))
			setFirstErrorStatus(BAD_REQUEST, "Contradictive body size and size-related headers");
		else if (_contentLength != -1 && static_cast<long long>(_body.size()) < _contentLength)
			setFirstErrorStatus(BAD_REQUEST, "Smaller body than expected");
		else if (_contentLength != -1 && static_cast<long long>(_body.size()) > _contentLength)
			setFirstErrorStatus(CONTENT_TOO_LARGE, "Body exceding expected content Length");
		else if (!_body.empty() && _body.size() > _server.getMaxBodySize())
			setFirstErrorStatus(CONTENT_TOO_LARGE, "Body exceding max allowed size by server config");
	}
	_state = READY;
}

/*
** --------------------------------- Logs -------------------------------------
*/

void	Request::printRequest() const {
	const char*	topSection = "╔═════════";
	const char*	wall = "║";
	const char*	botSection = "╚════════════════════════";
	std::ostream	*out = &std::cout;

	if (Logger::getLogLevel() >= Logger::DEBUG &&  Logger::getFilteredOutput())
		out = Logger::getDebugFile();
	(*out) << Logger::getColorStr(Logger::INFO) << topSection << " Request" << std::endl;
	(*out) << wall << std::left << std::setw(20) << " Method" << ": " << _method << std::endl;
	(*out) << wall << std::left << std::setw(20) << " Target" << ": " << _target << std::endl;
	(*out) << wall << std::left << std::setw(20) << " Query" << ": " << _query << std::endl;
	(*out) << wall << std::left << std::setw(20) << " Host" << ": " << _host << std::endl;
	(*out) << wall << std::left << std::setw(20) << " Status" << ": " << _status << std::endl;
	if (_status != REQUEST_DEFAULT_STATUS)
		(*out) << wall << std::left << std::setw(20) << " DebugMSG" << ": " << _debugMSG << std::endl;
	(*out) << wall << " "  << topSection << " Headers" << std::endl;
	if (_headers.empty())
		(*out) << wall << " " << wall << " (No headers)" << std::endl;
	else {
		for (std::map<std::string, std::string>::const_iterator it=_headers.begin(); it!=_headers.end(); it++)
			(*out) << wall << " " << wall << " " << std::left << std::setw(20) << it->first << ": " << it->second << std::endl;
	}
	(*out) << wall << " "  << botSection << std::endl;
	(*out) << wall << " "  << topSection << " Body" << std::endl;
	if (_body.empty())
		(*out) << wall << " "  << wall << " (No body)" << std::endl;
	else {
		std::stringstream	bodyStream(_body);
		std::string			line;
		while (std::getline(bodyStream, line))
			(*out) << wall << " "  << wall << line << std::endl;
	}
	(*out) << wall << " "  << botSection << std::endl;
	(*out) << botSection << Logger::getColorStr(Logger::NONE) << std::endl;
	_server.printServerBloc(0);
}
