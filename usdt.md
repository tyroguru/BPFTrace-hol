## Statically Defined User Probes (USDT)

(... or "User(land) Statically Defined Tracepoints"). The name tells us everything:

1. *User* - these probes are for applications and libraries.
2. *Statically Defined Tracepoints* - The tracepoints (a.k.a the *probes and arguments*) are statically defined by the developer apriori via macros. You as a developer decide where to place probes in your code and these can then be *dynamically* enabled via bpftrace scripts when the code is executing. A huge advantage of this approach is that the probe can be given a name with specific semantics: for example, you can name a probe `transaction-start` and use this name to enable it instead of having to know a mangled C++ function name! This gives us stability and consistency in probe nomenclature.

Be aware that you may also see USDT probes referred to as *markers*.

In this lab you will experiment with tracing static user probes.

---
### Probe discovery

USDT probes are statically defined within applications and libraries and there are several ways of discovering probes that are available for enabling.

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
# /tmp/bpftrace -l /lib64/libc-2.17.so
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

### Probe components

The format of a USDT probe is:

```
usdt:binary_or_library_path:[probe_namespace]:probe_name
```

Where:

* `usdt`: Provider name (currently fixed at *usdt*)
* `binary_or_library_path:` If an absolute path is specified then that will be used or the users $PATH will be searched.
* `[probe_namespace]:` optional and will default to the basename of the binary or library path.
* `probe_name:` name of the probe


### Probe arguments

USDT probes don't have to export any arguments but if they do we reference then via the `argX` builtin variables with the first argument being `arg0`, the second, `arg1` and so on.  Discovering the number of arguments a probe exports is possible using the `tplist.py` tool which is part of the `bcc` package (this package is shipped internally as `fb-bcc-tools`):

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

The output from `tplist.py` given above raises an important point about USDT probes: a single probe can be located at multiple places in a code base. For example, if some code has multiple locations where a transaction can be inititiated we may want to define a single probe for this operation, `transaction-start` for example, and we would need to place this probe at each of these call sites. When we subsequently enable the `transaction-start` probe the bpf subsystem would ensure that all locations where this probe is defined are instrumented.

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


Discovering the types of a probes arguments is currently only possible through either documentation or code inspection.

---

### Inserting probes into code

How to insert USDT probes into code isn't specific to bpftrace but as it's such an important area for userland developers we thought it best to include a small section on how to do this.

### Folly SDT (Internal Facebook Way!)

Facebook developers should use Folly to declare static USDT probes. To declare a probe you simply need to use the `FOLLY_SDT` macro:

```
  FOLLY_SDT(provider, name, arg1, arg2, ...)
```

Yes, it's just one macro regardless of the number of arguments! Simply `#include <folly/tracing/StaticTracepoint.h>` and in your buck TARGETS file include a dependency for `//folly/tracing:static_tracepoint`.

A quick example from the hiphop web server code (`hphp/runtime/vm/runtime.cpp`). Several functions are defined to concatonate strings, each taking a different number of strings as arguments. Each function exports the `hhvm:hhvm_cow_concat` probe which exports two integer arguments. For example, from `concat_s3()`:

```
    FOLLY_SDT(hhvm, hhvm_cow_concat, s1.size(), s2.size() + s3.size());
```

With hhvm runing on my devserver:

```
[root@devvm1362.cln1 /root] /tmp/bpftrace -l '*hhvm_cow_concat*' -p 789965
usdt:/data/users/jonhaslam/fbsource/fbcode/buck-out/dbgo/gen/hphp/hhvm/hhvm/hhvm:hhvm:hhvm_cow_concat
```

XXX Finish this!



#### IS_ENABLED probes

USDT probes are relatively inexpensive in terms of cpu cycles and extra instructions but they are not free. Sometimes the cost of preparing arguments may be costly especially if the probe is in a latency sensitive area. If this is the case then FOLLY provides a mechanism which allows us to only do the expensive argument preparation of the probe is actually enabled. To do this a semaphore is associated with a probe and the semaphore is incremented on probe enabling and decremented on probe disabling.

Firstly, in global scope declare the probe that has a semaphore associated with it:

```
  FOLLY_SDT_DEFINE_SEMAPHORE(provider, probe)
```

Then when we want to do the costly argument preparation we first check if the probe is enable:

```
  FOLLY_SDT_IS_ENABLED(provider, name)
```

and finally call:

```
 FOLLY_SDT_WITH_SEMAPHORE(provider, name, arg1, arg2, ...);
```

So, a trivial example would to use the `mapper:large_map` probe in this way would be:

```
  FOLLY_SDT_DEFINE_SEMAPHORE(mapper, large_map);

  int map_large_region(struct context* ctx, uint64_t pgsz, uint32_t numpgs)
  {
    if (FOLLY_SDT_IS_ENABLED(mapper, large_map))
    {
      /* The call to get_num_physical_pages() is very expensive */
      int phys_pages = get_num_physical_pages(ctx, pgsz * numpgs);
      FOLLY_SDT_WITH_SEMAPHORE(mapper, large_map, pgsz * numpgs, phys_pages);
    }
    /* rest of code */
```

### Additional Facebook related note:

If an application has been converted to use large pages for its text region then this causes USDT probes (and uprobes) to not work (see task T22479091). The only workaround for this at the minute is to explictly exclude functions that contain USDT probes from being include in the large page relocation process. We do this by using the `NEVER_HUGIFY()` macro to specifiy that a function should never be relocated to a large page. Obviously you will need to consider whether you want to sacrific performance for observability here.


### Non-Folly way (External to Facebook!)

The macros a developer uses to place static probes in their code are defined in `/usr/include/sys/sdt.h`. The macros are simple to use and can be used within C++, C or assembler. the format is simply:

```
  STAP_PROBEn(provider, probename, arg0, ..., argn-1);
```


A trivial example from the glibc source base (`glibc-2.26/malloc/__libc_mallopt()`):


```
  LIBC_PROBE (memory_mallopt, 2, param_number, value);

  switch (param_number)
    {
    case M_MXFAST:
      if (value >= 0 && value <= MAX_FAST_SIZE)
        {
          LIBC_PROBE (memory_mallopt_mxfast, 2, value, get_max_fast ());
          set_max_fast (value);
        }
      else
        res = 0;
      break;


```
where the glibc macro `LIBC_PROBE` is defined in terms of the `STAP_PROBE` macro:

```
# define LIBC_PROBE(name, n, ...) \
  LIBC_PROBE_1 (MODULE_NAME, name, n, ## __VA_ARGS__)

# define LIBC_PROBE_1(lib, name, n, ...) \
  STAP_PROBE##n (lib, name, ## __VA_ARGS__)
```

And therefore we have the two probes defined:

output

---

## A comparison of USDT and uprobes (Advantages / Disadvantages)

Let's summarise the advantages and disadvantages of USDT probes and uprobes:

### USDT Advantages

1. USDT provides a more stable naming and argument scheme. With uprobes a probe consumer as at the mercy of both the compiler toolchain and the developer. Functions may be renamed as is the case with some implementations of large pages and function signatures may change, either through mangling or function arguments changing. Although Linux USDT currently offers no scheme to enforce stability contracts on USDT probes the mere presence of a probe macro makes code maintainers think about the probe and its arguments.

1. No inlining. A big problem with uprobes is that it can only instrument functions at the entry and return sites and therefore if the compiler inlines a function it is no longer available to instrument. As inlining is used extremely aggressively in optimised code for performance this is a very common problem wih uprobes. USDT with its explicit probe sites is not open to this issue.

1. With USDT we can place probes at any place in the code, not just entry and return sites.

1. We can export whatever data we want and are not limited to function arguments. This is a massive advantage especially without a full typing system available to us (it's virtually impossible to manipulate members of C++ classes passed as arguments at the minute). With USDT probes though we can access whatever data we want in a function to provide it as a probe argument and even  call functions to prepare arguments!


### USDT Disadvantages

1. Performance. As we've seen, USDT probes introduce more instructions into the instruction stream. Arguments have to be placed in registers and instrumentation instructions have to be inserted. We also have to keep record metadata about probes and their arguments into the ELF binary. This additions are very minor but need to be kept in mind.

1. USDT probes are static and therefore require judicious developer insight for placement and arguments.


## Exercises

NOTE: before attempting the tasks in this section select the `usdt` option from the `bpfhol` menu.

### (userpasswd)

1. What probe is the `usdt-passwd` program exporting?
1. How many arguments does it provide?
1. All of the probes arguments are character pointers: the first argument is the username of interest and the second is the home directory of that user. What are they?
1. *XXX* How about getting them to add a second probe?
1. *XXX* How about getting them to add a second `passwdfound` probe and looking at the locations?

### thrift example

---

## Further Reading

## Instrumentation Methodology

When a DTRACE_PROBE/STAP_PROBE macro is used to insert a probe into code we end up with a sequence of instructions to setup arguments for the probe follwed by a 'nop' instruction. The 'nop' instruction is the single byte variant and this is a placeholder for where to patch when the probe is enabled (remember the point of the instrumentation is to vector us off into the kernel so that we can enter into the BPF VM).

Take this extremely simple example of a probe with a single argument:

```
#include <sys/sdt.h>
#include <sys/time.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  struct timeval tv;

  while (1)
  {
    gettimeofday(&tv, NULL);
    DTRACE_PROBE1(jons-app, bigprobe, tv.tv_sec);
    sleep(1);
  }
  return 0;
}

[jonhaslam@devvm1362.cln1 ~/BPF/USDT] clang++ -g -O3 -o example example.cc

[jonhaslam@devvm1362.cln1 ~/BPF/USDT] readelf -n ./example

Displaying notes found at file offset 0x00000254 with length 0x00000020:
  Owner                 Data size       Description
  GNU                  0x00000010       NT_GNU_ABI_TAG (ABI version tag)
    OS: Linux, ABI: 2.6.32

Displaying notes found at file offset 0x0000113c with length 0x0000004c:
  Owner                 Data size       Description
  stapsdt              0x00000035       NT_STAPSDT (SystemTap probe descriptors)
    Provider: jons-app
    Name: bigprobe
    Location: 0x0000000000400584, Base: 0x0000000000400630, Semaphore: 0x0000000000000000
    Arguments: -8@8(%rsp)
```

If we disassemble the instructions we can see which are related to our tracing:

```
[jonhaslam@devvm1362.cln1 ~/BPF/USDT] mygdb ./example -q
Reading symbols from ./example...
(No debugging symbols found in ./example)
(gdb) disas main
Dump of assembler code for function main:
   0x0000000000400560 <+0>:     push   %rbx
   0x0000000000400561 <+1>:     sub    $0x20,%rsp
   0x0000000000400565 <+5>:     lea    0x10(%rsp),%rbx
   0x000000000040056a <+10>:    nopw   0x0(%rax,%rax,1)
   0x0000000000400570 <+16>:    xor    %esi,%esi
   0x0000000000400572 <+18>:    mov    %rbx,%rdi
   0x0000000000400575 <+21>:    callq  0x400440 <gettimeofday@plt>
   /* Load up the 'tv' pointer */
   0x000000000040057a <+26>:    mov    0x10(%rsp),%rax
   /* Load tv->tv_sec onto the stack */
   0x000000000040057f <+31>:    mov    %rax,0x8(%rsp)
   /* This is our patch slot */
   0x0000000000400584 <+36>:    nop
   0x0000000000400585 <+37>:    mov    $0x1,%edi
   0x000000000040058a <+42>:    callq  0x400460 <sleep@plt>
   0x000000000040058f <+47>:    jmp    0x400570 <main+16>
End of assembler dump.
```

If we enable this probe we can see that the 'nop' instruction at `0x0000000000400584` has been patched dynamically to be an `int3` instruction which will cause us to trap into the kernel.

```
(gdb) disas main
Dump of assembler code for function main(int, char**):
   0x0000000000400560 <+0>:     push   %rbx
   0x0000000000400561 <+1>:     sub    $0x20,%rsp
   0x0000000000400565 <+5>:     lea    0x10(%rsp),%rbx
   0x000000000040056a <+10>:    nopw   0x0(%rax,%rax,1)
   0x0000000000400570 <+16>:    xor    %esi,%esi
   0x0000000000400572 <+18>:    mov    %rbx,%rdi
   0x0000000000400575 <+21>:    callq  0x400440 <gettimeofday@plt>
   0x000000000040057a <+26>:    mov    0x10(%rsp),%rax
   0x000000000040057f <+31>:    mov    %rax,0x8(%rsp)
   0x0000000000400584 <+36>:    int3            <---- !!!
   0x0000000000400585 <+37>:    mov    $0x1,%edi
   0x000000000040058a <+42>:    callq  0x400460 <sleep@plt>
   0x000000000040058f <+47>:    jmp    0x400570 <main(int, char**)+16>
End of assembler dump.
```

The `int3` instruction causes the application to trap into the kernel where the trap handler code vectors us off into the BPF virtual machine (XXX Reference / MORE).


