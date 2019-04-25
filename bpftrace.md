## The bpftrace language

In this lab we will work through some of the key language features of bpftrace that you will need to be familiar with. It is not an exhaustive treatment of the language but just the key concepts. I refer you to the [BPFTrace Reference Guide](https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md) for details on all language features.

**bpftrace** is a language that is specifically designed for tracing user and kernel software. As its primary purpose is to facilitate observation of software behaviour it provides a number of key language primitives that enable us to gain detailed insights into the real runtime behaviour of the code we write (which is rarely what we think it actually is!). In this section we will look at the key primitives and techniques which enable us to obtain fresh insights.

## Dynamic Tracing - So What!??

A key attribute of bpftrace is its *dynamic* nature. To understand the myriad complexities and nuances of the execution profile of a modern software stack we tend to go through a cyclical process when reasoning about a systems behaviour (sometimes referred to as the 'Virtuous Circle'):

1. Form a hypothesis based upon our current understanding.
1. Gather data / evidence to prove or disprove our hypothesis
1. Analyse data.
1. Rinse and Repeat until satisfaction is achieved.

The problem with the above sequence is that modifying software to generate the trace data and re-running experiments tends to dominate the time. In production it is often impossible install such debug binaries and even on anything but trivial development systems it can be painful to do this. IN addition to this we rarely capture the data that we need the first time around and it often takes many iterations to gather all the data we ned to debug a problem.

bpftrace solves these problems by allowing us to trivially dynamically modify the system to capture arbitrary data without modifying any code. As modifying the system is so easy to do we can very quickly iterate through different hypothesis and gain novel insights about systemic behaviour in very short periods of time.

## Action Blocks

bpftrace scripts are made up of one or more *Action Blocks*. An action block has 3 parts:

* **A probe**: this is a place of interest where we interrupt the executing thread. It can be a location of a function (e.g., strcmp(3)) or it can be an event such as a performance counter overflow event or a timer firing. The key point here is that this is somewhere where we can collect data.
* **An optional predicate** (sometimes called a *filter*). This is a logical condition which allows us to decide if we are interested in recording data for this event. For example, is the current process named 'hhvm' or is the file we are writing to located in `/tmp`.
* **Actions to record data** . Actions are numerous and  capture data for us that we are interested in. This may be recording the contents of a buffer in an associative array (known as *maps* - see below) or simply printing something to stdout.

We'll start with the classic example of looking a system calls (see the syscalls lab for further details XXX - link).

* First let's see what system calls are being made:

```
# bpftrace -e 'tracepoint:syscalls:sys_enter_*{@calls[probe] = count();}'
Attaching 314 probes...

^C
<output elided>
@[tracepoint:syscalls:sys_enter_pread64]: 119249
@[tracepoint:syscalls:sys_enter_munmap]: 152063
@[tracepoint:syscalls:sys_enter_mmap]: 156844
@[tracepoint:syscalls:sys_enter_close]: 302237
@[tracepoint:syscalls:sys_enter_read]: 311111
@[tracepoint:syscalls:sys_enter_futex]: 652906
```

A few things to note:

* We use a wildcard (`*`) to pattern match all the system call entry probes.
* The `@calls[]` declares a special type of associative array (known as *maps* or *aggregations*). We didn't have to name the associative array as we only have one - we could have just used the `@` sign on its own (known as the `anonymous array`).
* We index the `@calls[]` array using the *probe* builtin variable. This expands to the name of the probe that has been fired (e.g., `tracepoint:syscalls:sys_enter_futex`).
* Each entry in an associative array can have one of a number of pre-defined functions associated with it. Here the `count()` function simply increments an associated counter every time we hit the probe and we therefore keep count of the number of times a probe has been hit.

NOTE: maps are a key data structure that you'll use very frequently!

* Now let's iterate using the data we just acquired to drill down and discover who is making those `futex` syscalls!

```
# bpftrace -e 'tracepoint:syscalls:sys_enter_futex{@calls[comm] = count();}'
Attaching 1 probe...
^C
<output elided>
@calls[rs:main Q:Reg]: 2999
@calls[cfgator-sub]: 3866
@calls[LadDxChannel]: 5869
@calls[FS_DSSHander_GC]: 7586
```

### Associative arrays

