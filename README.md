# port-scanner

A small multithreaded TCP/UDP port scanner for Linux, written in C++20.

I made this project to learn C++. It covers sockets, non-blocking I/O, threads, RAII and splitting a program into multiple files.

```
  scanme.nmap.org  (45.33.32.156)
  TCP scan  ·  ports 1-1000  ·  100 workers  ·  1.21s
  │
  ├── open            2
  │   ├── 22/tcp      ssh             SSH-2.0-OpenSSH_6.6.1p1 Ubuntu-2ubuntu2.13
  │   └── 80/tcp      http            HTTP/1.1 200 OK
  │
  ├── filtered        0
  │
  └── closed          998
      └── 1-21, 23-79, 81-1000
```

## Features

- TCP connect scan with a timeout, so filtered ports don't hang the scan
- UDP scan (`-sU`)
- Banner grabbing, plus an HTTP probe for services that wait for the client to speak first
- Scans many ports at once with worker threads
- Accepts domain names or IPs
- Port lists like nmap: `80`, `1-1024`, `22,80,443`, `-p-` for all ports
- Colored tree output. Colors switch off automatically when output goes to a file

## Building

You need Linux, `g++` with C++20 support, and `make`.

```sh
git clone https://github.com/Vynar10/port-scanner.git
cd port-scanner
make
```

This creates the `./scanner` binary. `make clean` removes the build files.

## Usage

```
./scanner <host> [options]
```

| Option | Description | Default |
|---|---|---|
| `-p <ports>` | ports to scan: `80`, `1-1024`, `22,80,443` or a mix | `1-1024` |
| `-p-` | scan all ports (1-65535) | |
| `-t <seconds>` | timeout per port | `1` |
| `-w <workers>` | ports scanned at the same time (1-500) | `100` |
| `-sU` | UDP scan instead of TCP | |
| `--no-color` | plain output | |
| `-h` | show help | |

### Examples

```sh
./scanner scanme.nmap.org -p 1-100
./scanner scanme.nmap.org -p 22,80,443 -t 2
./scanner 127.0.0.1 -p- -w 200
./scanner 127.0.0.53 -p 53 -sU
./scanner 192.168.1.1 > results.txt
```

The progress counter goes to stderr and the results go to stdout, so redirecting with `>` saves only the results.

## How it works

**TCP:** for each port, the scanner starts a non-blocking `connect()` and waits on it with `poll()` until the timeout:

- connected: **open**. It then reads a banner, or sends `HEAD / HTTP/1.0` if the service stays quiet.
- connection refused: **closed**
- no answer before the timeout: **filtered**

**UDP:** UDP has no handshake, so the scanner sends a packet (a real DNS or NTP request on ports 53 and 123) and waits:

- a reply: **open**
- ICMP "port unreachable": **closed**
- nothing: **open|filtered**. Silence can't tell an open service apart from a firewall.

**Threads:** the worker threads share an atomic counter, and each one takes the next port from it. The results are collected under a mutex and sorted at the end.

## Project layout

```
include/     headers
src/
  main.cpp       resolve the host, run the scan, print the report
  parse.cpp      command line options
  tcp.cpp        TCP scan of one port
  udp.cpp        UDP scan of one port
  threader.cpp   worker threads
  output.cpp     tree report
  colors.cpp     color setup
```

## Limitations

- IPv4 only
- UDP scans of remote hosts are slow and often show closed ports as open|filtered, because most systems rate-limit their ICMP replies
- Port names come from `/etc/services`, so a service on an unusual port gets the wrong name or `unknown`

## Note

Only scan machines you own or have permission to scan. `scanme.nmap.org` is run by the Nmap project and is free to use for testing.

