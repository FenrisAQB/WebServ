#include <sys/types.h>
#include <signal.h>
#include "../../includes/Client.hpp"
#include "../../includes/Cgi.hpp"

Client::Client(std::string &connectionIp, int clientFd, std::vector<pollfd>	&fds): _connectionIp(connectionIp), _timeout(time(NULL) + CLIENT_TIMEOUT), _hasTimedOut(false), _clientFd(clientFd), _fileFd(-1), _fds(fds) {
	_request = NULL;
	_request = new Request(connectionIp);
	_handler = NULL;
	_response = NULL;
}

Client::~Client() {
	if (_request)
		delete _request;
	_deleteHandler();
	if (_response)
		delete _response;
}

void	Client::_deleteHandler() {
	if (_handler) {
		_handler->setState(Handler::READY);
		delete _handler;
	}
}

/*
** ---
*/

int	Client::getClientFd() const { return _clientFd; }

int	Client::getFileFd() const { return _fileFd; }

void	Client::resetClientFd() { _clientFd = -1; }

void	Client::resetFileFd() { _fileFd = -1; }

bool	Client::handleTimeout(EventResult &result, int statusCode) {
	time_t	current = time(NULL);
	if (statusCode == SERVICE_UNAVAILABLE || (!_hasTimedOut && current > _timeout)) {
		_hasTimedOut = true;
		Logger::log(Logger::TRACE, "Client terminated on fd: %d. With code: %d", _clientFd, statusCode);
		if (_request->getState() != Request::READY) {
			result.sockType = CLIN;
			_request->setStatus(statusCode);
			_request->setState(Request::READY);
			_handler = new Handler(_request, statusCode);
			if (_handler->getState() == Handler::READY) {
				_response = new Response(*_handler);
				Logger::log(Logger::DEBUG, "Setting POLLOUT on client fd: %d", _clientFd);
				result.newEvents = POLLOUT;
			}
			else {
				_fileFd = _handler->getFileFd();
				Logger::log(Logger::DEBUG, "Setting new file fd to be added: %d", _fileFd);
				result.newFd = _fileFd;
			}
		}
		else if (_request->getState() == Request::READY && _handler->getState() != Handler::READY) {
			result.sockType = FILEFD;
			if (_fileFd != -1) {
				Logger::log(Logger::DEBUG, "Setting old file fd to be removed: %d", _fileFd);
				result.oldFd = _fileFd;
			}
			Cgi* cgi = _handler->getCgi();
			if (cgi)
				kill(cgi->getChildPid(), 0);
			_deleteHandler();
			_handler = new Handler(_request, statusCode);
			if (_handler->getState() == Handler::READY) {
				_response = new Response(*_handler);
				Logger::log(Logger::DEBUG, "Setting POLLOUT on client fd: %d", _clientFd);
				result.newEvents = POLLOUT;
			}
			else {
				_fileFd = _handler->getFileFd();
				Logger::log(Logger::DEBUG, "Setting new file fd to be added: %d", _fileFd);
				result.newFd = _fileFd;
			}
		}
		else if (_request->getState() == Request::READY && _handler->getState() == Handler::READY) {
			Logger::log(Logger::WARNING, "Client timed out while sending response. Closing connection.");
			Logger::log(Logger::DEBUG, "Setting old client fd to be removed: %d", _clientFd);
			result.oldFd = _clientFd;
			Logger::log(Logger::DEBUG, "Setting client as done with fd: %d", _clientFd);
			result.isDone = true;
		}
		return true;
	}
	return false;
}

/*
** ---
*/

void	Client::_setSockType(pollfd &pfd, bool shutdown) {
	if (!shutdown && _handler && _handler->getIsCgi())
		pfd.revents &= POLLIN | POLLOUT;
	if (pfd.revents & (POLLHUP | POLLERR |POLLNVAL))
		_sockType = ERRORFD;
	else if (pfd.fd == _clientFd && (pfd.revents & POLLIN))
		_sockType = CLIN;
	else if (pfd.fd == _clientFd && (pfd.revents & POLLOUT))
		_sockType = CLOUT;
	else if (pfd.fd == _fileFd)
		_sockType = FILEFD;
	else
		_sockType = UNKNOWN;
}

/*
** ---
*/

void	Client::_handleUnusableSocket(EventResult &result) {
	if (_request->getState() == Request::READY && _handler->getIsCgi()) {
		Cgi* cgi = _handler->getCgi();
		if (cgi)
			kill(cgi->getChildPid(), 0);
	}
	Logger::log(Logger::DEBUG, "Setting old client fd to be removed: %d", _clientFd);
	result.oldFd = _clientFd;
	Logger::log(Logger::DEBUG, "Setting client as done with fd: %d", _clientFd);
	result.isDone = true;
}

void	Client::_handleClientIn(std::vector<ServerBloc> &servers, EventResult &result) {
	char	buffer[BUFFER_SIZE];
	std::memset(buffer, 0, BUFFER_SIZE);
	int bytes_read = recv(_clientFd, buffer, BUFFER_SIZE, 0);
	if (!bytes_read) {
		Logger::log(Logger::DEBUG, "Client disconnected on fd: %d", _clientFd);
		Logger::log(Logger::DEBUG, "Setting old client fd to be removed: %d", _clientFd);
		result.oldFd = _clientFd;
		Logger::log(Logger::DEBUG, "Setting client as done with fd: %d", _clientFd);
		result.isDone = true;
		return;
	}
	_request->parse(std::string(buffer, bytes_read), servers);
	if (_request->getState() != Request::READY)
		return;
	try {
		_handler = new Handler(_request, _fds);
	}
	catch (const HttpError& e) {
		_handler = new Handler(_request, e.getCode());
	}
	_fileFd = _handler->getFileFd();
	if (_handler->getState() == Handler::READY) {
		_response = new Response(*_handler);
		Logger::log(Logger::DEBUG, "Setting POLLOUT on client fd: %d", _clientFd);
		result.newEvents = POLLOUT;
	}
	else {
		Logger::log(Logger::DEBUG, "Setting new file fd to be added: %d", _fileFd);
		result.newFd = _fileFd;
	}
}

void	Client::_handleFileFd(EventResult &result) {
	try {
		_handler->handle();
	}
	catch (const HttpError& e) {
		Logger::log(Logger::DEBUG, "Setting old file fd to be removed: %d", _fileFd);
		result.oldFd = _fileFd;
		_deleteHandler();
		_handler = new Handler(_request, e.getCode());
		_fileFd = _handler->getFileFd();
		if (_handler->getState() == Handler::READY) {
			_response = new Response(*_handler);
			Logger::log(Logger::DEBUG, "Setting POLLOUT on client fd: %d", _clientFd);
			result.newEvents = POLLOUT;
		}
		else {
			Logger::log(Logger::DEBUG, "Setting new file fd to be added: %d", _fileFd);
			result.newFd = _fileFd;
		}
		return;
	}
	if (_handler->getState() == Handler::READY) {
		_response = new Response(*_handler);
		Logger::log(Logger::DEBUG, "Setting POLLOUT on client fd: %d", _clientFd);
		result.newEvents = POLLOUT;
		Logger::log(Logger::DEBUG, "Setting old file fd to be removed: %d", _fileFd);
		result.oldFd = _fileFd;
	}
}

void	Client::_handleClientOut(EventResult &result) {
	std::string	response = _response->buildResponse();
	size_t bytes_sent = send(_clientFd, response.data(), response.size(), 0);
	if (bytes_sent < response.size())
		Logger::log(Logger::ERROR, "Partial send on fd: %d. Sent %zu/%zu bytes", _clientFd, bytes_sent, response.size());
	if (_response->getState() == Response::CHUNK)
			_response->markChunkSent(bytes_sent);
	if (_response->getState() == Response::FINISH) {
		Logger::log(Logger::DEBUG, "Setting old client fd to be removed: %d", _clientFd);
		result.oldFd = _clientFd;
		Logger::log(Logger::DEBUG, "Setting client as done with fd: %d", _clientFd);
		result.isDone = true;
		Logger::log(Logger::TRACE, "Transaction complete with code: %d %s", _response->getStatusCode(), _response->getStatusMsg().c_str());
	}
}

/*
** ---
*/

EventResult	Client::handleEvent(pollfd &pfd, std::vector<ServerBloc> &servers, bool shutdown) {
	EventResult	result;
	_setSockType(pfd, shutdown);
	result.sockType = _sockType;
	if (_sockType == ERRORFD) {
		Logger::log(Logger::ERROR, "Handling event on unusable socket: %d, Events: %d, REvents: %d", pfd.fd, pfd.events, pfd.revents);
		_handleUnusableSocket(result);
		return result;
	}
	Logger::log(Logger::DEBUG, "Handling event on _socketType: %d. Socket: %d. Events: %d. REvents: %d", _sockType, pfd.fd, pfd.events, pfd.revents);
	if (_sockType == CLIN)
		_handleClientIn(servers, result);
	else if (_sockType == FILEFD && _request->getState() == Request::READY)
		_handleFileFd(result);
	else if (_sockType == CLOUT && _handler->getState() == Handler::READY)
		_handleClientOut(result);
	else if (_sockType == UNKNOWN)
		Logger::log(Logger::WARNING, "Unrecognized socket: %d. Events: %d. REvents: %d", pfd.fd, pfd.events, pfd.revents);
	return result;
}
