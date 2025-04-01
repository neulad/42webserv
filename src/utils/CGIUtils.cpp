#include "./CGIUtils.hpp"
#include <cstddef>

/**
Safe open function

This function opens a file at the specified path with the given mode.
If the open operation fails, it throws an HttpError with a message indicating that the file couldn't be created.
It returns the file descriptor of the opened file if successful.

@param path The path to the file to be opened
@param mode The mode in which to open the file (e.g., O_RDONLY, O_WRONLY, etc.)
@return The file descriptor of the opened file
@throws HttpError if the open operation fails
*/
int safeOpen(std::string const &path, int mode)
{
	int output = open(path.c_str(), mode, 0644);
	if (output < 0)
		throw http::HttpError("Couldn't create a tmp file",
							  http::InternalServerError);
	else
		return (output);
}

/**
Safe fork function

This function creates a child process using the fork() system call.
If the fork fails, it throws an HttpError with a message indicating that the child process couldn't be created.
It returns the process ID of the child process if successful.

@param None
@return The process ID of the child process
@throws HttpError if the fork fails
*/
pid_t safeFork()
{
	pid_t pid = fork();
	if (pid < 0)
		throw http::HttpError("Couldn't create a child process",
							  http::InternalServerError);
	else
		return (pid);
}

/**
Get a query string from a URI

This function takes a URI and returns the query string.
It finds the first occurrence of '?' in the URI and extracts the substring from that position to the end of the string.

@param uri The URI to extract the query string from
@return The query string
*/
std::string getQueryString(std::string uri)
{
	ssize_t pos = uri.find("?");
	return uri.substr(pos + 1, uri.size());
}

/**
Get the script path from a URI

This function takes an URI and returns the path of the script.
It finds the first occurrence of '/' and '?' in the URI and extracts the substring between them.

@param uri The URI to extract the path from
@return The path of the script
*/
std::string getScriptPath(std::string uri)
{
	ssize_t pos1 = uri.find("/");
	ssize_t pos2 = uri.find("?");
	std::string path;
	if (pos2 != 0)
		path = uri.substr(pos1 + 1, pos2 - 1);
	else
		path = uri.substr(pos1 + 1, uri.size());
	return path;
}

/**
Read a file into a string

This function takes a filename as input and reads the contents of the file into a string.
It opens the file, reads its contents, and returns the string representation of the file's contents.
If the file cannot be opened, it throws a runtime_error with a message indicating the failure.

@param filename The name of the file to be read
@return The contents of the file as a string
@throws std::runtime_error if the file cannot be opened
*/
std::string readFileToString(const std::string &filename)
{
	std::ifstream file;
	file.open(filename.c_str());
	if (!file)
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}
	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

/**
Set the response header

This function takes a response string and a reference to an http::Response object.
It extracts the key and value from the response string and sets them as headers in the http::Response object.
If the key-value pair is not found, it throws a runtime_error with a message indicating the failure.

@param response The response string containing the header
@param res A reference to the http::Response object to set the header in
@throws std::runtime_error if the key-value pair is not found
*/
void setResHeader(std::string response, http::Response &res)
{
	std::string key;
	std::string value;
	ssize_t pos = response.find(":");
	ssize_t pos2 = response.find_first_of("\n");
	if (pos <= 0)
		throw std::runtime_error("Mariusz smierdzi ze hej");
	key = response.substr(0, pos);
	value = response.substr(pos + 2, pos2 - (pos + 2));
	res.setHeader(key, value);
}

/**
Set the response body

This function takes a response string and a reference to an http::Response object.
It extracts the body from the response string and sets it in the http::Response object.
It also calculates the content length of the body and sets it as a header in the http::Response object.
If the body is not found, it throws a runtime_error with a message indicating the failure.

@param response The response string containing the body
@param res A reference to the http::Response object to set the body in
*/
void setResBody(std::string response, http::Response &res)
{
	ssize_t pos = response.find_first_of("\n");
	std::string body = response.substr(pos + 1, response.size());
	res.setBody(body);
	int contentLength = body.length();
	std::stringstream ss;
	ss << contentLength;
	res.setHeader("Content-Length", ss.str());
}

/**
Check if a file is executable

This function takes a file path as input and checks if the file is executable.
It uses the access() system call to check for execute permission.

@param path The path to the file to be checked
@return true if the file is executable, false otherwise
@throws std::runtime_error if the access check fails
*/
bool isExecutable(const std::string &path)
{
	return access(path.c_str(), X_OK) == 0;
}

/**
Get the interpreter for a file based on its extension

This function takes a file path and a map of file extensions to interpreters.
It checks the file extension and returns the corresponding interpreter from the map.
If the extension is not found in the map, it returns an empty string.

@param path The path to the file
@param extMap A map of file extensions to interpreters
@return The interpreter for the file, or an empty string if not found
*/
std::string getInterpreter(const std::string &path, const std::map<std::string, std::string> &extMap)
{
	const char *ext = &path[path.find_last_of('.')];
	std::map<std::string, std::string>::const_iterator it = extMap.find((std::string)ext);
	if (it != extMap.end())
	{
		return it->second;
	}
	return "";
}

/**
Check if a file is a CGI script

This function takes a file path and a map of file extensions to interpreters.
It checks if the file is executable and if it has a valid interpreter.

@param path The path to the file
@param extMap A map of file extensions to interpreters
@return true if the file is a CGI script, false otherwise
*/
bool isCgi(const std::string &path, const std::map<std::string, std::string> &extMap)
{
	if (!isExecutable(path))
		return false;
	if (getInterpreter(path, extMap).empty())
		return false;
	return true;
}

/**
Split a string into tokens based on a delimiter

This function takes a string and a delimiter character as input.
It splits the string into tokens based on the delimiter and returns a vector of strings.
It uses a stringstream to read the string and extract tokens until the end of the string is reached.

@param str The string to be split
@param delimiter The character used to split the string
@return A vector of strings containing the tokens
*/
std::vector<std::string> split(const std::string &str, char delimiter)
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;

	while (std::getline(ss, token, delimiter))
	{
		tokens.push_back(token);
	}

	return tokens;
}