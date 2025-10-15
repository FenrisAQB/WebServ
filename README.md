# Webserv
A simple web server written in c++98 in the context of a 42 School project. \
Grade: 110%

# Compatibility
Webserv has only ever been tested and run on a Linux environment.

# Compiling
If you want to try it for yourself and play around with it, you can clone the repository :
```
git clone https://github.com/FenrisAQB/WebServ.git WebServ
```
Compile the project with :
```
cd WebServ && make
```
And run it with :
```
./webserv
```
Or for more customization : \
./webserv [optional path to config file] (optionally add "log" as last argument to open the server logging settings)

# Usage
There is a full interactive testing script available in the /tests folder. For some tests, it requires siege (sudo apt install siege). \
There are three valid configurations provided for testing purposes. \
Start your browser, point it to one of the IPs in the server configurations and enjoy! \
Some IP's may not be available on your machine, simply change them in the configuration file and restart the server.

# Credit.
Once more after [Minishell](https://github.com/FenrisAQB/Minishell) and [Cub3D](https://github.com/FenrisAQB/Cub3D), a pleasure to work with my usual teammate [CDRX](https://github.com/CDRX2) and [LPittet](https://github.com/LPittet1).
