#!/usr/bin/env bash
set -euo pipefail


BASE_URL="${1:-http://127.0.0.1:9090}"

echo "GET /"
curl -i "$BASE_URL/"

echo -e "\nGET /hello"
curl -i "$BASE_URL/hello"

echo -e "\nGET /health"
curl -i "$BASE_URL/health"

echo -e "\nPOST /echo"
curl -i -X POST "$BASE_URL/echo" -H 'Content-Type: text/plain' --data 'hello from curl'

echo -e "\nGET /static/index.html"
curl -i "$BASE_URL/static/index.html"
