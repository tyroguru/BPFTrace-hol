## Dynamic User Probes

In this lab you will experiment with probing dynamic *u*ser probes (`uprobes`). These are locations in userland applications and libraries that are made available on-demand by the kernel uprobe subsystem. 

The `uprobe` is very similar to the kernel [kfunc/kprobe](kprobe.pdf) probes that we saw previously. When a `uprobe` is armed, the
kernel will dynamically replace the target address with an `INT3` (0xCC) instruction.
The original instruction is saved into a special part of the process address space and will later be single stepped there. Eventually if the breakpoint (`INT3`)
is hit, the `uprobe` subsystem will get notified and registered callbacks will be run.

With a uprobe, any instruction in a userland application can be traced. However, with great power comes great responsibility: for every traced instruction we must dive into the kernel to execute additional code to satisfy the required tracing scripts and this adds latency into the application being traced. While every effort is made to ensure this cost is kept to a minimum it is worth keeping this in mind when tracing latency sensitive parts of applications.

### uprobe format

The format of a uprobe is:

```
uprobe:<library_name>:[cpp:]<function_name>
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

* The additional `:cpp` probe qualifier allows us to use a demangled form of a C++ symbol. Not only does this mean we don't need to use unwieldy mangled symbols but, more importantly, this acts as a wildcard match for any parameters which is especially convenient matching methods with template parameters.

It isn't currently possible with bpftrace to display C++ type definitions as we can we kernel datatypes as shown previously. 

## Exercises

1. Using the bpfhol `uprobes` workload generator that we referenced above,  dump the arguments being passed into the `AddressBook::AddContact` function.
2. Write a script to sum the size of strings being passed in for each entry in the Address Book.


### Hands on: watch voluntary exit codes

Run on your devserver:

```
$ bpftrace -e 'uprobe:/lib64/libc.so.6:exit
{
    printf("%s exited with code %d\n", comm, arg0);
}'
```

There might be a lot of output, but if you open an extra shell and run
`exit 1234`, you should be able to see it (or maybe pipe the output to `grep`).

*C++ Note:* When observing C++ remember that each function is passed the `this` pointer implcitly as the first argument. This means that when dealing with C++ applications we index a functions arguments starting at arg1 and not arg0.


### Hands on: track process lifetimes

This example brings the previous two examples to the next logical step:
tracking process lifetimes. Suppose we wanted to figure out how long on
average particular processes live.

First let's find an appropriate start function to trace:

```
# bpftrace -l 'uprobe:/lib64/libc.so.6:*' | grep _start
uprobe:/lib64/libc.so.6:__libc_start_call_main
uprobe:/lib64/libc.so.6:__libc_start_main
```

It looks like `__libc_start_main` will work for us.

Now let's create a bpftrace script that will track in histograms how long
each process lives for:

```
# cat process_lifetime.bt
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

---

## Exercises

### Track process lifetimes

1. Can you make the histgrams display _seconds_ instead of nanoseconds?

Now we've seen dynamic user probes we move on to [user static probes](usdt.pdf).

---

## Further Reading

* https://lwn.net/Articles/391974/
* https://lwn.net/Articles/543924/
* https://www.kernel.org/doc/html/latest/trace/uprobetracer.html
