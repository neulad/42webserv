NAME := webserv
CPP_FLAGS := -Wall -Wextra -Werror -std=c++98

SRC := src/main.cpp src/server/server.cpp
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

.PHONY: all clean fclean re


