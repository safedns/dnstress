# dnstress

## General description

`dnstress` is a tool that provides stress tests for dns servers. This tool will help you quickly test perfomance of your dns servers (several in the same time).

## Features

First of all, `dnstress` uses a configuration file to setup internal structures. The default path is `./dnstress.json` (it will be changed in the future). Configuration file itself can be like this:

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

| Field | Description |
| ----- | ----------- |
| dname | domain name string |
| blocked |  |
| type |  |




### Stress test modes

### Several DNS servers

### Workers

### Custom queries

## How to build

## How to use

## Requirements

These libraries are necessary for building:

- [libevent](https://libevent.org/)

- [libldns](https://nlnetlabs.nl/projects/ldns/about/)

- [libbsd](https://libbsd.freedesktop.org/wiki/)

## Report Problems or Request for New Features

## TODO
