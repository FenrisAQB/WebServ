#ifndef CGI_HPP
#define CGI_HPP
#include <map>
#include <string>
#include <cstdlib>
#include <poll.h>
#include <vector>

class Handler;

class Cgi
{
	private:
		Handler								&_handler;
		std::map<std::string, std::string>	_env;
		int									_state;
		int 								_socketPair[2];
		std::string							_srciptType;
		std::string							_interpreter;
		int									_childPid;
		std::string							_cgiContent;
		bool								_childExited;
		std::vector<pollfd>					&_fds;
		void								checkAllowedMethod(const std::string &method) const;
		void 								createEnv(void);
		void								cgiError(void);
		void								findScriptType(void);
		void								checkContent(void);

	public:
		Cgi();
		Cgi(Handler &handler, std::vector<pollfd> &fds);
		~Cgi();
		enum e_cgi_status
		{
			INIT,
			READY
		};
		int			execute(void);
		char**		getEnv(void);
		void		cgiHandle(void);
		int			getState(void) const;
		std::string getContent(void) const;
		int			getChildPid(void) const;
};

#endif
