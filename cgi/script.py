#!/usr/bin/env python3

import sys
import os
import urllib.parse
import datetime

print("Content-type: text/html\n")

# Get current time for greeting
now = datetime.datetime.now()
hour = now.hour

greeting = "Good Morning" if hour < 12 else "Good Afternoon" if hour < 18 else "Good Evening"

# Read Raw Stdin
content_length = int(os.environ.get("CONTENT_LENGTH", 0))
stdin_data = sys.stdin.read(content_length)

# Debugging Output
print("<pre>")  # Preformatted text for debugging
print("Raw Stdin Data:")
print(stdin_data if stdin_data else "No stdin data received.")

# Parse form data manually
form_data = urllib.parse.parse_qs(stdin_data)
name = form_data.get("name", [""])[0].strip()

# Debugging Output
print("\nParsed Name:")
print(name if name else "None")
print("</pre>")

html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CGI Dynamic Website</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
    <style>
        body {{ background-color: #f8f9fa; }}
        .container {{ margin-top: 50px; max-width: 600px; }}
        .card {{ border-radius: 15px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }}
    </style>
</head>
<body>
    <div class="container text-center">
        <div class="card p-4">
            <h1 class="text-primary">{greeting}!</h1>
            <p class="lead">Welcome to my Python CGI-powered website.</p>
            <form method="post" action="/cgi/script.py">
                <div class="mb-3">
                    <label for="name" class="form-label">Enter your name:</label>
                    <input type="text" id="name" name="name" class="form-control" placeholder="Your name" required>
                </div>
                <button type="submit" class="btn btn-success">Submit</button>
            </form>
            <hr>
            <h3 class="mt-3">{f"Hello, {name}!" if name else "Enter your name to see a personalized greeting."}</h3>
        </div>
    </div>
</body>
</html>"""

print(html_content)
