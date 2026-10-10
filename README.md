# Multithreaded HTTP Server in C++

A small HTTP/1.1 server built from Linux POSIX sockets and the C++ standard library.
It demonstrates TCP networking, HTTP parsing, a fixed-size thread pool, synchronization,
routing, static file serving, logging, graceful shutdown, and basic benchmarking.

## Features

- IPv4 TCP server using `socket`, `bind`, `listen`, `accept`, `recv`, and `send`
- Fixed-size worker thread pool
- Synchronized task queue with `std::mutex` and `std::condition_variable`
- Minimal HTTP/1.1 request parser
- Simple router for GET/POST endpoints
- Static file serving from `public/`
- Request logging
- Graceful shutdown on SIGINT/SIGTERM
- Request-size limit (1 MiB)
- Partial-send handling
- `SO_REUSEADDR`

## Build

### Make

```bash
make
```

### CMake

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./http_server
```

Optional arguments:

```bash
./http_server <port> <worker_threads>
```

Example:

```bash
./http_server 8080 8
```

## Endpoints

- `GET /`
- `GET /hello`
- `GET /health`
- `POST /echo`
- `GET /static/index.html`

## Test

```bash
./scripts/test.sh
```

## Benchmark

Install `wrk` or `ab`, then:

```bash
./scripts/benchmark.sh
```

## Architecture

```text
Client connections
       |
       v
 listening socket
       |
     accept()
       |
       v
 synchronized task queue
       |
   +---+---+---+
   |   |   |   |
   v   v   v   v
 worker threads
   |   |   |   |
   +---+---+---+
       |
       v
 parse -> route -> response -> send
```

## Limitations

This is intentionally an educational HTTP server, not a production replacement for nginx/Apache.
It closes the connection after each request and does not yet implement TLS, chunked transfer encoding,
HTTP pipelining, full RFC-compliant parsing, or an event-driven `epoll` loop.
