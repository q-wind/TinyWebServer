## CSAPP-3e Tiny Web Server

### Description

- Now `tiny` is a simple iterative server
- Serve static content: files placed in `public/`
- Serve dynamic content: executable files in `cgi-bin/`

### Build

- Main server executable `tiny` will be placed in: `./bin/`
- CGI executable, such as `add`, will be placed in: `./cgi-bin/`

```bash
mkdir build
cmake -S . -B build
cmake --build build
```

### Run

Make sure the `public` and `cgi-bin` directories in the current running directory

```bash
./bin/tiny <port>
```

### Test

```bash
# GET
curl localhost:<port>/index.html

# POST
curl -X POST localhost:9876/cgi-bin/add \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "first=1&second=2"
```