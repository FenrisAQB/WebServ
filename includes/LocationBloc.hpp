#ifndef LOCATIONBLOC_HPP
# define LOCATIONBLOC_HPP

# include <vector>
# include <map>
# include <string>
# include <cstdlib>
# include <algorithm>
# include <fstream>
# include "utils.hpp"
# include "Logger.hpp"

class	LocationBloc
{
	private:
		const char*							_path;
		bool								_validLocation;
		bool								_empty;
		std::string							_route;
		std::vector<std::string>			_allowedMethods;
		std::pair<int, std::string>			_return;
		std::string							_alias;
		bool								_autoIndex;
		std::vector<std::string>			_indexes;
		std::map<std::string, std::string>	_cgis;
		std::string							_uploadPath;
	
		void	setAllowedMethods(std::vector<std::string> &args);
		void	setReturn(std::vector<std::string> &args);
		void	setAlias(std::string &arg);
		void	setAutoIndex(std::string &arg);
		void	setIndexes(std::vector<std::string> &args);
		void	addCgi(std::vector<std::string> &args);
		void	setUploadPath(std::string &arg);

		void	parseLocationKey(std::vector<std::string> &args);

	public:
		LocationBloc();
		LocationBloc(const char* path);
		~LocationBloc();

		LocationBloc	&operator=(const LocationBloc &rhs);

		bool	parseLocationBloc(std::ifstream &file, std::string &route);

		std::string							getRoute() const;
		std::vector<std::string>			getAllowedMethods() const;
		std::pair<int, std::string>			getReturn() const;
		std::string							getAlias() const;
		bool								getAutoIndex() const;
		std::vector<std::string>			getIndexes() const;
		std::map<std::string, std::string>	getCgis() const;
		std::string							getUploadPath() const;

		bool	containsMethod(const std::string &method) const;
		bool	hasCgis(std::string &ext) const;

		void	printLocationBloc(int indent) const;
};

#endif