## Dynamic User Probes

In this lab you will experiment with probing dynamic *u*ser probes. *Dynamic*
probes are probes that are not *statically* defined apriori in code (eg [USDT
probes](usdt.md)). Dynamic probes are discovered on-demand by the kernel uprobe
subsystem. For userland applications and libraries, a point of instrumentation
is called a `uprobe`.

The `uprobe` is similar to [kprobe's](kprobe.md), When a `uprobe` is armed, the
kernel will dynamically replace the target address with an `INT3` (0xCC) instruction.
The original instruction is saved into a special part of the process address
space and will later be single stepped there. Eventually if the breakpoint (`INT3`)
is hit, the `uprobe` subsystem will get notified and registered callbacks will be
run.

Unlike with `kprobe`s, any part of a userland application can be traced. However,
this comes with a little bit of extra work. Userspace must tell the kernel

1. The location of the executable. (More specifically the inode, but the distinction
   is not relevant here).
1. The offset into the executable to breakpoint.

Fortunately, `bpftrace` hides most of this complexity. The end result is the
interface looks extremely similar to `kprobe`.

### Attaching to a uprobe

```
$ sudo bpftrace -e 'uprobe:<library_name>:<function_name> { ... }'
```

Note the extra `<library_name>` between `uprobe` and `<function_name>`. It is
slightly different from the `kprobe` syntax.

### uprobe/uretprobe and their arguments

A key feature of `uprobe`s is that we get access to the arguments via reserved
keywords `arg0`, `arg1`, `...`,  `argN`.

Just like `kprobe`s, every `uprobe` comes with a complementary `uretprobe`.
`uretprobe`s fire _after_ the function returns. Because of this, we get access
to the return value in the reserved keyword `retval`.

### Hands on: watch voluntary exit codes

Run on your devserver:
```
$ sudo bpftrace -e 'uprobe:/lib64/libc-2.17.so:exit { printf("%s exited with code %d\n", comm, arg0); }'
```

There might be a lot of output, but if you open an extra shell and run
`exit 1234`, you should be able to see it.


### Hands on: explore symbols to trace

Sometimes you're not sure if the code you're viewing in your editor is the
same code that's running on a production box. This can get especially
mind boggling if the codebase churns a lot. In these cases, it can be helpful
to get the ground truth on what symbols exist on your production box.

For example, to confirm the `exit` from the last example exists in libc, run:
```
$ objdump -T /lib64/libc-2.17.so | grep exit
000000000003a0a0 g    DF .text  0000000000000011  GLIBC_2.10  __cxa_at_quick_exit
0000000000039c10 g    DF .text  0000000000000017  GLIBC_2.2.5 exit
00000000000c58d0 g    DF .text  0000000000000055  GLIBC_2.2.5 _exit
00000000001365d0 g    DF .text  0000000000000025  GLIBC_2.2.5 svc_exit
000000000003a080 g    DF .text  0000000000000014  GLIBC_2.10  quick_exit
0000000000039e20 g    DF .text  000000000000004f  GLIBC_2.2.5 __cxa_atexit
00000000003c63a4 g    DO .data  0000000000000004  GLIBC_2.2.5 argp_err_exit_status
000000000010c6a0 g    DF .text  000000000000002d  GLIBC_2.2.5 pthread_exit
00000000003c61f8 g    DO .data  0000000000000004  GLIBC_2.2.5 obstack_exit_failure
0000000000039c30  w   DF .text  000000000000004c  GLIBC_2.2.5 on_exit
0000000000115350 g    DF .text  0000000000000002  GLIBC_2.2.5 __cyg_profile_func_exit
```

`objdump` also has a nice C++ demangling feature:
```
$ objdump -t --demangle `which fb-oomd-cpp` | grep updateContext
0000000000595ef0 g     F .text  0000000000000998              Oomd::Oomd::updateContext(std::unordered_set<Oomd::CgroupPath, std::hash<Oomd::CgroupPath>, std::equal_to<Oomd::CgroupPath>, std::allocator<Oomd::CgroupPath> > const&, Oomd::OomdContext&)
00000000005953e0 g     F .text  0000000000000b02              Oomd::Oomd::updateContextCgroup(Oomd::CgroupPath const&, Oomd::OomdContext&)
```

If you're going to trace a dynamically linked library, it can be a nice sanity
check to see what your application is going to link against:
```
$ ldd `which fb-oomd-cpp` 2> /dev/null
        linux-vdso.so.1 =>  (0x00007ffd0c7d7000)
        libm.so.6 => /lib64/libm.so.6 (0x00007efeffa64000)
        libstdc++.so.6 => /lib64/libstdc++.so.6 (0x00007efeff75d000)
        libatomic.so.1 => not found
        librt.so.1 => /lib64/librt.so.1 (0x00007efeff555000)
        libpthread.so.0 => /lib64/libpthread.so.0 (0x00007efeff339000)
        libdl.so.2 => /lib64/libdl.so.2 (0x00007efeff135000)
        libbz2.so.1 => /lib64/libbz2.so.1 (0x00007efefef25000)
        libgcc_s.so.1 => /lib64/libgcc_s.so.1 (0x00007efefed0f000)
        libc.so.6 => /lib64/libc.so.6 (0x00007efefe942000)
        /usr/local/fbcode/platform007/lib/ld.so => /lib64/ld-linux-x86-64.so.2 (0x00007efeffd66000)

$ ls -l /lib64/libc.so.6
lrwxrwxrwx. 1 root root 12 Apr 10  2018 /lib64/libc.so.6 -> libc-2.17.so
```

If your application is statically linked, you can probably find all the symbols
your application used directly in your binary.

### Hands on: track process lifetimes

This example brings the previous two examples to the next logical step:
tracking process lifetimes. Suppose we wanted to figure out how long on
average particular processes live.

First let's find an appropriate start function to trace:
```
$ objdump -T /lib64/libc-2.17.so | grep _start
0000000000000000  w   DO *UND*  0000000000000000  GLIBC_PRIVATE _dl_starting_up
0000000000022350 g    DF .text  00000000000001c0  GLIBC_2.2.5 __libc_start_main
```

It looks like `__libc_start_main` will work for us.

Now let's create a bpftrace script that will track in histograms how long
each process lives for:
```
$ cat process_lifetimes.bt
uprobe:/lib64/libc-2.17.so:__libc_start_main
{
  @start_times[pid] = nsecs;
}

uprobe:/lib64/libc-2.17.so:exit
{
  @lifetime_hist[comm] = hist(nsecs - @start_times[pid]);
}

END
{
  clear(@start_times);
}
```

This script records the start time in bpftrace-relative nanoseconds for each
pid. Then on process exit, we correlate the end time with the start time.
Finally, we place the data in a per-comm (or executable name) histogram.
The resulting data (in nanoseconds) should be printed to the console.

Let's see what happens when we run the script for ~30 seconds:
```
$ sudo bpftrace process_lifetime.bt
Attaching 3 probes...
^C

...
@lifetime_hist[timeout]:
[2M, 4M)               3 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@             |
[4M, 8M)               1 |@@@@@@@@@@@@@                                       |
[8M, 16M)              1 |@@@@@@@@@@@@@                                       |
[16M, 32M)             4 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[32M, 64M)             1 |@@@@@@@@@@@@@                                       |
[64M, 128M)            0 |                                                    |
[128M, 256M)           0 |                                                    |
[256M, 512M)           0 |                                                    |
[512M, 1G)             0 |                                                    |
[1G, 2G)               0 |                                                    |
[2G, 4G)               1 |@@@@@@@@@@@@@                                       |

@lifetime_hist[sh]:
[2M, 4M)               1 |@@@@@                                               |
[4M, 8M)               1 |@@@@@                                               |
[8M, 16M)              0 |                                                    |
[16M, 32M)             1 |@@@@@                                               |
[32M, 64M)             0 |                                                    |
[64M, 128M)            0 |                                                    |
[128M, 256M)           0 |                                                    |
[256M, 512M)           0 |                                                    |
[512M, 1G)             0 |                                                    |
[1G, 2G)               0 |                                                    |
[2G, 4G)              10 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@lifetime_hist[bash]:
[4M, 8M)               1 |@                                                   |
[8M, 16M)              1 |@                                                   |
[16M, 32M)             3 |@@@                                                 |
[32M, 64M)             0 |                                                    |
[64M, 128M)            0 |                                                    |
[128M, 256M)           0 |                                                    |
[256M, 512M)           0 |                                                    |
[512M, 1G)             0 |                                                    |
[1G, 2G)               0 |                                                    |
[2G, 4G)              43 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
...
```

Now we have some nice adhoc data on process lifetimes. We could easily customize
this script to provide more data.

---
## Exercises

### uprobeme

NOTE: before attempting task 2 in this section, select the `uprobes` option from the `bpfhol` menu.

1. Is `uprobeme` statically linked or dynamically linked?
1. `uprobeme` has two functions, `void foo(void)` and `void bar(void)`. How often
   are they called?

---

## Further Reading

* https://lwn.net/Articles/391974/
* https://lwn.net/Articles/543924/
* https://www.kernel.org/doc/html/latest/trace/uprobetracer.html
