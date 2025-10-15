# Output file name
NAME			= webserv

# Define sources
SRC				=	main.cpp \
					logger/Logger.cpp \
					config/ConfigFile.cpp \
					config/ServerBloc.cpp \
					config/LocationBloc.cpp \
					config/utils.cpp \
					init/init.cpp \
					request/Request.cpp \
					handler/Handler.cpp \
					response/Response.cpp \
					server/Server.cpp \
					client/Client.cpp \
					cgi/Cgi.cpp

# Define dirs
OBJ_DIR			= objs/
SRC_DIR			= srcs/

# Full sources path
SRCS			= $(addprefix $(SRC_DIR), $(SRC))

# Define object files
OBJS			= $(SRCS:$(SRC_DIR)%.cpp=$(OBJ_DIR)%.o)

# Compilation message
TOT_FILES		= $(shell expr $(words $(SRC)))
COMPILED_FILES	= 0
MESSAGE			= "Compiling: $(COMPILED_FILES)/$(TOT_FILES) ($(shell expr $(COMPILED_FILES) \* 100 / $(TOT_FILES))%)"

# Text styling
GREEN			= \033[32m
RED				= \033[31m
BLUE			= \033[34m
GREY			= \033[90m
YELLOW			= \033[33m
BG_BLUE			= \033[44m
ENDCOLOR		= \033[0m
BOLD			= \033[1m

# Texts
START			= echo "$(BLUE)Compilation of $(NAME) started\n$(ENDCOLOR)"
END				= echo "$(GREEN)$(BOLD)✔ Compilation finished\n$(ENDCOLOR)"
CLEAN_MSG		= echo "$(RED)$(BOLD)✖ Deleting object files\n$(ENDCOLOR)"
FCLEAN_MSG		= echo "$(RED)$(BOLD)✖ Deleting program\n$(ENDCOLOR)"
ENTER_LDIR		= echo "$(BLUE)➜$(ENDCOLOR)$(BOLD)  Entering directory$(RESET) $(YELLOW)$(LIBFT_DIR)$(ENDCOLOR)"
LEAVE_LDIR		= echo "$(BLUE)➜$(ENDCOLOR)$(BOLD)  Leaving directory$(RESET) $(YELLOW)$(LIBFT_DIR)$(ENDCOLOR)"
ENTER_MDIR		= echo "$(BLUE)➜$(ENDCOLOR)$(BOLD)  Entering directory$(RESET) $(YELLOW)$(MLX_DIR)$(ENDCOLOR)"
LEAVE_MDIR		= echo "$(BLUE)➜$(ENDCOLOR)$(BOLD)  Leaving directory$(RESET) $(YELLOW)$(MLX_DIR)$(ENDCOLOR)"

# Define bar variables
BAR_LEN			= 70
BAR				= $(shell expr $(COMPILED_FILES) \* $(BAR_LEN) / $(TOT_FILES))

# Cleaning command
RM				= rm -f

# Compiler / debugger
CC				= c++
DEBUGGER		= valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes --verbose --log-file=valgrind-out.txt

# Compiler / debugger flags
CFLAGS			= -Wall -Wextra -Werror -std=c++98 -g -Wno-conversion
DFLAGS			= -ggdb3

# Define all
all:			start2 start3 start $(NAME) end

# Define logo
logo:
				@tput setaf 5; tput bold; cat ./ascii_art/logo; echo "\n"; tput init;

# Define start visuals
start:
				@tput setaf 4; cat ./ascii_art/CDRX;  echo "\n"; tput init;

start2:
				@tput setaf 4; cat ./ascii_art/Fenris;  echo "\n"; tput init;

start3:
				@tput setaf 4; cat ./ascii_art/Loic;  echo "\n"; tput init;

# Define end visual
end:
				@tput setaf 2; tput bold; cat ./ascii_art/done; echo "\n"; tput init;

# Create object files
$(OBJ_DIR)%.o : $(SRC_DIR)%.cpp
				@mkdir -p $(dir $@)
				@$(CC) $(CFLAGS) -c $< -o $@
				$(eval COMPILED_FILES=$(shell echo $$(($(COMPILED_FILES)+1))))
				@printf "$(BLUE)%s\r\n$(ENDCOLOR)" $(MESSAGE)
				@sleep 0.1
				@printf "$(BG_BLUE)%*s$(ENDCOLOR)\r" $(BAR) " "
				@printf "\033[F"

# Create program file
$(NAME):		logo $(OBJS)
				@printf "\n\n"
				@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
				@$(END)

# Create program file for debugging
$(NAME)_debug:	logo $(OBJS)
				@printf "\n\n"
				@$(CC) $(CFLAGS) $(DFLAGS) $(OBJS) -o $(NAME)_debug
				@$(END)

# Run program
run:			start2 start3 start $(NAME) end
				./$(NAME) conf/valid/multiple_servers.conf

debug:			start2 start3 start $(NAME)_debug end
				@$(DEBUGGER) ./$(NAME)_debug conf/valid/multiple_servers.conf

# Clean object files & if on linux rm valgrind-out.txt
clean:
				@$(CLEAN_MSG)
				@$(RM) -r $(OBJ_DIR)
				@tput setaf 1; tput bold; cat ./ascii_art/trash; tput init;
				@tput setaf 1; tput bold; cat ./ascii_art/gone; echo "\n"; tput init;

# Clean object files and program file
fclean:			clean
				@$(FCLEAN_MSG)
				@$(RM) $(NAME)
				@$(RM) $(NAME)_debug
				@$(RM) valgrind-out.txt
				@tput setaf 1; tput bold; cat ./ascii_art/trash; tput init;
				@tput setaf 1; tput bold; cat ./ascii_art/all_gone; tput init;

# Redo everything
re:				fclean all

# Avoid collisions with files named the same as commands
.PHONY:			all clean fclean re run
