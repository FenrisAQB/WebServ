#include "../includes/webserv.hpp"

volatile sig_atomic_t	g_sigint = 0;

void	handleSigint(int signum) {
	if (signum == SIGINT)
		g_sigint = 1;
}

int main(int ac, char **av)
{
	std::map<int, ServerBloc> serverSockets;
	if (ac > 3) {
		Logger::log(Logger::ERROR, "too many arguments. Use: ./webserv ([optional config file]) ([optional 'log' for settings menu])");
		return 1;
	}
	if (std::string(av[ac-1]) == "log") {
		Logger::spawnLogSelectionMenu();
		ac--;
	}
	Logger::printLogSettings();
	ConfigFile	Config;
	if (ac == 2)
		Config.setPath(av[1]);
	try {
		Config.parseFile();
		std::vector<ServerBloc> servers = Config.getServerBlocs();
		serverSockets = init(servers);
		Server server(serverSockets, servers);
		std::signal(SIGINT, handleSigint);
		server.runServer();
	}
	catch (std::exception &e) {}
}
