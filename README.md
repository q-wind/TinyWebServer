## CSAPP-3e Tiny Web Server

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run

```bash
./bin/tiny <port>
```

### Test

```bash
telnet localhost 80

GET / HTTP/1.0

```