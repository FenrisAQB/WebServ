#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <map>
# include <string>
# include <vector>
# include <sys/socket.h>
# include <iomanip>
# include "ServerBloc.hpp"
# include "Logger.hpp"
# include "utils.hpp"
# include "StatusCode.hpp"
# include <netdb.h>
# include <arpa/inet.h>

# define REQUEST_DEFAULT_STATUS 200

class	Request
{
	private:
		int									_state;
		std::string							_connectionIp;

		std::string							_newline;
		std::string							_buffer;

		std::string							_debugMSG;

		int									_status;
		std::string							_method;
		std::string							_target;
		std::string							_query;
		std::string							_host;

		bool								_isChunked;
		long long							_contentLength;

		std::map<std::string, std::string>	_headers;

		std::string							_body;

		ServerBloc							_server;
		bool								_isLocation;
		LocationBloc						_location;

		void	setFirstErrorStatus(int status, std::string debugMSG);

		void	findDefaultServer(const std::vector<ServerBloc> &servers);
		bool	findNewlineFormat();
		void	extractQuery();
		void	storeRequestLine(const size_t &nlPos);
		bool	invalidCharsInHeader(const std::string &key) const;
		void	findServerAndLocation(const std::vector<ServerBloc> &servers);
		void	storeHostHeader(std::string &value, const std::vector<ServerBloc> &servers);
		void	storeParsingUsedHeader(const std::string &key, std::string &value);
		void	storeHeader(const size_t &nlPos, const std::vector<ServerBloc> &servers);
		void	parseChunked();
		void	storeBody();

		void	requestValid();

	public:
		enum	e_state {
			INIT,
			REQUEST_LINE,
			HEADERS,
			BODY,
			VERIFICATION,
			READY,
			SENT
		};

		Request();
		Request(std::string &connectionIp);
		~Request();

		int									getState() const;
		int									getStatus() const;
		std::string							getMethod() const;
		std::string							getTarget() const;
		std::string							getQuery() const;
		std::string							getHost() const;
		std::map<std::string, std::string>	getHeaders() const;
		std::string							findHeaderContent(const std::string &header) const;
		std::string							getBody() const;
		ServerBloc							getServer() const;
		bool								getIsLocation() const;
		LocationBloc						getLocation() const;
		std::string							getNewline() const;

		void								setState(e_state state);
		void								setStatus(int status);

		void	parse(const std::string &buffer, const std::vector<ServerBloc> &servers);

		void	printRequest() const;
};

#endif