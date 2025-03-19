#!/usr/bin/env python3

import os
import cgi
import urllib.parse

print("Content-type: text/html\n")

# Get the query string from environment variables
query_string = os.getenv("QUERY_STRING", "")

# Parse query parameters
params = urllib.parse.parse_qs(query_string)

# Extract the "name" parameter if present
name = params.get("name", [""])[0].strip()

# Debugging output
debug_info = f"""
<h3>Debugging Info:</h3>
<ul>
    <li>Raw Query String: {query_string}</li>
    <li>Parsed Name: {name}</li>
</ul>
"""

html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GET Request Test</title>
</head>
<body>
    <h1>GET Request Test</h1>
    <p>Enter your name in the URL query string to see a personalized greeting.</p>
    <p>Example: <code>?name=John</code></p>

    <h2>{f"Hello, {name}!" if name else "No name provided in the query string."}</h2>

    {debug_info}
</body>
</html>
"""

print(html_content)
