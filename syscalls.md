## System Call Tracing

In this lab you will experiment with tracing system call interfaces. As this is the primary mechanism with which a process interacts with the kernel it can often be of interest when investigating systemic and/or application behavior.

The number of system calls may vary from kernel to kernel and we can see which with `bpftrace -l`:

```
[root@devvm1362.cln1 ~/BPFTrace/HOL-CODE/syscalls] bpftrace -l tracepoint:syscalls:sys_enter*
tracepoint:syscalls:sys_enter_socket
tracepoint:syscalls:sys_enter_socketpair
tracepoint:syscalls:sys_enter_bind
tracepoint:syscalls:sys_enter_listen
tracepoint:syscalls:sys_enter_accept4
```

### Syscall probe naming format

Each syscall has two distinct probes: an **enter** which is fired when a system call in called and an **exit** probe which is fired on return from a system call. The format of the two different types of syscall probe are:

```
tracepoint:syscalls:sys_enter_{syscall_name}
tracepoint:syscalls:sys_exit_{syscall_name}
```

For example, the open(2) system call probes are:

```
tracepoint:syscalls:sys_enter_open
tracepoint:syscalls:sys_exit_open
```

### Probe arguments

#### Entry probes

The arguments for a system call probe are made available through the builtin `args` structure. According to the man page for open(2) the syscall has 3 arguments: `pathname`, `flags` and `mode`and we verify that with the '-lv' options to `bpftrace`.

```
    [root@devvm1362.cln1 ~] bpftrace -lv t:syscalls:sys_enter_open
    tracepoint:syscalls:sys_enter_open
    int __syscall_nr;
    const char * filename;
    int flags;
    umode_t mode;
```

To access an argument we reference it through the `args` array simply using its name, i.e. `args->filename`.

Note that we have an extra parameter - `int __syscall_nr`. This is really just an implementation detail that has been exposed to you and you'll probably have little use for it. It's the system call number assigned to this system call in the kernel (more detail in the "Further Reading section"??)


#### Return probes

As with any C function we only have a single return code from a syscall. As an exercise, compare the return codes specified in the man pages with the output of `bpftrace -lv` for the following syscalls exit probes:

- close
- mmap
- write

As you can see, the types don't agree as bpftrace always reports the return type as 'long' which is an 8 byte quantity. We may need to cast the return value accordingly to the correct return type as specified by the man page.

[ XXX On an error the errno appears to be returned and not -1 - INVESTIGATE ]

### Raw vs "Nornal" syscalls
---

## Exercises

NOTE: before attempting the tasks in this section make sure you execute 'bpfhol syscalls' (which can be found in the <whatever> directory). (I have 3 examples in mind):

#### `mmap(2)`

- Locate the process doing the most mmap calls in a 30 second period (it should be obvious which one I'm on about :-) ).
- What are the sizes of the segments being created?
- Can you tell what percentage of the created mappings are private to the process and which are shared?

#### `open(2)`

- Write a script to show which files are being opened.
- Extend that script to show which processes are opening which file.
- Change that script to only show open calls that are creating temp files (hint: use the `flags` argument).

#### `close(2)`

- Find all close() calls on invalid file descriptors.


## Further Detail

Each section should have some more advanced reading for those that are interested. It shouldn't be necessary to know this stuff but it may be useful for those who abolutely need to have more information to be able to use something.

# How are syscall probes instrumented?

