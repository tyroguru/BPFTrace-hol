## Dynamic User Probes

XXX use the glibc USDT's we now have. https://fb.workplace.com/groups/fbcode/permalink/5049402751763190/
In this lab you will experiment with probing dynamic *u*ser probes. These are locations in userland applications and libraries that are made available on-demand by the kernel uprobe subsystem. 

probes are probes that are not *statically* defined apriori in code (e.g., [USDT
probes](usdt.pdf)). Dynamic probes are discovered on-demand by the kernel uprobe
subsystem. For userland applications and libraries, a point of instrumentation
is called a `uprobe`.

The `uprobe` is very similar to the kernel [kfunc/kprobe](kprobe.pdf) probes that we saw previously. When a `uprobe` is armed, the
kernel will dynamically replace the target address with an `INT3` (0xCC) instruction.
The original instruction is saved into a special part of the process address space and will later be single stepped there. Eventually if the breakpoint (`INT3`)
is hit, the `uprobe` subsystem will get notified and registered callbacks will be run.

With a uprobe, any instruction in a userland application can be traced. However, with great power comes great responsibility: for every traced instruction we must dive into the kernel to execute additional code to satisfy the required tracing scripts and this adds latency into the application being traced. While every effort is made to ensure this cost is kept to a minimum it is worth keeping this in mind when tracing latency sensitive parts of applications.

### uprobe format

The format of a uprobe is:

```
'uprobe:[:cpp]:<library_name>:<function_name>
```

The optional `:cpp` component is specific to C++ application tracing and is discussed later. An important concept with uprobes is that the probing is applied to a file (a vnode to be precise) and not specifically to a process. This means that we reference paths to files when specifying a probe to be enabled (e.g `/lib64/libc.so`) and if we want to restrict the probe to a particular process we 

### probe discovery

To list the probe sites that are available we simply specify a path to a given library or executable. For example, all the function sites that can be probed in `/lib64/libc.so`:

```
# bpftrace -l 'uprobe:/lib64/libc.so.6:*'
uprobe:/lib64/libc.so.6:_Exit
uprobe:/lib64/libc.so.6:_Fork
uprobe:/lib64/libc.so.6:_IO_adjust_column
uprobe:/lib64/libc.so.6:_IO_adjust_wcolumn
uprobe:/lib64/libc.so.6:_IO_cleanup
<chop>
```

To list probes available in a running process we can simply specify a path to the `procfs` executable image for that process:

```
# bpftrace -l 'uprobe:/proc/1407556/exe:*'
uprobe:/proc/1407556/exe:_Exit
uprobe:/proc/1407556/exe:_GLOBAL__sub_I.00090_globals_io.cc
uprobe:/proc/1407556/exe:_GLOBAL__sub_I_cxx11_locale_inst.cc
uprobe:/proc/1407556/exe:_GLOBAL__sub_I_cxx11_wlocale_inst.cc
<chop>
```

### uprobe arguments

If DWARF debugging information is available for the target binary you wish to trace then arguments are available via the `args` variable. If debug information is not available then  access to function arguments is via
`arg0`, `arg1`, `...`,  `argN` variables.

Just like `kprobe`s, every `uprobe` comes with a complementary `uretprobe`.
`uretprobe`s fire _after_ the function returns. Because of this, we get access
to the return value in the reserved keyword `retval`.

Let's look at an example of how to probe a C++ application that has DWARF debug information available. Firstly start the `uprobes` workload generator from the `bpfhol` load generator main menu (option 4). Now let's look for some functions with arguments:

```
# pgrep -fl uprobe
1407556 uprobeme
# bpftrace -lv  'uprobe:/proc/2303674/exe:cpp:"AddressBook::AddContact"'
uprobe:/proc/2303674/exe:cpp:"AddressBook::AddContact(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>&)"
    AddressBook * this
    std::string & firstName
    std::string & lastName
    std::string & number
```

Things to note:

* The additional `":cpp` probe qualifier allows us to use a demangled form of a C++ symbol. Not only does this mean we don't need to use unwieldy mangled symbols but, more importantly, this acts as a wildcard match for any parameters which is especially convenient matching methods with template parameters.

It isn't currently possible with bpftrace to display C++ type definitions as we can we kernel datatypes as shown previously. 

## Exercises

1. Using the bpfhol `uprobes` workload generator that we referenced above,  dump the arguments being passed into the `AddressBook::AddContact` function.
2. Write a script to sum the size of strings being passed in for each entry in the Address Book.


```

### Hands on: watch voluntary exit codes

Run on your devserver:
```
$ bpftrace -e 'uprobe:/lib64/libc-2.17.so:exit
{
    printf("%s exited with code %d\n", comm, arg0);
}'
```

There might be a lot of output, but if you open an extra shell and run
`exit 1234`, you should be able to see it (or maybe pipe the output to `grep`).

*C++ Note:* When observing C++ remember that each function is passed the `this` pointer implcitly as the first argument. This means that when dealing with C++ applications we index a functions arguments starting at arg1 and not arg0.


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

*NOTE:* When tracing C++ applications you must use the *mangled* name of the function in your probe specification. Obviously this can get pretty horrible (especially with templates).

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
uprobe:/lib64/libc.so.6:__libc_start_main
{
  @start_times[pid] = nsecs;
}

uprobe:/lib64/libc.so.6:exit
/@start_times[pid] != 0/
{
  @lifetime_hist[comm] = hist(nsecs - @start_times[pid]);
  delete(@start_times[pid]);
}

END
{
  clear(@start_times);
}
```

Note: it is possible that you may need to adjust the above script to accomodate the libc being linked into your target applications.

This script records the start time in bpftrace-relative nanoseconds for each
pid. Then on process exit, we correlate the end time with the start time.
Finally, we place the data in a per-comm (or executable name) histogram.
The resulting data (in nanoseconds) should be printed to the console.

Let's see what happens when we run the script for ~30 seconds:
```
$ bpftrace process_lifetime.bt
Attaching 3 probes...
^C
@lifetime_hist[dig]:
[8M, 16M)              2 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@lifetime_hist[mkdir]:
[128K, 256K)           2 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@lifetime_hist[awk]:
[512K, 1M)             1 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[1M, 2M)               0 |                                                    |
[2M, 4M)               0 |                                                    |
[4M, 8M)               0 |                                                    |
[8M, 16M)              0 |                                                    |
[16M, 32M)             1 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@lifetime_hist[find]:
[256K, 512K)           2 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@lifetime_hist[grep]:
[256K, 512K)           1 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[512K, 1M)             0 |                                                    |
[1M, 2M)               0 |                                                    |
[2M, 4M)               0 |                                                    |
[4M, 8M)               0 |                                                    |
[8M, 16M)              0 |                                                    |
[16M, 32M)             0 |                                                    |
[32M, 64M)             1 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@lifetime_hist[cut]:
[128K, 256K)           3 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@lifetime_hist[cat]:
[128K, 256K)           2 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[256K, 512K)           1 |@@@@@@@@@@@@@@@@@@@@@@@@@@                          |
```

Now we have some nice adhoc data on process lifetimes. We could easily customize
this script to provide more data.

This example highlights a very important aspect of user probes that can be used to great advantage: we can instrument a library or executable's binary image and we will then fire this probe for every subsequent invocation of the binary image. This provides us with an excellent facility to gain true global insights into all applications that use the instrumented image.

### Hands on: uprobe folly functions

This is a more (than usual) contrived example of `uprobe`ing. Suppose we wanted
to examine how often and from where `folly::EventBase` methods are called
inside `dynolog`.  (`dynolog` monitors various system stats on all FB hosts).
Even more specifically, we're interested in when work is being scheduled.

First, let's double check the symbols we expect to see are there:
```
$ sudo objdump -t /proc/`pidof dynolog`/exe | c++filt | grep folly::EventBase::run
00000000048dd7e0 l     F .text  0000000000000063              unsigned long folly::detail::function::execBig<folly::EventBase::runInEventBaseThreadAndWait(folly::Function<void ()>)::{lambda()#1}>(folly::detail::function::Op, folly::detail::function::Data*, folly::detail::function::Data)
00000000048ddce0 l     F .text  0000000000000105              void folly::detail::function::FunctionTraits<void ()>::callBig<folly::EventBase::runInEventBaseThreadAndWait(folly::Function<void ()>)::{lambda()#1}>(folly::detail::function::Data&)
00000000048e1870 g     F .text  000000000000009e              folly::EventBase::runImmediatelyOrRunInEventBaseThreadAndWait(folly::Function<void ()>)
00000000048df6c0 g     F .text  0000000000000116              folly::EventBase::runInLoop(folly::Function<void ()>, bool)
00000000048e15c0 g     F .text  00000000000000b3              folly::EventBase::runInEventBaseThread(folly::Function<void ()>)
00000000048df5b0 g     F .text  0000000000000109              folly::EventBase::runInLoop(folly::EventBase::LoopCallback*, bool)
00000000048e1680 g     F .text  00000000000001ee              folly::EventBase::runInEventBaseThreadAndWait(folly::Function<void ()>)
00000000048de8e0 g     F .text  00000000000000fa              folly::EventBase::runBeforeLoop(folly::EventBase::LoopCallback*)
<snip>
```

Excellent, it looks like we have stuff available to trace. Note that
`/proc/<PID>/exe` is just a symlink to the actual binary.

Because C++ symbols are usually mangled, we have to be careful to attach to the
_mangled_ symbol names. Rather than spending a lot of time piping `objdump`
output to `c++filt`, copying the pertinent address, searching `objdump` output
for the symbol address, and then using the mangled name in bpftrace, it can be
easier to fuzzy match using wildcards:

```
# cat uprobe_folly.bt
uprobe:/usr/local/bin/dynolog:*folly*EventBase*run*
{
  printf("%s\n", ustack(1));
  @[ustack] = count();
}

# bpftrace uprobe_folly.bt
udo bpftrace uprobe_folly.bt
Attaching 78 probes...
        folly::EventBase::runInLoop(folly::EventBase::LoopCallback*, bool)+0


        folly::EventBase::runInEventBaseThread(folly::Function<void ()>)+0


        folly::EventBase::runInLoop(folly::EventBase::LoopCallback*, bool)+0


        folly::EventBase::runLoopCallbacks()+0


        folly::EventBase::runLoopCallbacks()+0


        folly::EventBase::runLoopCallbacks()+0
<snip>
```

---

## Exercises

### Track process lifetimes

1. Can you make the histgrams display _seconds_ instead of nanoseconds?

### uprobeme

NOTE: before attempting task 2 in this section, select the `uprobes` option from the `bpfhol` menu.

1. Is `uprobeme` statically linked or dynamically linked?
1. `uprobeme` has two functions, `void foo(void)` and `void bar(void)`. How often
   are they called?


Now we've seen dynamic user probes we move on to [user static probes](usdt.pdf).

---

## Further Reading

* https://lwn.net/Articles/391974/
* https://lwn.net/Articles/543924/
* https://www.kernel.org/doc/html/latest/trace/uprobetracer.html
