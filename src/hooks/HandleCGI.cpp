#include "HandleCGI.hpp"

/// Static map to store the interpreters for different extensions
/// This map is initialized in the constructor of the CGI class
std::map<std::string, std::string> CGI::_interpretersMap;

CGI::CGI(const std::string &extension, const std::string &interpreter) : _extension(extension), _interpreter(interpreter)
{
	initMap();
}

/**
Initializes the _interpretersMap with the given extensions and interpreters.

The extensions and interpreters are split by ':' and stored in the map.
If the number of extensions and interpreters do not match, an exception is thrown.
If the extensions or interpreters are empty, an exception is thrown.
*/
void CGI::initMap()
{
	std::vector<std::string> extensions = split(this->_extension, ':');
	std::vector<std::string> interpreters = split(this->_interpreter, ':');

	if (extensions.empty() || interpreters.empty())
		throw http::HttpError("Wrong CGI arguments", http::FailedDependency);

	if (extensions.size() != interpreters.size())
		throw http::HttpError("Wrong CGI arguments", http::FailedDependency);

	for (unsigned long i = 0; i < extensions.size(); ++i)
		_interpretersMap[extensions[i]] = interpreters[i];
}

/**
Handles the CGI request.

This function is responsible for handling the CGI request.
It sets the necessary environment variables, forks a child process to execute the CGI script,
and reads the output from the script to set the response body and headers.
It also handles POST requests by setting the CONTENT_TYPE and CONTENT_LENGTH environment variables.

@param req The HTTP request object.
@param res The HTTP response object.
*/
void CGI::handleCgi(http::Request const &req, http::Response &res)
{
	// Unsetting Content-Length to avoid issues with POST requests
	unsetenv("CONTENT_LENGTH");

	std::string scriptPath = getScriptPath(req.getUri());

	if (!isCgi(scriptPath, _interpretersMap))
		return;

	// Initialize variables
	bool isPost = (req.getMethod() == "POST");
	int pipefd[2];
	std::string uri = req.getUri();
	std::string queryString = isPost ? req.getBody() : getQueryString(uri);

	// If the request is a POST request, we need to set the environment variables
	if (isPost)
	{
		if (pipe(pipefd) == -1)
			throw http::HttpError("Pipes creation failed.",
								  http::InternalServerError);
		std::stringstream ss;
		ss << queryString.length();
		std::string val = req.getHeader("Content-Type");
		setenv("CONTENT_TYPE", val.c_str(), 1);
		setenv("CONTENT_LENGTH", ss.str().c_str(), 1);
	}

	// Create a pipe to communicate with the child process
	pid_t pid = safeFork();
	if (pid != 0)
	{
		handleParent(queryString.c_str(), isPost, pipefd, pid);
	}
	else
	{
		std::string interpreter = getInterpreter(scriptPath, _interpretersMap);
		char *castedInterpreter = const_cast<char *>(interpreter.c_str());
		handleChild(scriptPath.c_str(), queryString.c_str(), isPost, pipefd, castedInterpreter);
	}

	// Read the output to temp file and set the response
	std::string result = readFileToString(".cgi-output");
	setResBody(result, res);
	setResHeader(result, res);
	std::remove(".cgi-output");
}
