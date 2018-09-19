# dnstress

## General description

`dnstress` is a tool that provides performance stress tests for defined DNS servers.

## Features

First of all, `dnstress` uses a configuration file to setup some of internal structures. The default path is `./dnstress.json` (it will be changed in the future). The example of a configuration file is shown below:

```json
{
    "addrs": [ "127.0.0.2", "127.0.0.3", "[2a0d:b600:166:ff00:e2d5:5eff:fe3a:a880]:1337" ],
    "workers": 2,
    "queries": [
        { "dname": "google.com", "blocked": 0, "type": "AAAA" },
        { "dname": "vk.com",     "blocked": 1, "type": "A" }
    ]
}
```

### UDP and TCP
`dnstress` is able to work with both UDP and TCP protocols. Which one to use (or both) is defined in the command line arguments. See [Stress test modes](#stress-test-modes) for more information.

### Custom queries

In the config file you can specify which queries will be used for stressing. You can specify them as follows:

```json
    "queries": [
        { "dname": "google.com", "blocked": 0, "type": "AAAA" },
        { "dname": "vk.com",     "blocked": 1, "type": "A" }
    ]
```

Some (happy) little explanation of object fields:

| Field | Description |
| ----- | ----------- |
| dname | domain name string used for request |
| blocked | 1 if domain should be blocked by DNS server (only for statistics), 0 otherwise |
| type | type of request (A, AAAA, etc) for this specific domain name |

### Stress test modes

There are several executing modes that can be specified in the command line arguments before running `dnstress`. It's important to notice that every request is taken from the config file based on a stress test mode. All available modes are listed below:

| Mode | Description |
| ---- | ----------- |
| udp-valid | UDP requests with `blocked` flag equals `0` |
| tcp-valid | TCP requests with `blocked` flag equals `0` |
| udp-nonvalid | UDP requests with `blocked` flag equals `1` |
| tcp-nonvalid | TCP requests with `blocked` flag equals `1` |
| shuffle | random choice of a query from the config file |

### Several DNS servers

`dnstress` supports specifying a list of DNS servers. It is useful when your DNS server has several addresses and you want to check if all of them work correctly. 

In the configuration file you can specify DNS server addresses as follows:

```json
"addrs": [ "127.0.0.2", "127.0.0.3", "[2a0d:b600:166:ff00:e2d5:5eff:fe3a:a880]:1337" ] 
```

Notice that you can specify ports as well!

### Workers

In the configuration file you can also specify number of workers. `Worker` here is a process that was forked from the parent process of `dnstress`. The more workers you specify, the more requests you will be producing.

Speify number of workers in the json config like that:

```json
"workers": 2
```

There is some limitation on the number of workers: from 1 to 10

## How to build

Before building `dnstress`, take a look at the [requirements](#requirements) section. When all libraries are installed, type the next in the terminal to build or rebuild `dnstress`:

```sh
$ make clean && make
```

## How to run and use

To list all supported command line arguments, type `--help`:

```sh
$ ./dnstress --help

Usage: ./dnstress [--mode m] [--conf f] [--help]
       --mode: mode for traffic generating (low-valid by default). Available modes:          udp-valid:    UDP valid packets
           tcp-valid:    TCP valid packets
           tcp-nonvalid: TCP non-valid (to be blocked) packets
           shuffle: random packets generating
       --conf: file with dnstress configuration
```

As you can see, `help` is self-documented. You need to just specify desired options when you run `dnstress`. Defualt mode is `udp-valid`. Run it like this:

```sh
$ ./dnstress --mode shuffle


Enter Ctrl+C to close dnstress

[+] process workers are starting...

proc-worker: 0 | starting dnstress...
proc-worker: 1 | starting dnstress...
```

To close dnstress, you need to enter `Ctrl+C`. After this, all collected statistics will be printed on the screen. For instance, this can be:

```sh
   STRESS STATISTIC

    ├─ 127.0.0.2
    │    ├─── UDP
    │    │    ├── NETWORK
    │    │    │   ├─ queries:   1200
    │    │    │   ├─ responses: 770
    │    │    │   └─ loss:      35.83%
    │    │    │          
    │    │    └── RESPONSE CODES
    │    │        ├─ noerr:     770
    │    │        ├─ corrupted: 0
    │    │        ├─ formaerr:  0
    │    │        ├─ servfail:  0
    │    │        └─ errors:    0.00%
    │    │               
    │    └─── TCP
    │         ├── NETWORK
    │         │   ├─ queries:   1200
    │         │   ├─ responses: 1163
    │         │   └─ loss:      3.08%
    │         │          
    │         └── RESPONSE CODES
    │             ├─ noerr:     1163
    │             ├─ corrupted: 0
    │             ├─ formaerr:  0
    │             ├─ servfail:  0
    │             └─ errors:    0.00%
    │                   
    └─ [2a0d:b600:166:ff00:e2d5:5eff:fe3a:a880]:1337
         ├─── UDP
         │    ├── NETWORK
         │    │   ├─ queries:   1200
         │    │   ├─ responses: 770
         │    │   └─ loss:      35.83%
         │    │          
         │    └── RESPONSE CODES
         │        ├─ noerr:     770
         │        ├─ corrupted: 0
         │        ├─ formaerr:  0
         │        ├─ servfail:  0
         │        └─ errors:    0.00%
         │               
         └─── TCP
              ├── NETWORK
              │   ├─ queries:   1200
              │   ├─ responses: 653
              │   └─ loss:      45.58%
              │          
              └── RESPONSE CODES
                  ├─ noerr:     653
                  ├─ corrupted: 0
                  ├─ formaerr:  0
                  ├─ servfail:  0
                  └─ errors:    0.00%

```

## Where are logs

All logs are stored in syslog. Thus, you can explore them using the next command:

```sh
tail -f /var/log/syslog | grep dnstress
```

## Requirements

These libraries are necessary for building:

- [libevent](https://libevent.org/)

- [libldns](https://nlnetlabs.nl/projects/ldns/about/)

- [libbsd](https://libbsd.freedesktop.org/wiki/)

## Report Problems or Request for New Features

If you find a problem/bug or something, write an issue about this problem. Also, if you think that a feature will be a nice addition to `dnstress`, do the same -- write an issue and we will try to add your requested feature as soon as possible.

## TODO

- [ ] Add an option to control intensity of stressing (for instance, number of requests per second)
- [ ] Add an option for verbose or compressed output
- [ ] Change default path to `dnstress` configuration file
- [ ] Debian package