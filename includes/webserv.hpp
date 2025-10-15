#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <string>
#include <iostream>
# include <csignal>
# include "ConfigFile.hpp"
# include "Logger.hpp"
# include "Server.hpp"

std::map<int, ServerBloc> init(std::vector<ServerBloc>& servers);

#endif