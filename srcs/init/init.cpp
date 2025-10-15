#include "../../includes/Logger.hpp"
#include "../../includes/ServerBloc.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>

std::map<int, ServerBloc> init(std::vector<ServerBloc>& servers)
{
	std::map<int, ServerBloc> socketServer;
	std::pair<int, ServerBloc> match;
	int validSocket = 0;
	if (servers.empty())
		Logger::log(Logger::FATAL, "No valid server");
	for (std::vector<ServerBloc>::iterator server = servers.begin(); server != servers.end(); server++)
	{
		std::map<std::string, std::pair<std::string, std::string> > temp = server->getListens();
		for (std::map<std::string, std::pair<std::string, std::string> >::iterator it = temp.begin(); it != temp.end(); it++)
		{
			std::string host = it->second.first;
			std::string port = it->second.second;
			struct addrinfo hints;
			struct addrinfo *info;
			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = 0;
			int result = getaddrinfo(host.c_str(), port.c_str(), &hints, &info);
			if (result != 0)
				Logger::log(Logger::FATAL, "getaddrinfo failed %s", gai_strerror(result));
			int sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if (sockfd  < 0)
			{
				freeaddrinfo(info);
				Logger::log(Logger::FATAL, "socket creation failed: %s", strerror(errno));
			}
			int flags = fcntl(sockfd, F_GETFL, 0);
			if (flags == -1)
				flags = 0;
			if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
				freeaddrinfo(info);
				close(sockfd);
				Logger::log(Logger::FATAL, "Failed to set O_NONBLOCK on server socket: %s", strerror(errno));
			}
			int optval = 1;
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
			{
				freeaddrinfo(info);
				close(sockfd);
				Logger::log(Logger::FATAL, "setsockopt failed: %s", strerror(errno));
			}
			if (bind(sockfd, info->ai_addr, info->ai_addrlen) < 0)
			{
				freeaddrinfo(info);
				close(sockfd);
				continue;
			}
			else
				validSocket++;
			if (listen(sockfd, 10) < 0)
			{
				freeaddrinfo(info);
				close(sockfd);
				Logger::log(Logger::FATAL, "listen failed: %s", strerror(errno));
			}
			match.first = sockfd;
			match.second = *server;
			socketServer.insert(match);
			Logger::log(Logger::TRACE, "Server initialized on %s:%s", host.c_str(), port.c_str());
			freeaddrinfo(info);
		}
	}
	if (!validSocket)
		Logger::log(Logger::FATAL, "No valid socket were provided");
	return (socketServer);
}
