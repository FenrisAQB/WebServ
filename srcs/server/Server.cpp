#include "../../includes/Server.hpp"

Server::Server(std::map<int, ServerBloc> &socketServer, std::vector<ServerBloc> &servers): _servers(servers), _listenAmount(socketServer.size()), _shuttingDown(false) {
	for (std::map<int, ServerBloc>::iterator it=socketServer.begin(); it!=socketServer.end(); it++)
		_addToPoll(it->first);
}

Server::~Server() {};

/*
** ---
*/

void	Server::_addToPoll(int fd) {
	if (fd < 0) {
		Logger::log(Logger::ERROR, "Trying to add negative fd to poll: %d", fd);
		return;
	}
	pollfd	temp;
	temp.fd = fd;
	temp.events = POLLIN;
	temp.revents = 0;
	_pollFds.push_back(temp);
	Logger::log(Logger::DEBUG, "Added fd to poll: %d", fd);
}

void	Server::_updatePollEvents(EventResult &result, Client *client) {
	for (size_t i=0; i < _pollFds.size(); i++) {
		pollfd	&pfd = _pollFds[i];
		if (pfd.fd == client->getClientFd()) {
			Logger::log(Logger::DEBUG, "Changing event on fd: %d. To: %d", pfd.fd, result.newEvents);
			pfd.events = result.newEvents;
			pfd.revents = 0;
			break;
		}
	}
}

void	Server::_removeFromPoll(int fd) {
	if (fd < 0) {
		Logger::log(Logger::ERROR, "Trying to remove negative fd from poll: %d", fd);
		return;
	}
	for (std::vector<pollfd>::iterator it=_pollFds.begin(); it!=_pollFds.end(); it++) {
		if (it->fd == fd) {
			_pollFds.erase(it);
			close(fd);
			Logger::log(Logger::DEBUG, "Removing fd from poll: %d", fd);
			break;
		}
	}
}

/*
** ---
*/

std::string	Server::_logAddr(struct sockaddr_in *clientAddr, int clientFd) const {
	struct sockaddr_in serverAddr;
	socklen_t serverAddrLen = sizeof(serverAddr);
	getsockname(clientFd, (sockaddr*)&serverAddr, &serverAddrLen);
	char serverIpStr[INET_ADDRSTRLEN];
	char clientIpStr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &serverAddr.sin_addr, serverIpStr, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(clientAddr->sin_addr), clientIpStr, INET_ADDRSTRLEN);
	int serverPort = ntohs(serverAddr.sin_port);
	int clientPort = ntohs(clientAddr->sin_port);
	Logger::log(Logger::TRACE, "New connection accepted on fd: %d. From: %s:%d. On: %s:%d.", clientFd, clientIpStr, clientPort, serverIpStr, serverPort);
	std::ostringstream	addr;
	addr << serverIpStr << ":" << serverPort;
	return addr.str();
}

void	Server::_handleConnection(int fd) {
	while (true)
	{
		struct sockaddr_in	clientAddr;
		socklen_t			clientAddrLen = sizeof(clientAddr);

		int	clientFd = accept(fd, (sockaddr*)&clientAddr, &clientAddrLen);
		if (clientFd < 0) {
			if (errno == EAGAIN) {
				Logger::log(Logger::DEBUG, "Queue is empty on server socket fd: %d", fd);
				return;
			}
			Logger::log(Logger::ERROR, "Failed accepting new socket from fd: %d", fd);
			return;
		}
		std::string connectionIp = _logAddr(&clientAddr, clientFd);
		int flags = fcntl(clientFd, F_GETFL, 0);
		if (flags == -1)
			flags = 0;
		if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
			Logger::log(Logger::ERROR, "Failed setting flags on socket for fd: %d.", clientFd);
			return;
		}
		Client *temp;
		try { temp = new Client(connectionIp, clientFd, _pollFds); }
		catch (std::exception &e) {
			Logger::log(Logger::ERROR, "Cancelling connection as memory allocation failed for fd: %d", clientFd);
			close(clientFd);
			return;
		}
		_addToPoll(clientFd);
		_clients[clientFd] = temp;
	}
}

void	Server::_applyEventResult(EventResult &result, Client *client, size_t *i = NULL) {
	int	clientFd = client->getClientFd();
	int	fileFd = client->getFileFd();

	if (result.sockType == Client::UNKNOWN && i)
		Logger::log(Logger::WARNING, "Attempting event resolution on unknown socket for fd: %d.", _pollFds[*i].fd, result.oldFd);
	if (result.newFd > 2) {
		_addToPoll(result.newFd);
		_clients[result.newFd] = client;
	}
	if (result.newEvents)
		_updatePollEvents(result, client);
	if (result.oldFd > 2) {
		_removeFromPoll(result.oldFd);
		_clients.erase(result.oldFd);
		if (result.sockType == Client::CLIN || result.sockType == Client::CLOUT || result.sockType == Client::ERRORFD) {
			client->resetClientFd();
			if (fileFd > 2) {
				_removeFromPoll(fileFd);
				_clients.erase(fileFd);
				client->resetFileFd();
			}
		}
		else if ((result.sockType == Client::FILEFD || result.sockType == Client::ERRORFD) && fileFd == result.oldFd)
			client->resetFileFd();
		if (i)
			(*i)--;
	}
	if (result.isDone && result.sockType != Client::FILEFD && result.sockType != Client::UNKNOWN) {
		Logger::log(Logger::DEBUG, "Cleaned up client on fd: %d. With file fd: %d.", clientFd, fileFd);
		delete client;
	}
}

void	Server::_checkClientsTimeouts() {
	for (std::map<int, Client*>::iterator it=_clients.begin(); it!=_clients.end(); it++) {
		Client		*client = it->second;
		EventResult	result;
		try {
			if (_shuttingDown)
				client->handleTimeout(result, SERVICE_UNAVAILABLE);
			else if (!client->handleTimeout(result, REQUEST_TIMEOUT))
				continue;
			_applyEventResult(result, client);
		} catch (std::exception &e) { it--; }
	}
}

void	Server::_shutdown() {
	for (size_t i=0; i<_pollFds.size(); i++)
		close(_pollFds[i].fd);
	Logger::shutdown();
}

/*
** ---
*/

void	Server::runServer() {
	while (true) {
		if (g_sigint && !_shuttingDown) {
			Logger::log(Logger::WARNING, "Received Sigint signal, shutting down gracefully...");
			_shuttingDown = true;
		}
		if (_shuttingDown && _clients.empty())
			break;
		int socketReady = poll(&_pollFds[0], _pollFds.size(), 15);
		if (socketReady == -1) {
			if (errno == EINTR)
				continue;
			Logger::log(Logger::FATAL, "Failed poll call");
		}
		else if (!socketReady) {
			_checkClientsTimeouts();
			continue;
		}
		for (size_t i = 0; i < _pollFds.size() && socketReady > 0; i++) {
			pollfd	&pfd = _pollFds[i];
			if (!pfd.revents)
				continue;
			--socketReady;
			std::map<int, Client*>::iterator it = _clients.find(pfd.fd);
			if (it != _clients.end()) {
				Client	*client = it->second;
				EventResult result;
				try {
					result = client->handleEvent(pfd, _servers, _shuttingDown);
					_applyEventResult(result, client, &i);
				} catch (std::exception &e) {}
			}
			else if (i < _listenAmount && !_shuttingDown)
				_handleConnection(pfd.fd);
		}
		_checkClientsTimeouts();
	}
	_shutdown();
}
