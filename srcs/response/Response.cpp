#include "../../includes/Response.hpp"
#include "../../includes/Handler.hpp"

/*
** ------------------------ Constructor and Destructor ---------------------------
*/

Response::Response() {}

Response::Response(Handler &handler): _state(INIT), _handler(&handler), _statusCode(handler.getStatusCode()), _body(handler.getContent()), _headers(""), _contentType(_getContentType()), _location(""), _chunk(""), _headersBuilt(false), _headersSent(false), _bytesSent(0), _chunkSize(0) {
	if (getStatusMsg() == "Unknown Status")
		this->_statusCode = INTERNAL_SERVER_ERROR;
	_bodyBuilder();
	_buildHeaders();
}

Response::~Response() {}

/*
** ----------------------------- Boolean helpers ---------------------------------
*/

bool Response::_needsBody() const {
	if (this->_statusCode == OK || this->_statusCode == AUTOINDEX || this->_statusCode == CREATED || this->_statusCode / 100 == 4 || this->_statusCode / 100 == 5)
		return true;
	return false;
}

bool Response::_needsContentHeaders() const {
	if (this->_statusCode == OK || this->_statusCode == CREATED || this->_statusCode == AUTOINDEX || this->_statusCode / 100 == 4 || this->_statusCode / 100 == 5)
		return true;
	return false;
}

bool Response::_needsLocation() const {
	if (this->_statusCode == CREATED || this->_statusCode / 100 == 3)
		return true;
	return false;
}

/*
** ------------------------------ String helpers ---------------------------------
*/

std::string Response::getStatusMsg() const
{
	switch (this->_statusCode) {
		case OK: return MSG_200;
		case CREATED: return MSG_201;
		case NO_CONTENT: return MSG_204;
		case AUTOINDEX: return MSG_242;

		case MOVED_PERMANENTLY: return MSG_301;
		case FOUND: return MSG_302;
		case TEMPORARY_REDIRECT: return MSG_307;
		case PERMANENT_REDIRECT: return MSG_308;

		case BAD_REQUEST: return MSG_400;
		case FORBIDDEN: return MSG_403;
		case NOT_FOUND: return MSG_404;
		case METHOD_NOT_ALLOWED: return MSG_405;
		case REQUEST_TIMEOUT: return MSG_408;
		case CONFLICT: return MSG_409;
		case LENGTH_REQUIRED: return MSG_411;
		case CONTENT_TOO_LARGE: return MSG_413;
		case IM_A_TEAPOT: return MSG_418;

		case INTERNAL_SERVER_ERROR: return MSG_500;
		case NOT_IMPLEMENTED: return MSG_501;
		case SERVICE_UNAVAILABLE: return MSG_503;
		case HTTP_VERSION_NOT_SUPPORTED: return MSG_505;
		case LOOP_DETECTED: return MSG_508;

		default: return "Unknown Status";
	}
}

std::string Response::_getContentType() const {
	static std::map<std::string, std::string>	mimeTypes;

	if (mimeTypes.empty()) {
		mimeTypes[".html"] = "text/html";
		mimeTypes[".htm"] = "text/html";
		mimeTypes[".css"] = "text/css";
		mimeTypes[".js"] = "application/javascript";
		mimeTypes[".jpg"] = "image/jpeg";
		mimeTypes[".jpeg"] = "image/jpeg";
		mimeTypes[".png"] = "image/png";
		mimeTypes[".svg"] = "image/svg+xml";
		mimeTypes[".json"] = "application/json";
		mimeTypes[".txt"] = "text/plain";
		mimeTypes[".pdf"] = "application/pdf";
		mimeTypes[".zip"] = "application/zip";
		mimeTypes[".tar"] = "application/x-tar";
		mimeTypes[".gz"] = "application/gzip";
		mimeTypes[".mp3"] = "audio/mpeg";
		mimeTypes[".mp4"] = "video/mp4";
		mimeTypes[".avi"] = "video/x-msvideo";
		mimeTypes[".mpeg"] = "video/mpeg";
		mimeTypes[".webm"] = "video/webm";
		mimeTypes[".ogg"] = "video/ogg";
		mimeTypes[".ico"] = "image/x-icon";
		mimeTypes[".webp"] = "image/webp";
		mimeTypes[".woff"] = "font/woff";
		mimeTypes[".woff2"] = "font/woff2";
		mimeTypes[".ttf"] = "font/ttf";
		mimeTypes[".otf"] = "font/otf";
		mimeTypes[".eot"] = "application/vnd.ms-fontobject";
		mimeTypes[".csv"] = "text/csv";
		mimeTypes[".xml"] = "application/xml";
		mimeTypes[".xhtml"] = "application/xhtml+xml";
		mimeTypes[".webmanifest"] = "application/manifest+json";
		mimeTypes[".mkv"] = "video/x-matroska";
	}

	std::string	file = this->_handler->getTarget();
	size_t		ext = file.rfind('.');

	if (ext != std::string::npos) {
		std::string extension = file.substr(ext);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		if (mimeTypes.find(extension) != mimeTypes.end())
			return mimeTypes[extension];
	}
	return "application/octet-stream";
}

std::string Response::_getCurrentDateTime() const {
	time_t				now = time(0);
	struct tm			*timeData = gmtime(&now);
	std::stringstream	ss;
	const char			*days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	const char			*months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	ss << days[timeData->tm_wday] << ", "
		<< std::setfill('0')
		<< std::setw(2) << timeData->tm_mday << " "
		<< months[timeData->tm_mon] << " "
		<< (timeData->tm_year + 1900) << " "
		<< std::setw(2) << timeData->tm_hour << ":"
		<< std::setw(2) << timeData->tm_min << ":"
		<< std::setw(2) << timeData->tm_sec << " GMT";
	return ss.str();
}

std::string Response::_encodeDblQuotes(const std::string relPath) const {
	std::string	escaped;
	for (size_t i = 0; i < relPath.size(); ++i)
		if (relPath[i] == '"')
			escaped += "%22";
		else
			escaped += relPath[i];
	return escaped;
}

/*
** ------------------------------ Body builders ----------------------------------
*/

std::string Response::_htmlBodyBuilder() {
	std::stringstream	ss;
	std::string			message;


	this->_contentType = "text/html";
	if (this->_statusCode == AUTOINDEX) {
		ss << _listDirectory();
		if (this->_statusCode == OK)
			return ss.str();
	}
	message = getStatusMsg();
	if (message == "Unknown Status") {
		this->_statusCode = INTERNAL_SERVER_ERROR;
		message = getStatusMsg();
	}
	ss << "<!DOCTYPE html>\n"
		<< "<html>\n"
		<< "<head><title>" << this->_statusCode << " " << message << "</title></head>\n"
		<< "<body>\n"
		<< "<h1>" << this->_statusCode << " " << message << "</h1>\n"
		<< "<p>";
	switch (this->_statusCode) {
		case CONFLICT:
			ss << "Directory not empty";
			break;
		case HTTP_VERSION_NOT_SUPPORTED:
			ss << "This server supports HTTP/1.1 only.";
			break;
		case IM_A_TEAPOT:
			ss << "I'm a teapot, not a coffee machine!";
			break;
		case NOT_FOUND:
			ss << "The requested resource could not be found.";
			break;
		case FORBIDDEN:
			ss << "You don't have permission to access this resource.";
			break;
		default:
			ss << "It's all in the title!";
		break;
	}
	ss << "</p>\n"
		<< "<p><a href=\"/\">Back to Home</a></p>\n"
		<< "</body>\n"
		<< "</html>";
	return ss.str();
}

std::string Response::_jasonBodyBuilder() {
	std::stringstream	ss;
	std::string			fileName;

	this->_contentType = "application/json";
	fileName = this->_location.substr(this->_location.rfind('/') + 1);
	ss << "{\n"
		<< "\"status\": \"created\",\n"
		<< "\"message\": \"File uploaded successfully\",\n"
		<< "\"filename\": \"" << fileName << "\",\n"
		<< "\"url\": \"" << this->_location << "\"\n"
		<< "}\n";
	return ss.str();
}

std::string Response::_listDirectory() {
	std::string			path = this->_handler->getTarget();
	std::stringstream	ss;
	DIR					*dir = opendir(path.c_str());

	if (!dir) {
		if (access(path.c_str(), R_OK))
			this->_statusCode = FORBIDDEN;
		else
			this->_statusCode = NOT_FOUND;
		return "";
	}
	this->_statusCode = OK;
	ss << "<!DOCTYPE html>\n"
	<< "<html>\n"
	<< "<head><title>Index of " << path << "</title></head>\n"
	<< "<body>\n"
	<< "<h1>Index of " << path << "</h1>\n"
	<< "<ul>\n";

	std::vector<std::string> files;
	struct dirent *file;
	while ((file = readdir(dir))) {
		std::string fileName = file->d_name;
		if (fileName != "." && fileName != "..")
			files.push_back(fileName);
	}
	closedir(dir);
	std::sort(files.begin(), files.end());
	for (size_t i = 0; i < files.size(); ++i) {
		std::string	fullPath = path;
		std::string	relPath = path;
		std::string	root = this->_handler->getRequest()->getServer().getRoot();
		struct		stat fileStat;

		if (!relPath.find(root))
			relPath = relPath.substr(root.length());
		if (!relPath.empty() && *relPath.begin() != '/')
			relPath = "/" + relPath;
		if (*(--relPath.end()) != '/')
			relPath += "/";
		relPath += files[i];
		relPath = _encodeDblQuotes(relPath);
		ss << "<li><a href=\"" << relPath;
		if (fullPath[fullPath.size() - 1] != '/')
			fullPath += "/";
		fullPath += files[i];
		bool isDir = stat(fullPath.c_str(), &fileStat) == 0 && S_ISDIR(fileStat.st_mode);
		if (isDir)
			ss << "/";
		ss << "\">" << files[i];
		if (isDir)
			ss << "/";
		ss << "</a></li>\n";
	}
	ss << "</ul>\n"
		<< "</p>\n"
		<< "<p><a href=\"/\">Back to Home</a></p>\n"
		<< "</body>\n"
		<< "</html>";
	return ss.str();
}

void Response::_bodyBuilder() {
	if (!this->_handler)
		return ;
	if (_needsLocation())
		this->_location = this->_body;
	if (this->_statusCode == CREATED) {
		this->_contentType = "application/json";
		this->_body = _jasonBodyBuilder();
	}
	else if (this->_body.size() == 0 && _needsBody() && this->_state != CHUNK) {
		this->_contentType = "text/html";
		this->_body = _htmlBodyBuilder();
	}
	if (this->_body.size() > READ_SIZE)
		this->_state = CHUNK;
	else
		this->_state = WORKING;
}

/*
** ----------------------------- Headers builder ---------------------------------
*/

void Response::_buildHeaders() {
	std::stringstream	ss;
	std::string			statusMSG = getStatusMsg();
	std::string			location;

	ss << "HTTP/1.1 " << this->_statusCode << " " << statusMSG << "\r\n";
	ss << "Date: " << _getCurrentDateTime() << "\r\n";
	if (_needsContentHeaders()) {
		ss << "Content-Type: " << this->_contentType << "; charset=utf-8" << "\r\n";
		if (this->_state != CHUNK)
			ss << "Content-Length: " << this->_body.size() << "\r\n";
	}
	if (_needsLocation())
		ss << "Location: " << this->_location << "\r\n";
	if (this->_state == CHUNK)
		ss << "Transfer-Encoding: chunked\r\n";
	ss << "Cache-Control: no-store\r\n";
	ss << "Connection: close\r\n";
	ss << "\r\n";
	this->_headers = ss.str();
	this->_headersBuilt = true;
}

/*
** ------------------------------ Chunk handling ---------------------------------
*/

std::string Response::_handleChunk() {
	if (!this->_headersSent) {
		this->_headersSent = true;
		return this->_headers;
	}
	if (!this->_chunk.empty() && this->_bytesSent > 0) {
		if (this->_bytesSent >= this->_chunk.size()) {
			this->_chunk.clear();
			this->_bytesSent = 0;
		} else {
			std::string	remaining(this->_chunk.begin() + this->_bytesSent, this->_chunk.end());
			return remaining;
		}
	}
	if (this->_body.empty()) {
		this->_state = FINISH;
		if (Logger::getLogLevel() >= Logger::DEBUG)
			printResponse();
		return "0\r\n\r\n";
	}

	if (this->_chunk.empty()) {
		size_t				chunkLen = std::min(static_cast<size_t>(READ_SIZE), this->_body.size());
		this->_chunkSize = chunkLen;
		std::stringstream	ss;
		ss << std::hex << chunkLen << "\r\n";
		this->_chunk = ss.str();
		this->_chunk.append(this->_body.data(), chunkLen);
		this->_chunk.append("\r\n");
		this->_bytesSent = 0;
	}
	return this->_chunk;
}

void Response::markChunkSent(size_t bytes) {
	if (bytes >= this->_chunk.size()) {
		if (this->_chunkSize > 0 && this->_chunkSize <= this->_body.size())
			this->_body.erase(0, this->_chunkSize);
		this->_chunk.clear();
		this->_bytesSent = 0;
		this->_chunkSize = 0;
	} else if (bytes > 0) {
		this->_bytesSent += bytes;
	}
	if (this->_bytesSent < this->_chunk.size())
		this->_state = CHUNK;
}

/*
** --------------------------------- Getters -------------------------------------
*/

Handler *Response::getHandler() { return this->_handler; }

int Response::getState() const { return this->_state; }

int Response::getStatusCode() const { return this->_statusCode; }

/*
** ----------------------------- Response builder --------------------------------
*/

std::string Response::buildResponse() {
	if (this->_state == WORKING) {
		this->_state = FINISH;
		this->_headersSent = true;
		if (Logger::getLogLevel() >= Logger::DEBUG)
			printResponse();
		return this->_headers + this->_body;
	}
	else if (this->_state == CHUNK)
		return _handleChunk();
	return "";
}

/*
** --------------------------------- Logs -------------------------------------
*/

void	Response::printResponse() const {
	const char*	topSection = "╔═════════";
	const char*	wall = "║";
	const char*	botSection = "╚════════════════════════";
	std::ostream	*out = &std::cout;

	if (Logger::getLogLevel() >= Logger::DEBUG &&  Logger::getFilteredOutput())
		out = Logger::getDebugFile();
	(*out) << Logger::getColorStr(Logger::INFO) << topSection << " Response" << std::endl;
	(*out) << wall << std::left << std::setw(20) << " Status" << ": " << this->_statusCode << std::endl;
	(*out) << wall << std::left << std::setw(20) << " State" << ": " << this->_state << std::endl;
	(*out) << wall << " "  << topSection << " Headers" << std::endl;
	if (this->_headers.empty())
		(*out) << wall << " " << wall << " (No headers)" << std::endl;
	else
		(*out) << wall << " " << wall << std::left << std::setw(20) << " Headers" << ": " << this->_headers << std::endl;
	(*out) << wall << " "  << botSection << std::endl;
	(*out) << wall << " "  << topSection << " Body" << std::endl;
	if (this->_body.empty())
		(*out) << wall << " "  << wall << " (No body)" << std::endl;
	else
		(*out) << wall << " " << wall << std::left << std::setw(20) << " Body" << ": " << this->_body << std::endl;
	(*out) << wall << " "  << topSection << " Chunk" << std::endl;
		if (this->_chunk.empty())
		(*out) << wall << " "  << wall << " (No chunk)" << std::endl;
	else
		(*out) << wall << " " << wall << std::left << std::setw(20) << " Chunk" << ": " << this->_chunk << std::endl;
	(*out) << wall << " " << botSection << std::endl;
	(*out) << botSection << Logger::getColorStr(Logger::NONE) << std::endl;
}
