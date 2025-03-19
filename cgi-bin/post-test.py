#!/usr/bin/env python3

import os
import sys
import urllib.parse

print("Content-type: text/html\n")

# Read raw POST data from stdin
post_data = ""
if "CONTENT_LENGTH" in os.environ:
    content_length = int(os.environ["CONTENT_LENGTH"])
    post_data = sys.stdin.read(content_length)

# Parse the POST data
form_data = urllib.parse.parse_qs(post_data)
name = form_data.get("name", [""])[0].strip()

# Debugging output (optional)
with open("/tmp/cgi_debug.log", "a") as log:
    log.write(f"Raw POST Data: {post_data}\nParsed Data: {form_data}\n")

# Generate response HTML
html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>POST Request Test</title>
</head>
<body>
    <h1>Test POST Request</h1>

    <form action="/cgi-bin/post-test.py" method="post">
        <label for="name">Enter your name:</label>
        <input type="text" id="name" name="name" required>
        <button type="submit">Send POST Request</button>
    </form>

    <hr>

    <h3>Received Data:</h3>
    <p>{f"Hello, {name}!" if name else "No data received yet."}</p>

    <!-- Debug Info -->
    <pre>POST Data: {post_data}</pre>
</body>
</html>
"""

print(html_content)
