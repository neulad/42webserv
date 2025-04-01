#pragma once

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>
#include <stdexcept>
#include "CGIUtils.hpp"
#include "../http/http.hpp"

void handleParent(const char *query, bool isPost, int *pipefd, pid_t pid);
void handleChild(const char *path, const char *query, bool isPost, int *pipefd, char *interpreter);