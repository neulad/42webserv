#!/usr/bin/env node

const querystring = require("querystring");

const contentLength = process.env.CONTENT_LENGTH
  ? parseInt(process.env.CONTENT_LENGTH, 10)
  : 0;
let body = "";

if (contentLength > 0) {
  process.stdin.on("data", (chunk) => {
    body += chunk;
  });

  process.stdin.on("end", () => {
    handleRequest(body);
  });
} else {
  handleRequest(""); // Handle cases where there's no POST data
}

function handleRequest(body) {
  const postData = querystring.parse(body);
  const name = postData.name
    ? decodeURIComponent(postData.name.replace(/\+/g, " "))
    : "No data received yet.";

  // Debugging output (optional)
  require("fs").appendFileSync(
    "/tmp/cgi_debug.log",
    `Raw POST Data: ${body}\nParsed Name: ${name}\n`,
  );

  // Generate response
  console.log("Content-type: text/html\n");
  console.log(`<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>POST Request Test</title>
</head>
<body>
    <h1>Test POST Request</h1>
    <form action="/cgi-bin/post-test.js" method="post">
        <label for="name">Enter your name:</label>
        <input type="text" id="name" name="name" required>
        <button type="submit">Send POST Request</button>
    </form>
    <hr>
    <h3>Received Data:</h3>
    <p>${name}</p>
</body>
</html>`);
}
