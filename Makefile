NAME := webserv
CPP_FLAGS := -Wall -Wextra -Werror -std=c++98

SRC := src/main.cpp src/server/server.cpp \
	src/server/Request.cpp src/server/RequestFactory.cpp \
	src/utils/utils.cpp
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
	c++ $(shell find . -type f -name "*.cpp") -g -o test

.PHONY: all clean fclean re test


