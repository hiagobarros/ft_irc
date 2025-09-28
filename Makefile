# --- Compilation Variables ---
CXX = c++

# --- Server (Mandatory Part) ---
NAME = ircserv
SRCS = src/main.cpp src/Server.cpp src/Client.cpp src/Channel.cpp
OBJS = $(SRCS:.cpp=.o)
# Strict C++98 flags for the server
CXXFLAGS_SERVER = -Wall -Wextra -Werror -std=c++98

# --- Bot (Bonus Part) ---
BOT_NAME = ircbot
BOT_SRCS = src/bot_main.cpp src/Bot.cpp
BOT_OBJS = $(BOT_SRCS:.cpp=.o)
# C++11 flags and include directory for the bot
CXXFLAGS_BOT = -Wall -Wextra -Werror -std=c++98 -I./include/
# Required libraries for the bot (CPR, cURL, Threads)
BOT_LIBS = -lcpr -lcurl -lpthread

# --- DCC Client (Bonus Part) ---
DCC_NAME = dcc_client
DCC_SRCS = src/dcc_main.cpp src/DccClient.cpp
DCC_OBJS = $(DCC_SRCS:.cpp=.o)
# Flags podem ser C++98, já que não usamos threads aqui
CXXFLAGS_DCC = -Wall -Wextra -Werror -std=c++98




# --- Main Rules ---

# The 'all' rule compiles only the mandatory part (server)
all: $(NAME)

# Rule to compile the server (mandatory part)
$(NAME): $(OBJS)
	@echo "Compiling the IRC Server..."
	$(CXX) $(CXXFLAGS_SERVER) -o $(NAME) $(OBJS)
	@echo "Server $(NAME) compiled successfully."

# --- Bonus Rules ---

# The 'bonus' rule compiles the server + bot + dcc_client
bonus: $(NAME) $(BOT_NAME) 
#$(DCC_NAME)

# Rule to compile the bot (bonus part)
$(BOT_NAME): $(BOT_OBJS)
	@echo "Compiling the Bot..."
	$(CXX) $(CXXFLAGS_BOT) -o $(BOT_NAME) $(BOT_OBJS)
	@echo "Bot $(BOT_NAME) compiled successfully."

# Rule to compile the DCC Client (bonus part)
#$(DCC_NAME): $(DCC_OBJS)
#	@echo "Compiling the DCC Client..."
#	$(CXX) $(CXXFLAGS_DCC) -o $(DCC_NAME) $(DCC_OBJS)
#	@echo "DCC Client $(DCC_NAME) compiled successfully."

# --- Cleanup Rules ---

# The 'clean' rule removes object files for both targets
clean:
	@echo "Cleaning object files..."
	rm -f $(OBJS) $(BOT_OBJS) #$(DCC_OBJS)
	@echo "Cleanup complete."

# The 'fclean' rule removes object files and executables
fclean: clean
	@echo "Cleaning executables..."
	rm -f $(NAME) $(BOT_NAME) #$(DCC_NAME) 
	@echo "Full cleanup complete."

# The 're' rule performs a full cleanup and recompiles everything
re: fclean all

# The 'rebonus' rule performs a full cleanup and recompiles everything including bonus
rebonus: fclean bonus

# --- Debug Rule (Optional) ---
# Allows compiling with debug flags, e.g., 'make debug'
debug: CXXFLAGS_SERVER += -g
debug: CXXFLAGS_BOT += -g
debug: CXXFLAGS_DCC += -g
debug: rebonus

# --- Phony ---
# Informs 'make' that these rules do not produce files with their names
.PHONY: all bonus clean fclean re rebonus debug
