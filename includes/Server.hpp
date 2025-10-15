#ifndef SERVER_HPP
# define SERVER_HPP

# include <vector>
# include <map>
# include <poll.h>
# include <fcntl.h>
# include <csignal>
# include "Logger.hpp"
# include "Client.hpp"

extern volatile sig_atomic_t g_sigint;

class Server {
	private:
		std::vector<ServerBloc> &_servers;
		size_t					_listenAmount;
		std::vector<pollfd>		_pollFds;
		std::map<int, Client*>	_clients;
		bool					_shuttingDown;

		void		_addToPoll(int fd);
		void		_updatePollEvents(EventResult &result, Client *client);
		void		_removeFromPoll(int fd);
		std::string	_logAddr(struct sockaddr_in *clientAddr, int clientFd) const;
		void		_handleConnection(int fd);
		void		_applyEventResult(EventResult &result, Client *client, size_t *i);
		void		_checkClientsTimeouts();
		void		_shutdown();

	public:
		Server(std::map<int, ServerBloc> &socketServer, std::vector<ServerBloc> &servers);
		~Server();
		void	runServer();
};

#endif