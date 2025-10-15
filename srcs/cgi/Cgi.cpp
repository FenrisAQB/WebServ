#include "../../includes/Cgi.hpp"
#include "../../includes/Handler.hpp"
#include <sys/wait.h>

Cgi::~Cgi()
{

}

Cgi::Cgi(Handler &handler,  std::vector<pollfd> &fds): _handler(handler), _state(INIT), _childExited(false), _fds(fds)
{
	findScriptType();
	createEnv();
	if (execute() == 1)
		throw HttpError(INTERNAL_SERVER_ERROR);
}


void Cgi::findScriptType()
{
	std::string target = _handler.getTarget();
	const std::map<std::string, std::string>& cgis = _handler.getRequest()->getLocation().getCgis();
	for (std::map<std::string, std::string>::const_iterator it = cgis.begin(); it != cgis.end(); it++)
	{
		if (target.find(it->first) != std::string::npos)
		{
			_srciptType = std::string(it->first);
			_interpreter = std::string(it->second);
			return;
		}
	}
	throw HttpError(FORBIDDEN);
}

std::string getSpecificQuery(std::string query, std::string toFind)
{
	size_t pos = query.find(toFind);
	if (pos == std::string::npos)
		return std::string();
	size_t endPos = query.find("&", pos);
	if (endPos == std::string::npos)
		endPos = query.size();
	return query.substr(pos + toFind.size(), endPos - (pos + toFind.size()));
}

void Cgi::checkAllowedMethod(const std::string &method) const
{
	const std::vector<std::string> &allowedMethods = _handler.getRequest()->getLocation().getAllowedMethods();
	for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it) {
		if (*it == method)
			return;
	}
	throw HttpError(METHOD_NOT_ALLOWED);
}

void Cgi::createEnv()
{
	Request *request = _handler.getRequest();
	std::string target = _handler.getTarget();
	size_t pos = target.find(_srciptType);

	checkAllowedMethod(request->getMethod());
	_env["PATH"] = "/usr/bin";
	_env["SERVER_NAME"] = request->getHost();
	_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	_env["REQUEST_METHOD"] = request->getMethod();
	_env["PATH_TRANSLATED"] = target.substr(0, pos + _srciptType.size()).replace(0, 2, "");
	size_t startPos = target.find("/cgi-bin");
	size_t endPos = target.find(_srciptType) +_srciptType.size();
	_env["SCRIPT_NAME"] = request->getTarget().substr(startPos, endPos - startPos);
	_env["CONTENT_TYPE"] = request->findHeaderContent("content-type");
	_env["CONTENT_LENGTH"] = request->findHeaderContent("content-length");
	_env["HTTP_ACCEPT"] = request->findHeaderContent("accept");
	_env["HTTP_ACCEPT_LANGUAGE"] = request->findHeaderContent("accept-language");
	_env["HTTP_USER_AGENT"] = request->findHeaderContent("user-agent");
	_env["PATH_INFO"] = target.substr(pos + _srciptType.size());
	if (_env["REQUEST_METHOD"] == "POST")
	{
		_env["BODY"] = _handler.getContent();
		std::string dir = request->getLocation().getUploadPath();
		if (dir.empty())
			throw HttpError(INTERNAL_SERVER_ERROR);
		if (access(dir.c_str(), W_OK))
			throw HttpError(FORBIDDEN);
		_env["UPLOAD_DIR"] = dir;
	}
	if (_env["REQUEST_METHOD"] == "GET")
	{
		std::string query = request->getQuery();
		query = getSpecificQuery(query, "chapter=");
		if (!query.empty())
			_env["QUERY_STRING"] = query;
		if (access(_env["PATH_TRANSLATED"].c_str(), F_OK))
			throw HttpError(NOT_FOUND);
		if (access(_env["PATH_TRANSLATED"].c_str(), X_OK))
			throw HttpError(FORBIDDEN);
	}
}

char** Cgi::getEnv()
{
	char** env = new char*[_env.size() + 1];
	int i = 0;

	for (std::map<std::string, std::string>::iterator it = _env.begin(); it != _env.end(); it++)
	{
		std::string temp = it->first + "=" + it->second;
		env[i] = new char[temp.size() + 1];
		std::strcpy(env[i], temp.c_str());
		i++;
	}
	env[i] = 0;
	return env;
}

int Cgi::getState() const {return _state; };

std::string Cgi::getContent() const {return _cgiContent; }

int Cgi::getChildPid() const {return _childPid ;}

void Cgi::cgiError()
{
	close(_socketPair[0]);
	throw HttpError(INTERNAL_SERVER_ERROR);
}


int Cgi::execute()
{
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, _socketPair) < 0)
		throw HttpError(INTERNAL_SERVER_ERROR);
	int parentSocket = _socketPair[1];
	int childSocket = _socketPair[0];
	_childPid = fork();
	if (_childPid == -1)
		cgiError();
	if (_childPid == 0)
	{
		_handler.setFilefd(childSocket);
		if (dup2(childSocket, STDOUT_FILENO) == -1)
			std::exit(1);
		close(parentSocket);
		close(childSocket);
		for (size_t i=0; i<_fds.size(); i++)
			close(_fds[i].fd);
		Logger::shutdown();
		char *args[4];
		args[0] = const_cast<char*>(_interpreter.c_str());
		args[1] = const_cast<char*>(_env["PATH_TRANSLATED"].c_str());
		args[2] = 0;
		char** envp = getEnv();
		if (execve(_interpreter.c_str(), args, envp) == -1)
		{
			std::cerr << "execve failed" << std::endl;
			for (size_t j = 0; envp[j]; ++j)
				delete envp[j];
			delete[] envp;
			std::exit(1);
		}
	}
	else
	{
		close(childSocket);
		_handler.setFilefd(parentSocket);
	}
	return 0;
}

void Cgi::checkContent()
{
	if (_handler.getMethod() == GET)
	{
		if (_cgiContent.find("Error opening file:") != std::string::npos)
			throw HttpError(INTERNAL_SERVER_ERROR);
		if (_cgiContent.find("Chapter not found") != std::string::npos || _cgiContent.find("Error finding chapter:") != std::string::npos)
			throw HttpError(NOT_FOUND);
	}
	if (_handler.getMethod() == POST)
	{
		if (_cgiContent.find("POST succeed") == std::string::npos)
			throw HttpError(INTERNAL_SERVER_ERROR);
	}
	_state = READY;
}

void Cgi::cgiHandle()
{
	if (!_childExited)
	{
		int status;
		int waitReturn = waitpid(_childPid, &status, WNOHANG);
		if (waitReturn == -1)
			throw HttpError(INTERNAL_SERVER_ERROR);
		if (waitReturn == 0)
			return;
		if (!WIFEXITED(status) || WEXITSTATUS(status))
			throw HttpError(INTERNAL_SERVER_ERROR);
		_childExited = true;
	}
	char buf[READ_SIZE];
	int bytesRead = read(_socketPair[1], buf, READ_SIZE);
	if (bytesRead < 0)
		throw HttpError(INTERNAL_SERVER_ERROR);
	_cgiContent.append(buf, bytesRead);
	if (bytesRead < READ_SIZE) {
		checkContent();
	}
}
