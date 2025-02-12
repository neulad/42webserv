NAME := webserv
CPP_FLAGS := -Wall -Wextra -Werror -std=c++98

SRC := src/main.cpp src/server/server.cpp \
	src/http/http.cpp src/server/RequestFactory.cpp \
	src/utils/utils.cpp src/hooks/ParseQuery.cpp src/hooks/HandleStatic.cpp
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
	c++ $(SRC) -g -o test

.PHONY: all clean fclean re test


