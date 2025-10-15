#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <unistd.h>
#include <cstring>
#include <csignal>
#include <poll.h>
#include "Request.hpp"
#include "StatusCode.hpp"

#define READ_SIZE 100000

extern volatile sig_atomic_t g_sigint;

class HttpError : public std::exception
{
	private:
		int _errorCode;
	public:
		HttpError(int code);
		int getCode(void) const;
};

enum  e_method
{
	POST,
	GET,
	DELETE,
	BREW
};

class Cgi;

class Handler
{
	private:
		Request 	*_request;
		int			_state;
		int			_fileFd;
		e_method	_method;
		int			_statusCode;
		std::string	_target;
		std::string _content;
		bool		_isDir;
		bool		_isCgi;
		Cgi*		_cgi;

		std::vector<pollfd>	*_fds;

		void		setMethod();
		bool		hasExtension();
		void		createFd(void);
		void		createErrorFd(void);
		void		buildTarget(void);
		void		postParsing(void);
		void		urlDecoding(void);
		bool		isDir(const char* path) const;
		void		post(void);
		void		autoIndex(void);
		void		get(void);
		void		del(void);
		void		brew(void);
		void		cgi(void);
	public:
		Handler();
		Handler(Request* request, std::vector<pollfd> &fds);
		Handler(Request* request, int code);
		~Handler();
		enum e_status
		{
			INIT,
			IN_PROGRESS,
			READY,
			SENT
		};
		int			getState(void) const;
		int			getFileFd(void) const;
		e_method	getMethod(void) const;
		std::string getTarget(void) const;
		std::string getContent(void) const;
		int			getStatusCode(void) const;
		Request*	getRequest(void) const;
		bool		getIsDir(void) const;
		Cgi*		getCgi(void) const;
		void		handle(void);
		void		setFilefd(int fd);
		bool		getIsCgi(void) const;
		void		setState(e_status state);
};

#endif
