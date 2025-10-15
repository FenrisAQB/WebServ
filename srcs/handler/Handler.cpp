#include "../../includes/Handler.hpp"
#include "../../includes/Cgi.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

HttpError::HttpError(int code): _errorCode(code)
{

}

int HttpError::getCode(void) const {return _errorCode; }

Handler::Handler()
{

}

Handler::~Handler()
{
	if (_isCgi && (_state == READY || g_sigint))
		delete _cgi;
}

Handler::Handler(Request* request, std::vector<pollfd> &fds): _request(request), _state(INIT), _fileFd(-1), _statusCode(request->getStatus()), _isDir(false), _isCgi(false), _cgi(NULL), _fds(&fds)
{
	std::string route = _request->getLocation().getRoute();
	setMethod();
	if (_request->getIsLocation() && _request->getLocation().getReturn().first != 0)
	{
		std::pair<int, std::string> redir = _request->getLocation().getReturn();
		_statusCode = redir.first;
		_content = redir.second;
		if (_content == route)
			throw HttpError(LOOP_DETECTED);
		_state = READY;
		return;
	}
	if (_method != BREW && _statusCode == OK) {
		buildTarget();
		if (_isCgi && _target[_target.size() - 1] != '/')
		{
			_cgi = new Cgi(*this, *_fds);
			return;
		}
		createFd();
	}
	else if (_statusCode == OK) {
		_fileFd = -1;
		brew();
	}
	if (_statusCode > 299)
	{
		if (_fileFd > 2)
			close(_fileFd);
		_fileFd = -1;
		throw HttpError(_statusCode);
	}
}

Handler::Handler(Request* request, int code): _request(request),_state(INIT), _fileFd(-1), _method(GET), _statusCode(code), _isDir(false), _isCgi(false), _cgi(NULL)
{
	if (!_request->getServer().hasErrorPages(_statusCode))
		_state = READY;
	else
	{
		_target = _request->getServer().getErrorPages()[_statusCode];
		createErrorFd();
	}
}


int Handler::getState(void) const {return _state;}

std::string Handler::getContent(void) const {return _content;}

int Handler::getFileFd(void) const {return _fileFd;}

e_method Handler::getMethod(void) const {return _method;}

std::string Handler::getTarget(void) const {return _target;}

int Handler::getStatusCode(void) const {return _statusCode;}

Request* Handler::getRequest(void) const {return _request;}

bool Handler::getIsDir(void) const {return _isDir;}

Cgi* Handler::getCgi(void) const {return _cgi; }

void Handler::setFilefd(int fd) {_fileFd = fd ;}

bool Handler::getIsCgi() const { return _isCgi; }

void	Handler::setState(e_status state) { _state = state; }


bool Handler::hasExtension()
{
	std::string target = _request->getTarget();

	const std::map<std::string, std::string>& cgis = _request->getLocation().getCgis();
	for (std::map<std::string, std::string>::const_iterator it = cgis.begin(); it != cgis.end(); it++)
	{
		if (target.find(it->first) != std::string::npos)
			return true;
	}
	return false;
}

void Handler::setMethod()
{
	std::string method = _request->getMethod();

	if (method == "GET")
		_method = GET;
	else if (method == "POST")
		_method = POST;
	else if (method == "DELETE")
		_method = DELETE;
	else
		_method = BREW;
	if ((_method == GET || _method == POST) && !_request->getLocation().getCgis().empty())
		_isCgi = hasExtension();
}

void Handler::autoIndex(void)
{
	if (!_request->getIsLocation())
	{
		_fileFd = -1;
		_isDir = true;
		return;
	}
	if (_request->getLocation().getAutoIndex() && _method == GET)
	{
		_fileFd = -1;
		_statusCode = AUTOINDEX;
		_state = READY;
		return;
	}
	std::vector<std::string> index = _request->getLocation().getIndexes();
	if (!index.empty())
	{
		DIR* dir = opendir(_target.c_str());
		if (!dir)
			throw HttpError(NOT_FOUND);
		struct dirent* entry;
		std::vector<std::string> entryVect;
		while ((entry = readdir(dir)) != 0)
			entryVect.push_back(entry->d_name);
		for (size_t i = 0; i < index.size(); i++)
		{
			for (size_t j = 0; j < entryVect.size(); j++)
			{
				if (index[i] == entryVect[j])
				{
					if (*(_target.end() - 1) != '/')
						_target += "/";
					_target += index[i];
					createFd();
					closedir(dir);
					return;
				}
			}
		}
		closedir(dir);
	}
	_fileFd = -1;
	throw HttpError(NOT_FOUND);
}

void Handler::createErrorFd(void)
{
	int tempFd;
	tempFd = open(_target.c_str(), O_RDONLY);
	if (tempFd == -1)
	{
		_fileFd = -1;
		_state = READY;
		return;
	}
	_fileFd = tempFd;
}

void Handler::createFd(void)
{
	int fd = -1;
	const char* target = _target.c_str();

	if (_statusCode != OK)
		return createErrorFd();
	if (isDir(target) && _method == GET)
	{
			autoIndex();
			return;
	}
	_isDir = false;
	if (_method == GET)
	{
		if (access(target, F_OK) == -1)
			throw HttpError(NOT_FOUND);
		if (access(target, R_OK) == -1)
			throw HttpError(FORBIDDEN);
		fd = open(target, O_RDONLY);
		if (fd == -1)
			throw HttpError(INTERNAL_SERVER_ERROR);
	}
	if (_method == POST)
	{
		if (access(target, W_OK) == -1 && access(target, F_OK) == 0)
			throw HttpError(FORBIDDEN);
		fd = open(_target.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (fd == -1)
			throw HttpError(INTERNAL_SERVER_ERROR);
	}
	if (_method == DELETE)
	{
		fd = -1;
		struct stat buf;
		if (stat(target, &buf) == -1)
			throw HttpError(NOT_FOUND);
		if ((buf.st_mode & S_IWUSR) == 0 && S_ISREG(buf.st_mode))
			throw HttpError(FORBIDDEN);
		if (S_ISDIR(buf.st_mode))
		{
			DIR* dir = opendir(target);
			struct dirent *file;
			if (!dir)
				throw HttpError(INTERNAL_SERVER_ERROR);
			while ((file = readdir(dir)))
			{
				if (static_cast<std::string>(file->d_name) != "." && static_cast<std::string>(file->d_name) != "..") {
					closedir(dir);
					throw HttpError(CONFLICT);
				}
			}
			closedir(dir);
		}
		if (access(target, W_OK))
			throw HttpError(INTERNAL_SERVER_ERROR);
		else
		{
			del();
			_state = READY;
		}

	}
	_fileFd = fd;
}

void Handler::handle(void)
{
	if (_isCgi)
	{
		_cgi->cgiHandle();
		if (_cgi->getState() == Cgi::READY)
		{
			if (_method == GET)
			{
				_content = _cgi->getContent();
				_target = "cgi.html";
			}
			if (_method == POST)
			{
				_content = _target;
				_statusCode = CREATED;
			}
			_state = READY;
		}
		return;
	}
	switch (_method)
	{
		case POST:
			post();
			break;
		case GET:
			get();
			break;
		default:
			break;
	}
	if (_method != GET)
		_state =  READY;
}

void Handler::get(void)
{
	if (!_isDir)
	{
		char buffer[READ_SIZE + 1];
		for (int i = 0 ; i < READ_SIZE + 2; i++)
			buffer[i] = 0;
		int bytes_read = read(_fileFd, buffer, READ_SIZE);
		if (bytes_read < 0)
			throw HttpError(INTERNAL_SERVER_ERROR);
		_content.append(buffer, bytes_read);
		if (bytes_read < READ_SIZE)
			_state = READY;
	}
}

void Handler::del(void)
{
	int res = std::remove(_target.c_str());
	if (res)
		throw HttpError(INTERNAL_SERVER_ERROR);
	else
		_statusCode = NO_CONTENT;
}

void Handler::post(void)
{
	write(_fileFd, _content.c_str(), _content.size());
	_content = _target;
	_statusCode = CREATED;
}

void Handler::buildTarget(void)
{
	std::string target = _request->getTarget();
	if (_request->getIsLocation() && !(_request->getLocation().getAlias().empty()))
	{
		size_t pos = target.find("/", 1);
		std::string sub;
		if (pos != std::string::npos)
			sub = target.substr(pos, target.size() - pos);
		else
			sub = "";
		_target = _request->getLocation().getAlias() + sub;
	}
	else
		_target = _request->getServer().getRoot() + target;
	if (_method == POST)
		postParsing();
	urlDecoding();
	if (isDir(_target.c_str()) && _target.rfind("/") != _target.size() - 1)
		_target.append("/");
}

void Handler::urlDecoding(void)
{
	size_t pos = 0;
	while ((pos = _target.find("%", pos)) != std::string::npos)
	{
		std::string value = _target.substr(pos + 1, 2);
		long val = std::strtol(value.c_str(), 0, 16);
		if (val > 127)
		{
			pos++;
			continue;
		}
		std::string newVal(1, val);
		_target.replace(pos, 3, newVal);
		newVal.clear();
	}
}

void Handler::postParsing(void)
{
	std::string body = _request->getBody();
	std::string boundary = _request->findHeaderContent("content-type");
	size_t pos = boundary.find("boundary=");
	if (pos == std::string::npos)
		throw HttpError(NOT_IMPLEMENTED);
	boundary = boundary.substr(pos + 9);
	boundary = "--" + boundary;
	std::string newline = _request->getNewline();

	size_t posStart = body.find("filename=\"") + 10;
	size_t posEnd = body.find("\"", posStart);
	if (*(_target.end() - 1) != '/')
		_target.append("/");
	_target.append(body.substr(posStart, posEnd - posStart));

	posStart = body.find(newline + newline) + 2 * newline.size();
	posEnd = body.find(boundary, posStart);
	_content = body.substr(posStart, posEnd - posStart - newline.size());
}

bool Handler::isDir(const char* path) const
{
	struct stat buf;
	if (stat(path, &buf) == 0 && S_ISDIR(buf.st_mode))
	{
		return true;
	}
	return false;
}

void Handler::brew()
{
	std::srand(time(NULL));

	int rnum = std::rand();
	int result = rnum % 3;
	switch (result)
	{
	case 0:
		throw HttpError(SERVICE_UNAVAILABLE);
		break;
	case 1:
		throw HttpError(IM_A_TEAPOT);
		break;
	case 2:
		_target = _request->getServer().getRoot() + "/html/coffee.html";
		_method = GET;
		_fileFd = -1;
		createFd();
		break;
	default:
		break;
	}
}
