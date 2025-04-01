#include "CGIProcessHandling.hpp"

/**
Handle the parent process for CGI execution.

@param query The query string to be passed to the CGI script.
@param isPost Indicates if the request is a POST request.
@param pipefd The file descriptors for the pipe used for communication.
@param pid The process ID of the child process.
*/
void handleParent(const char *query, bool isPost, int *pipefd, pid_t pid)
{
	// Write params to the pipe if POST
	if (isPost)
	{
		ssize_t written = write(pipefd[1], query, strlen(query));
		if (written == -1)
			throw http::HttpError("Write to pipe failed", http::InternalServerError);
		close(pipefd[1]);
	}

	waitpid(pid, NULL, 0);
	close(pipefd[0]);
}

/**
Handle the child process for CGI execution.

@param path The path to the CGI script.
@param query The query string to be passed to the CGI script.
@param isPost Indicates if the request is a POST request.
@param pipefd The file descriptors for the pipe used for communication.
@param interpreter The path to the interpreter (e.g., "/usr/bin/python").
*/
void handleChild(const char *path, const char *query, bool isPost,
				 int *pipefd, char *interpreter)
{
	// Redirect stdin to the pipe if POST
	if (isPost)
	{
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
	}

	if (access(path, F_OK) != 0)
		throw http::HttpError("Can't access CGI file", http::BadRequest);

	int output = safeOpen(".cgi-output", O_CREAT | O_TRUNC | O_RDWR);

	if (!isPost)
		setenv("QUERY_STRING", query, 1);
	dup2(output, STDOUT_FILENO);
	close(output);

	// Handle execve
	extern char **environ;
	char *argv[] = {interpreter, (char *)path, NULL};
	execve(interpreter, argv, environ);

	throw http::HttpError("Execve failed", http::BadRequest);
}