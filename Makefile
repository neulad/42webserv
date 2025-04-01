NAME := webserv
CPP_FLAGS := -Wall -Wextra -Werror -std=c++98 -g

SRC := src/main.cpp src/server/server.cpp src/server/Config.cpp src/server/FilefdFactory.cpp \
	src/http/http.cpp src/server/ConnectionFactory.cpp src/hooks/HandleCGI.cpp \
	src/utils/utils.cpp src/hooks/ParseQuery.cpp src/hooks/HandleStatic.cpp \
	src/hooks/ConfigHandler.cpp src/utils/CGIUtils.cpp src/utils/CGIProcessHandling.cpp
OBJS := $(SRC:.cpp=.o)
all: $(NAME)

.cpp.o:
	c++ $(CPP_FLAGS) -c $< -o $@

$(NAME): $(OBJS)
	c++ $(CPP_FLAGS) $^ -o $(NAME)

clean:
	rm -rf $(OBJS)
fclean: clean
	rm -rf $(NAME)
re: fclean all
test:
	c++ $(SRC) -Wall -Wextra -Werror -o test

.PHONY: all clean fclean re test


