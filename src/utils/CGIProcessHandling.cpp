#include "CGIProcessHandling.hpp"

void handleParent(const char *query, bool isPost, int *pipefd, pid_t pid)
{
	if (isPost)
	{
		ssize_t written = write(pipefd[1], query, strlen(query));
		if (written == -1)
		{
			std::cerr << "Error writing to pipe: " << strerror(errno) << std::endl;
			exit(1);
		}
		close(pipefd[1]);
	}
	waitpid(pid, NULL, 0);
	close(pipefd[0]);
}

void handleChild(const char *path, const char *query, bool isPost,
				 int *pipefd, char *interpreter)
{
	if (isPost)
	{
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
	}

	if (access(path, F_OK) != 0)
	{
		throw http::HttpError("Can't access CGI file", http::BadRequest);
	}

	try
	{
		int output = safeOpen("output.txt", O_CREAT | O_TRUNC | O_RDWR);
		if (!isPost)
		{
			setenv("QUERY_STRING", query, 1);
		}
		dup2(output, STDOUT_FILENO);
		close(output);
		extern char **environ;
		char *argv[] = {interpreter, (char *)path, NULL};
		execve(interpreter, argv, environ);
		throw http::HttpError("execve failed", http::BadRequest);
	}
	catch (const std::exception &e)
	{
		throw http::HttpError(e.what(), http::BadRequest);
	}
}