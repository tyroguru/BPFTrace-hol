# bpftrace Hands-On Labs

[bpftrace](https://github.com/iovisor/bpftrace) is an OSS project that provides a high level tracing language interface on top of the Enhanced Berkely Packet Filter (eBPF) framework. It was initially developed by Alistair Robertson and now has a flourishing developer community on github.

This hands-on lab is designed to be completed in half a day though that may vary depending upon how thorough you want to be with the exercises. It has been written as a kind of "hands-on bootstrapping" guide for the bpftrace beginner and aims to give you enough information to kickstart your productivity with bpftrace. Note that it is by no means a complete treatment of the subject and many things are missed out for the sake of brevity.

We suggest that you complete the labs in the order they are presented below. The sections in each lab generally contain a mix of presented information and suggested exercises. We strongly suggest that you manually run any bpftrace scripts that are used in explanations and feel free to modify them and see what happens!

Finally, note that the lab is designed to be ran stand alone and without a lecturer but it is probably at its best when undertook alongside the short presentations that accompany it (assuming your lecturer doesn't suck I guess...).


---

### Prerequisites

1. Get yourself a dev server.
1. Unless specified differently, all commands will be executed as the `root` user so ensure you have access to that account.
1. Install the `fb-bpftrace` package (it should already be on devservers):

```
[root@twshared6749.09.cln1 ~]# yum install -q -y fb-bpftrace
[root@twshared6749.09.cln1 ~]# rpmquery fb-bpftrace
fb-bpftrace-20190308-171433.x86_64
```


---

### Labs

1. [bpftrace: core language features](bpftrace.pdf)
1. [Working with system calls](syscalls.pdf)
1. [Working with kernel probes](kprobe.pdf)
1. [Working with dynamic user probes](uprobe.pdf)
1. [Working with static user probes](usdt.pdf)
1. [Working with performance counters](perfcnt.pdf)
