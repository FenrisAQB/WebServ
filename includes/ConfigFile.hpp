#ifndef CONFIGFILE_HPP
# define CONFIGFILE_HPP

# include <vector>
# include <fstream>
# include <string>
# include "ServerBloc.hpp"
# include "utils.hpp"

class	ServerBloc;

class	ConfigFile
{
	private:
		const char*				_path;
		std::vector<ServerBloc>	_serverBlocs;

		void	duplicateServerName() const;

	public:
		ConfigFile();
		~ConfigFile();

		std::vector<ServerBloc>	getServerBlocs() const;

		void	setPath(const char* path);

		void	parseFile();

		void	printServerBlocs() const;
};

#endif