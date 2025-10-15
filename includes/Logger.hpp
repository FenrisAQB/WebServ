#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <set>
# include <map>
# include <vector>
# include <iostream>
# include <fstream>
# include <cstdarg>
# include <cstdio>
# include <ctime>
# include <termios.h>
# include <unistd.h>
# include <fcntl.h>
# include <iomanip>

class	Logger
{
	public:
		enum	logLevel {
			NONE,
			FATAL,
			ERROR,
			WARNING,
			INFO,
			TRACE,
			DEBUG
		};

	private:
		static struct termios	_originalTermios;

		static logLevel				_logLevel;
		static std::set<logLevel>	_silencedLevels;

		static bool				_filteredOutput;
		static std::ofstream	_debugFile;

		static std::map<logLevel, std::string>	_levelStr;
		static std::map<logLevel, std::string>	_colorStr;
		static std::map<logLevel, std::string>	_genLevelStr();
		static std::map<logLevel, std::string>	_genColorStr();

		static void			_enableTermiosRawMode();
		static void			_disableTermiosRawMode();
		static void			_clearTermios();
		static std::string	_toggleFilteredOutputOption(bool state = _filteredOutput);
		static int			_readKey();
		static void			_printOptions(int &selected, std::set<logLevel> &mutedLevels, std::vector<std::string> &options);
		static bool			_handleInput(int &selected, bool &filtering, std::set<logLevel> &mutedLevels, std::vector<std::string> &options);

		static std::string	_getTimeStamp();

		static void	_printLog(logLevel level, std::vector<char> &buffer);

	public:
		static void	setLogLevel(logLevel level);
		static void	setFilteredOutput(bool enabled);

		static Logger::logLevel	getLogLevel();
		static bool				getFilteredOutput();
		static std::ofstream	*getDebugFile();
		static std::string		getLevelStr(logLevel level);
		static std::string		getColorStr(logLevel level);

		static void	muteLevel(logLevel level);
		static void	unmuteLevel(logLevel level);
		static void	resetMutedCache();

		static void	spawnLogSelectionMenu();
		static void	printLogSettings();

		static void	log(logLevel level, const char* format, ...);

		static void	shutdown();
};

#endif