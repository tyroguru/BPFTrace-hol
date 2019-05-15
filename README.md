# bpftrace Hands-On Labs

[bpftrace](https://github.com/iovisor/bpftrace) is an OSS project that provides a high level tracing language interface on top of the Enhanced Berkely Packet Filter (eBPF) framework. It was initially developed by Alistair Robertson and now has a flourishing developer community on github.

This hands-on lab is designed to be completed in half a day though that may vary depending upon how thorough you want to be with the exercises. It has been written as a kind of "hands-on bootstrapping" guide for the bpftrace beginner and aims to give you enough information to kickstart your productivity with bpftrace. Note that it is by no means a complete treatment of the subject and many things are missed out for the sake of brevity.

We suggest that you complete the labs in the order they are presented below. The sections in each lab generally contain a mix of presented information and suggested exercises. We strongly suggest that you manually run any bpftrace scripts that are used in explanations and feel free to modify them and see what happens!

Finally, note that the lab is designed to be ran stand alone and without a lecturer but it is probably at its best when undertook alongside the short presentations that accompany it (assuming your lecturer doesn't suck I guess...).


---

### Prerequisites

1. Currently this lab can only be ran on a Facebook devserver so make sure you've bought yours to the party.
1. Unless specified differently, all commands will be executed as the `root` user so ensure you have access to that account.
1.The 'fb-bpftrace' package should already be installed on your devserver. To verify this execute the following command:

```
# rpmquery fb-bpftrace
```

If the above `rpmquery` command returns `package fb-bpftrace is not installed` then manually install the package:

```
# yum install -q -y fb-bpftrace
```

1. Copy `bpftrace-hol.tar.gz` to your dev server. To copy the compressed archive from your laptop to your devserver:

```
scp bpftrace-hol.tar.gz <unixname>@<hostname>:/home/<unixname>
```

1. Upack the archive:

```
# cd ~
# tar zxvf bpftrace-hol.tar.gz
bpftrace-hol/
bpftrace-hol/load_generators/
bpftrace-hol/load_generators/kprobes/
bpftrace-hol/load_generators/kprobes/kprobeme
bpftrace-hol/load_generators/syscalls/
bpftrace-hol/load_generators/syscalls/closeall
bpftrace-hol/load_generators/syscalls/tempfiles
bpftrace-hol/load_generators/syscalls/mapit
bpftrace-hol/load_generators/uprobes/
bpftrace-hol/load_generators/uprobes/uprobeme
bpftrace-hol/load_generators/usdt/
bpftrace-hol/load_generators/usdt/thrift/
bpftrace-hol/load_generators/usdt/thrift/cpp2/
bpftrace-hol/load_generators/usdt/thrift/if/
bpftrace-hol/load_generators/usdt/usdt-passwd
bpftrace-hol/load_generators/utils/
bpftrace-hol/load_generators/utils/allprobes.py
bpftrace-hol/bpfhol
bpftrace-hol/bpftrace.pdf
bpftrace-hol/kprobe.pdf
bpftrace-hol/README.pdf
bpftrace-hol/syscalls.pdf
bpftrace-hol/uprobe.pdf
bpftrace-hol/usdt.pdf

# cd bpftrace-hol
```

1. You're good to go!

*NOTE:*  a number of exercises in the lab will make use of the `bpfhol` binary that is located at the top level directory of the `bpftrace-hol` package install. Its purpose if to run load generators in the background that aid in demostrating features of the bpftrace language. When executed, the `bpfhol` binary presents the following menu:

```
1. syscalls
2. kprobes
3. usdt
4. uprobes
8. stop current generator
9. exit
```

Simply select the integer value corresponding to the area you have been told to run load generators for, e.g., `1` for syscalls, `2` for kprobes etc. .If at any time you're not sure whether you already have load generators running you can simply select option `8` to kill all exisiting load generators that may be running.

---

Also, some of the lab exercises will give you a small hint as to what bpftrace language primitive to use to solve them. If this is the case then it is generallyexpected that you will look up the language feature in the [online reference guide](https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md).

### Labs

1. [bpftrace: core language features](bpftrace.pdf)
1. [Working with system calls](syscalls.pdf)
1. [Working with kernel probes](kprobe.pdf)
1. [Working with dynamic user probes](uprobe.pdf)
1. [Working with static user probes](usdt.pdf)
1. [Working with performance counters](perfcnt.pdf)
1. [Solutions to lab exercises](solutions.pdf)
