#ifndef SERVERBLOC_HPP
# define SERVERBLOC_HPP

# include <vector>
# include <map>
# include <string>
# include <cstdlib>
# include <fstream>
# include <iostream>
# include <iomanip>
# include "LocationBloc.hpp"
# include "utils.hpp"
# include "Logger.hpp"

class	LocationBloc;

class	ServerBloc
{
	private:
		const char*													_path;
		bool														_validServer;
		bool														_empty;
		std::map<std::string, std::pair<std::string, std::string> >	_listens;
		std::vector<std::string>									_serverNames;
		std::string													_root;
		std::map<int, std::string>									_errorPages;
		unsigned long long											_maxBodySize;
		std::map<std::string, LocationBloc>							_locations;

		void	addListens(std::string &arg);
		void	setServerNames(std::vector<std::string> &args);
		void	setRoot(std::string &arg);
		void	addErrorPages(std::vector<std::string> &args);
		void	setMaxBodySize(std::string &arg);

		void	parseServerKey(std::vector<std::string> &args);

		void	printListens(std::ostream *out) const;

	public:
		ServerBloc();
		ServerBloc(const char* path);
		~ServerBloc();

		ServerBloc	&operator=(const ServerBloc &rhs);

		bool	parseServerBloc(std::ifstream &file);

		std::map<std::string, std::pair<std::string, std::string> >	getListens() const;
		std::vector<std::string>									getServerNames() const;
		std::string													getRoot() const;
		std::map<int, std::string>									getErrorPages() const;
		unsigned long long											getMaxBodySize() const;
		std::map<std::string, LocationBloc>							getLocations() const;

		bool	areServerNamesfound(std::vector<std::string> &names) const;

		bool	findLocation(const std::string &target, LocationBloc &location) const;

		bool	containsListen(const std::string &target) const;
		bool	containsServerName(const std::string &target) const;
		bool	hasErrorPages(int &code) const;

		void	printServerBloc(int indent) const;
};

#endif