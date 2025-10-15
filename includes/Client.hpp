#ifndef CLIENT_HPP
# define CLIENT_HPP

#define BUFFER_SIZE 5000

# include <poll.h>
# include "Logger.hpp"
# include "StatusCode.hpp"
# include "Request.hpp"
# include "Handler.hpp"
# include "Response.hpp"

# define CLIENT_TIMEOUT 45

struct EventResult {
	int		sockType;
	int		newFd;
	int		oldFd;
	short	newEvents;
	bool	isDone;

	EventResult(): sockType(3), newFd(-1), oldFd(-1), newEvents(0), isDone(false) {}
};

class Client {
	private:
		std::string	_connectionIp;
		time_t		_timeout;
		bool		_hasTimedOut;
		int			_clientFd;
		int			_fileFd;
		int			_sockType;
		Request		*_request;
		Handler		*_handler;
		Response	*_response;

		std::vector<pollfd>	&_fds;

		void	_deleteHandler();

		void		_setSockType(pollfd &pfd, bool shutdown);
		void		_handleUnusableSocket(EventResult &result);
		void		_handleClientIn(std::vector<ServerBloc> &servers, EventResult &result);
		void		_handleFileFd(EventResult &result);
		void		_handleClientOut(EventResult &result);

	public:
		Client();
		Client(std::string &connectionIp, int clientFd, std::vector<pollfd>	&fds);
		~Client();

		enum	e_sockType { CLIN, CLOUT, FILEFD, ERRORFD, UNKNOWN };

		bool		handleTimeout(EventResult &result, int statusCode);
		EventResult	handleEvent(pollfd &pfd, std::vector<ServerBloc> &servers, bool shutdown);

		int		getClientFd() const;
		int		getFileFd() const;

		void	resetClientFd();
		void	resetFileFd();
};

#endif
