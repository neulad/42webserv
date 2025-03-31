#!/bin/bash

echo "Content-type: text/html"
echo ""

# Read POST data from stdin
if [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
    read -r -n "$CONTENT_LENGTH" POST_DATA
else
    POST_DATA=""
fi

# Parse the POST data
NAME=$(echo "$POST_DATA" | sed -n 's/^name=//p' | tr + ' ' | sed 's/%20/ /g')

# Debugging output (optional)
echo "Raw POST Data: $POST_DATA" >> /tmp/cgi_debug.log
echo "Parsed Name: $NAME" >> /tmp/cgi_debug.log

# Generate response HTML
echo "<!DOCTYPE html>"
echo "<html lang=\"en\">"
echo "<head>"
echo "    <meta charset=\"UTF-8\">"
echo "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
echo "    <title>POST Request Test</title>"
echo "</head>"
echo "<body>"
echo "    <h1>Test POST Request</h1>"
echo "    <form action=\"/cgi-bin/post-test.sh\" method=\"post\">"
echo "        <label for=\"name\">Enter your name:</label>"
echo "        <input type=\"text\" id=\"name\" name=\"name\" required>"
echo "        <button type=\"submit\">Send POST Request</button>"
echo "    </form>"
echo "    <hr>"
echo "    <h3>Received Data:</h3>"
echo "    <p>${NAME:-No data received yet.}</p>"
echo "</body>"
echo "</html>"