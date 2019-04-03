## Statically Defined User Probes (USDT)

(... or "Userland Statically Defined Tracepoints"). The name tells us everything:

1. *Userland* - these probes are for applications and libraries.
2. *Statically Defined Tracepoints* - The tracepoints (a.k.a the *probes and arguments*) are statically defined by the developer apriori via macros. You as a developer decide where to place probes in your code and these can then be *dynamically* enabled via bpftrace scripts when the code is executing.

In this lab you will experiment with tracing static user probes.

---
### Probe discovery

USDT probes are defined withon applications and libraries and there are several ways of discovering probes that are available for enabling.o

Using `bpftrace` for a running process:

```
# /tmp/bpftrace -l -p `pgrep dyno`| grep usdt
usdt:/usr/local/bin/dynolog:folly:request_context_switch_before
usdt:/usr/local/bin/dynolog:thrift:crochet_request_data_context
usdt:/usr/local/bin/dynolog:thrift:strobelight_probe_data_destruct
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_pre_write
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_on_write_data
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_post_write
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_pre_read
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_on_read_data
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_post_read
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_handler_error
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_handler_error_wrapped
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_user_exception
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_user_exception_wrapped
usdt:/usr/local/bin/dynolog:thrift:thrift_context_stack_async_complete
```

*NOTE: change the above example to bot grep when dale fixes that *

Using `bpftrace` on an executable or library:

```
[root@devvm1362.cln1 /data/users/jonhaslam/BPFTrace-hol] /tmp/bpftrace -l /lib64/libc-2.17.so
usdt:/lib64/libc-2.17.so:libc:setjmp
usdt:/lib64/libc-2.17.so:libc:longjmp
usdt:/lib64/libc-2.17.so:libc:longjmp_target
usdt:/lib64/libc-2.17.so:libc:memory_heap_new
usdt:/lib64/libc-2.17.so:libc:memory_sbrk_less
usdt:/lib64/libc-2.17.so:libc:memory_arena_reuse_free_list
usdt:/lib64/libc-2.17.so:libc:memory_arena_reuse_wait
usdt:/lib64/libc-2.17.so:libc:memory_arena_reuse
<elided>
```

### Probe arguments

USDT probes don't have to export any arguments but if they do we reference then via the `argX` builtin variables with the first argument being `arg0`, the second, `arg1` and so on.  Discovering the number of of arguments a probe exports is possible using the `tplist.py` tool which is part of the `bcc` package:

```
# tplist.py -v -l /lib64/libpthread.so.0
libpthread:lll_futex_wake [sema 0x0]
  12 location(s)
  3 argument(s)
libpthread:pthread_start [sema 0x0]
  1 location(s)
  3 argument(s)
libpthread:pthread_create [sema 0x0]
  1 location(s)
  4 argument(s)
<chop>
```

(*NOTE:* The number of arguments can also be discovered using `readelf -n` but the information isn't pretty printed as nicely. Give it a go!)

The output from `tplist.py` given above raises an important point about USDT probes: a single probe can be located at multiple places in a code base. For example, if some code has multiple locations where a transaction can be inititiated we may want to define a single probe for this operation, `txn-start` for example, and we would need to place this probe at each of these call sites. When we subsequently enable the `txn-start` probe the bpf subsystem would ensure that all locations where this probe is defined are instrumented.

In the tplist.py example above we can see that the `libpthread:lll_futex_wake` probe has been inserted at 12 different locations! We can use our good friend gdb to tell us exactly where:

```
(gdb) info probes stap libpthread lll_futex_wake
Type Provider   Name           Where              Semaphore Object
stap libpthread lll_futex_wake 0x00007fffee27a174           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27a8e3           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27a916           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27adcb           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27b0da           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27b810           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27b870           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27bfff           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27c5ee           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee27f7fe           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee281ed1           /lib64/libpthread.so.0
stap libpthread lll_futex_wake 0x00007fffee284704           /lib64/libpthread.so.0
```


```


Discovering the types of a probes arguments is currently only possible through either documentation or code inspection


### Probe definitions

- sdt.h
- folly

---
## Exercises

NOTE: before attempting the tasks in this section select the `syscalls` option from the `bpfhol` menu.

### (userpasswd)

1. What probe is the `usdt-passwd` program exporting?
1. How many arguments does it provide?
1. All of the probes arguments are character pointers: the first argument is the username of interest and the second is the home directory of that user. What are they?
1. *XXX* How about getting them to add a second probe?
1. *XXX* How about getting them to add a second `passwdfound` probe and looking at the locations?


---

## Further Reading
