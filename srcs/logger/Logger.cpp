#include "../../includes/Logger.hpp"

struct termios	Logger::_originalTermios;

Logger::logLevel			Logger::_logLevel = Logger::TRACE;
std::set<Logger::logLevel>	Logger::_silencedLevels;

bool			Logger::_filteredOutput = true;
std::ofstream	Logger::_debugFile;

std::map<Logger::logLevel, std::string>	Logger::_levelStr = Logger::_genLevelStr();
std::map<Logger::logLevel, std::string>	Logger::_colorStr = Logger::_genColorStr();

std::map<Logger::logLevel, std::string>	Logger::_genLevelStr() {
	std::map<Logger::logLevel, std::string>	levelStr;

	levelStr[Logger::NONE] = "[NONE]\t";
	levelStr[Logger::FATAL] = "[FATAL]\t";
	levelStr[Logger::ERROR] = "[ERROR]\t";
	levelStr[Logger::WARNING] = "[WARNING] ";
	levelStr[Logger::INFO] = "[INFO]\t";
	levelStr[Logger::TRACE] = "[TRACE]\t";
	levelStr[Logger::DEBUG] = "[DEBUG]\t";

	return levelStr;
}

std::map<Logger::logLevel, std::string>	Logger::_genColorStr() {
	std::map<Logger::logLevel, std::string>	colorStr;

	colorStr[Logger::NONE] = "\033[0m";
	colorStr[Logger::FATAL] = "\033[31m";
	colorStr[Logger::ERROR] = "\033[31m";
	colorStr[Logger::WARNING] = "\033[33m";
	colorStr[Logger::INFO] = "\033[32m";
	colorStr[Logger::TRACE] = "\033[35m";
	colorStr[Logger::DEBUG] = "\033[36m";

	return colorStr;
}

/*
 *	---
 */

void	Logger::_enableTermiosRawMode() {
	tcgetattr(0, &_originalTermios);
	struct termios	raw = _originalTermios;
	raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(0, TCSAFLUSH, &raw);
}

void	Logger::_disableTermiosRawMode() {
	tcsetattr(0, TCSANOW, &_originalTermios);
}

void	Logger::_clearTermios() {
	std::cout << "\x1b[3J\x1b[2J\x1b[H" << std::flush;
}

std::string Logger::_toggleFilteredOutputOption(bool state) {
	return std::string(_colorStr[TRACE]) + ("Filtered Output: ") + (state ? "ON" : "OFF") + (_colorStr[NONE]);
}

int Logger::_readKey() {
	char	buffer[6] = {0};
	int		bytesRead = read(0, buffer, sizeof(buffer));

	if (bytesRead <= 0)
		return -1;
	if (bytesRead == 1) {
		char c = buffer[0];
		if (c == '\n' || c == '\r')
			return '\n';
		if (c == 'f' || c == 'F')
			return 'f';
		if (c == 'q' || c == 'Q')
			return 'q';
		if (c == 'm' || c == 'M')
			return 'm';
		return -1;
	}
	if (buffer[0] == '\x1b' && buffer[1] == '[') {
		char c = buffer[2];
		if (c == 'A' || c == 'B')
			return c;
		return -1;
	}
	return -1;
}

void	Logger::_printOptions(int &selected, std::set<logLevel> &mutedLevels, std::vector<std::string> &options) {
	for (int i = 0; i < static_cast<int>(options.size()); ++i) {
		if (i == selected)
			std::cout << "\x1b[7m" << _colorStr[TRACE] << "►  ";
		else
			std::cout << _colorStr[TRACE] << "  ";
		if (i == 0)
			std::cout << options[i] << std::endl << std::endl;
		else {
			std::string levelName = options[i];
			std::cout << std::left << std::setw(12) << levelName;
			Logger::logLevel lvl = static_cast<Logger::logLevel>(i - 1);
			bool muted = mutedLevels.count(lvl);
			std::cout <<"  Muted: " << (muted ? "ON" : "OFF") << _colorStr[NONE] << std::endl;
		}
		std::cout << "\x1b[0m";
	}
}

bool	Logger::_handleInput(int &selected, bool &filtering, std::set<logLevel> &mutedLevels, std::vector<std::string> &options) {
	int	key = _readKey();
	if (key == -1)
		return true;
	if (key == 'A') {
		if (selected > 1)
			selected--;
		else
			selected = options.size() - 1;
	}
	else if (key == 'B') {
		if (selected < static_cast<int>(options.size()) - 1)
			selected++;
		else
			selected = 1;
	}
	else if (key == 'm') {
		Logger::logLevel lvl = static_cast<logLevel>(selected - 1);
		if (mutedLevels.find(lvl) != mutedLevels.end())
			mutedLevels.erase(lvl);
		else
			mutedLevels.insert(lvl);
	}
	else if (key == 'f') {
		filtering = !filtering;
		options.front() = _toggleFilteredOutputOption(filtering);
	}
	else if (key == '\n' || key == '\r') {
		_logLevel = static_cast<logLevel>(selected - 1);
		_silencedLevels = mutedLevels;
		setFilteredOutput(filtering);
		return false;
	}
	else if (key == 'q') {
		return false;
	}
	return true;
}

void	Logger::spawnLogSelectionMenu() {
	std::cout << "\x1b[?1049h" << std::flush;
	_enableTermiosRawMode();
	const char*					levels[] = { "NONE", "FATAL", "ERROR", "WARNING", "INFO", "TRACE", "DEBUG" };
	std::vector<std::string>	options;
	int							selected = _logLevel + 1;
	bool						filtering = _filteredOutput;
	std::set<logLevel>			mutedLevels;

	options.push_back(_toggleFilteredOutputOption());
	for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i)
		options.push_back(levels[i]);
	while (true) {
		_clearTermios();
		std::cout << _colorStr[INFO]
		<< "╔══════════════════════════════════════════════════╗\n"
		<< "║                 LOGGER SETTINGS                  ║\n"
		<< "╚══════════════════════════════════════════════════╝\n\n" << _colorStr[NONE];
		_printOptions(selected, mutedLevels, options);
		std::cout << std::endl << _colorStr[INFO]
		<< " ↑/↓   : Navigate\tM : Toggle Mute\n"
		<< " Enter : Confirm \tF : Toggle Filtered Output\n"
		<< " Q     : Use default\n\n" << _colorStr[NONE];
		if (!_handleInput(selected, filtering, mutedLevels, options))
			break;
	}
	_disableTermiosRawMode();
	std::cout << "\x1b[?1049l" << std::flush;
	usleep(30000);
}


/*
 *	---
 */

void	Logger::setLogLevel(logLevel level) { _logLevel = level; }

void	Logger::setFilteredOutput(bool enabled) {
	_filteredOutput = enabled;

	if (enabled && _logLevel >= DEBUG && !_silencedLevels.count(DEBUG) && !_debugFile.is_open()) {
		_debugFile.open("debug.log", std::ios::out | std::ios::trunc);
		if (!_debugFile)
			log(WARNING, "Logger: Failed to open debug.log for writing.");
	}
}

Logger::logLevel	Logger::getLogLevel() { return _logLevel; }

bool	Logger::getFilteredOutput() { return _filteredOutput; }

std::string	Logger::getLevelStr(logLevel level) { return _levelStr[level]; }

std::string	Logger::getColorStr(logLevel level) { return _colorStr[level]; }

std::ofstream	*Logger::getDebugFile() { return &_debugFile; }

/*
 *	---
 */

void	Logger::muteLevel(logLevel level) { _silencedLevels.insert(level); }

void	Logger::unmuteLevel(logLevel level) { _silencedLevels.erase(level); }

void	Logger::resetMutedCache() { _silencedLevels.clear(); }

/*
 *	---
 */

void	Logger::printLogSettings() {
	std::cout << _colorStr[_logLevel] << "Current Log Level Selected: " << _levelStr[_logLevel];
	std::cout << " Filtered output is: " << (_filteredOutput ? "ON" : "OFF") << _colorStr[NONE] << std::endl;
	if (!_silencedLevels.empty()) {
		std::cout << _colorStr[WARNING] << "Muted Levels: ";
		bool	first = true;
		for (std::set<logLevel>::iterator it=_silencedLevels.begin(); it!=_silencedLevels.end() && *it <= _logLevel; it++) {
			if (!first)
				std::cout << ", ";
			else
				first = false;
			std::string	level = _levelStr[*it];
			*(level.end() - 1) = '\0';
			std::cout << level;
		}
		std::cout << _colorStr[NONE] << std::endl;
	}
}

void	Logger::shutdown() {
	if (_debugFile.is_open())
		_debugFile.close();
}

/*
 *	---
 */

std::string	Logger::_getTimeStamp() {
	char	buffer[20];
	time_t	current = time(NULL);
	tm*		local = localtime(&current);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
	return std::string(buffer);
}

void	Logger::_printLog(logLevel level, std::vector<char> &buffer) {
	std::ostream	*out = &std::cout;
	std::string		timestamp = "[" + _getTimeStamp() + "] ";

	if (_filteredOutput) {
		switch (level) {
			case NONE:
				return;
			case FATAL:
			case ERROR:
			case WARNING:
				out = &std::cerr;
				*out << _colorStr[level] << timestamp << _levelStr[level] << buffer.data() << _colorStr[NONE] << std::endl;
				break;
			case INFO:
			case TRACE:
				*out << _colorStr[level] << timestamp << _levelStr[level] << buffer.data() << _colorStr[NONE] << std::endl;
				break;
			case DEBUG:
				if (_debugFile.is_open())
					_debugFile << timestamp << _levelStr[level] << buffer.data() << std::endl;
				break;
			default:
				break;
		}
	}
	else
		std::cout << _colorStr[level] << timestamp << _levelStr[level] << buffer.data() << _colorStr[NONE] << std::endl;
	if (level == FATAL)
		throw std::runtime_error("FATAL log occured");
}

/**
 * @brief Prints log "[LEVEL]\t msg"
 * @param level Requires corresponding log level to be enabled
 * @param format log to print (using printf syntax)
 */
void	Logger::log(logLevel level, const char* format, ...) {
	if (level > _logLevel || _silencedLevels.count(level)) {
		if (level == FATAL)
			throw std::runtime_error("FATAL log occured");
		return;
	}
	std::vector<char>	buffer(1024);
	va_list				args;
	va_start(args, format);
	while (vsnprintf(buffer.data(), buffer.size(), format, args) >= static_cast<int>(buffer.size())) {
		buffer.resize(buffer.size() * 2);
		va_end(args);
		va_start(args, format);
	}
	va_end(args);
	_printLog(level, buffer);
}