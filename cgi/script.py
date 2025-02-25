#!/usr/bin/python3
import os

query = os.environ.get("QUERY_STRING", "")
params = dict(param.split("=") for param in query.split("&") if "=" in param)

name = params.get("name", "Guest")
age = params.get("age", "15")

print("Content-Type: text/html")
print()
print(f"<html><body><h1>Hello, {name}, aged {age}!</h1></body></html>")
