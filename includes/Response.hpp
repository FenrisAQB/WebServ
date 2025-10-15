#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <dirent.h>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include "StatusCode.hpp"

class Handler;

class Response {
	private:
		int			_state;
		Handler		*_handler;
		int			_statusCode;
		std::string	_body;
		std::string	_headers;
		std::string	_contentType;
		std::string	_location;
		std::string	_chunk;
		bool		_headersBuilt;
		bool		_headersSent;
		size_t		_bytesSent;
		size_t		_chunkSize;

		bool		_needsBody() const;
		bool		_needsContentHeaders() const;
		bool		_needsLocation() const;

		
		std::string	_getContentType() const;
		std::string	_getCurrentDateTime() const;
		std::string	_encodeDblQuotes(const std::string relPath) const;

		std::string	_htmlBodyBuilder();
		std::string	_jasonBodyBuilder();
		std::string	_listDirectory();
		void		_bodyBuilder();

		void		_buildHeaders();
		std::string	_handleChunk();

	public:
		enum e_response_state {
			INIT,
			WORKING,
			CHUNK,
			FINISH
		};

		Response();
		Response(Handler &handler);
		~Response();

		std::string	getStatusMsg() const;
		void		markChunkSent(size_t bytes);
		Handler		*getHandler();
		int			getState() const;
		int			getStatusCode() const;
		std::string	buildResponse();
		void		printResponse() const;
};

#endif
