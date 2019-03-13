# BPFTrace Hands-On Labs

This repo contains hands-on labs to help an engineer get up to speed with the BPFTrace utility and framework. Currently  this material should work on a FB dev server but no ideas about other platforms.

[BPFTrace](https://github.com/iovisor/bpftrace) is an OSS project that provides a high level tracing language interface on top of the Enhanced Berkely Packet Filter (eBPF) framework. It was initially developed by Alistair Robertson and now has a flourishing developer community on github.

---

### Prerequisites

1. Get yourself a dev server.
1. Unless specified differently, all commands will be executed as the `root` user so ensure you have access to that account.
1. Install the `fb-bpftrace` package:

```
[root@twshared6749.09.cln1 ~]# yum install -q -y fb-bpftrace
[root@twshared6749.09.cln1 ~]# rpmquery  fb-bpftrace
fb-bpftrace-20190308-171433.x86_64
```


---

### Labs

1. [Working with system calls](syscalls.md)

---
