## System Call Tracing

In this lab you will experiment with tracing system call interfaces. As this is the primary mechanism with which a process interacts with the kernel it can often be of interest when investigating systemic and/or application behavior.

The number of system calls may vary from kernel to kernel and we can see which exist on our system with `bpftrace -l`:

```
# bpftrace -l tracepoint:syscalls:sys_enter*
tracepoint:syscalls:sys_enter_socket
tracepoint:syscalls:sys_enter_socketpair
tracepoint:syscalls:sys_enter_bind
tracepoint:syscalls:sys_enter_listen
tracepoint:syscalls:sys_enter_accept4
<elided>
```

You should have in excess of 300 system calls to choose from!

---

### Syscall probe naming format

Each syscall has two distinct probes: an **enter** probe which is fired when a system call is called and an **exit** probe which is fired on return from a system call. The format of the two different types of syscall probe are:

```
tracepoint:syscalls:sys_enter_{syscall_name}
tracepoint:syscalls:sys_exit_{syscall_name}
```

For example, the `open(2)` system call probes are:

```
tracepoint:syscalls:sys_enter_open
tracepoint:syscalls:sys_exit_open
```

**Question:** Does `t:syscalls:sys_exit_exit` exist? Is so, when will it fire?

---

### Probe arguments

#### Entry probes

The arguments for a system call probe are made available through the builtin `args` structure. For example, according to the man page for write(2) the syscall has 3 arguments: `fd`, `buf` and `count`and we can verify that with the `-lv` options to `bpftrace`.

```
[root@twshared6749.09.cln1 ~]# bpftrace -lv t:syscalls:sys_enter_write
tracepoint:syscalls:sys_enter_write
    int __syscall_nr;
    unsigned int fd;
    const char * buf;
    size_t count;
```

[NOTE: We've used the abbreviated name for `tracepoint` above - simply `t`!]

Those with a keen eye may have noted that we have an extra parameter - `int __syscall_nr`. This is really just an implementation detail that has been exposed to you and you'll probably have little use for it. It's the system call number assigned to this system call in the kernel (more detail in the "Further Reading section"??)

To access an argument we reference it through the `args` array using its name, i.e., `args->buf`. In the following example we capture the first 200 bytes (or less) of any buffer being sent to file descriptor 2 which is usually `stderr`

```
# cat write.bt
t:syscalls:sys_enter_write
/args->fd == 2/
{
        printf("%s: %s\n", comm,  str(args->buf));
}

# BPFTRACE_STRLEN=200 bpftrace ./write.bt
agent-helper: V0313 09:20:08.559548 42665 ServiceHandler.cpp:35] (s3 fb_user 1218794780) Starting job
 job
endencies
V0313 09:20:08.478726 39766 ServiceHandler.cpp:78] (s3 fb_user 100034610238583) Scheduling extra
CPUThreadPool26: V0313 09:20:08.619328 42669 ServiceHandler.cpp:64] (s3 fb_user 100000663391190) Missing 2 dependencies

CPUThreadPool26: E0313 09:20:08.619365 42669 ServiceHandler.cpp:70] (s3 fb_user 100000663391190) Reached max dependency checks (3)
æª^K<85><88>?
agent-helper: V0313 09:20:08.619328 42669 ServiceHandler.cpp:64] (s3 fb_user 100000663391190) Missing 2 dependencies
E0313 09:20:08.619365 42669 ServiceHandler.cpp:70] (s3 fb_user 100000663391190) Reached max depe
CPUThreadPool26: V0313 09:20:08.622006 42669 ServiceHandler.cpp:92] (s3 fb_user 100000663391190) Finished job
À]¨]<v<9c><87>_-Êî)^\Kî^N^A^O<94><97>C<89><8a>¶ì@ñ·<84>¢<9d>W^C£<87>9t^Lûa¢Ìiæ~^Z54w=P¼C+2<88>êï^G^K,^XË^B^A^B^A^A^B^F
^B
agent-helper: V0313 09:20:08.622006 42669 ServiceHandler.cpp:92] (s3 fb_user 100000663391190) Finished job
endencies
```

A few things to note from the above example:

- Owing to a current architectural limit in BPF, BPFTrace restricts strings to be 64 bytes by default. We can increase these using the `BPFTRACE_STRLEN` environment variable to a maximum of 200 bytes.
- `char *`'s must be explicitly converted to strings using the `str()` builtin.
- The 'comm' builtin gives us the name of the process doing the write call.
- The output of multiple threads is interleaved. **Question**: can you think of another way of writing the script to obtain non-interleaved output? (HINT: it's a very simple modification!).

#### Return probes

As with any C function we only have a single return code from a syscall. As an exercise, compare the return codes specified in the man pages with the output of `bpftrace -lv` for the following syscalls exit probes:

- `close`
- `mmap`
- `write`

As you can see, the types don't agree as bpftrace always reports the return type as 'long' which is an 8 byte quantity. We may need to cast the return value accordingly to the correct return type as specified by the man page.

[ XXX On an error the errno appears to be returned and not -1 - explain that and expand ]

### Raw vs "Normal" syscalls
---

## Exercises

NOTE: before attempting the tasks in this section select the `syscalls` option from the `bpfhol` menu.

#### `mmap(2)`

1. Locate the process doing the most mmap calls in a 30 second period (it should be obvious which one I'm on about :-) ).
1. What are the sizes of the segments being created?
1. Can you tell what percentage of the created mappings are private to the process and which are shared?

#### `open(2)`

1. Write a script to show which files are being opened.
1. Extend that script to show which processes are opening which file.
1. Change that script to only show open calls that are creating temp files (hint: use the `flags` argument).

#### `close(2)`

1. Find all close() calls on invalid file descriptors.


## Further Reading

Each section should have some more advanced reading for those that are interested. It shouldn't be necessary to know this stuff but it may be useful for those who abolutely need to have more information to be able to use something.

- How are syscall probes instrumented?

