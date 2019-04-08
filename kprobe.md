## Kernel Dynamic Probes

In this lab you will experiment with tracing kernel probes. `kprobes`, short
for *k*ernel probes, are dynamically instrumented probes that do not have overhead
at disabled and minimal overhead when enabled. The details of the implementation
are a bit hairy, but the overall concept is rather straight forward: when a `kprobe`
is enabled, the kernel dynamically inserts an `INT3` instruction at the specified
address. The original instruction is saved and later single stepped.
When the code is executed and the trap is generated, the `kprobe` interrupt service
routine is run and execution is redirected to the registered `kprobe` handler. Normal
execution flow resumes after the handler.

### Examine available probe points

Modulo a few exceptions, all exported symbols are available to be kprobed. Even better,
most *un*exported symbols can be kprobed as well. We'll first go over how to verify
a symbol is available.

There are two ways to find an appropriate symbol to trace.

* `/proc/kallsyms`
* github.com/torvalds/linux

### github.com/torvalds/linux

Before you trace the kernel, it's probably useful to take a look at the source and
figure out the most appropriate function(s). Exported symbols are usually good
candidates (as they are probably important). Symbols are usually exported with
macros `EXPORT_SYMBOL` and `EXPORT_SYMBOL_GPL`.

Some things to be wary of:
* `static` functions are often inlined by the compiler. Inlined functions are not
  tracked down by `bpftrace` and traceable. If you've installed a `kprobe` and your
  handlers aren't being hit, you might be tracing an inlined function.
* Functions you find in the source might be hidden depending on where in the kernel
  code it's defined.

### `/proc/kallsyms`

`/proc/kallsyms` contains all the base object symbols in the kernel. The best way to
verify if a symbol exists in your running kernel is to just grep `/proc/kallsyms`.
Usually when I'm tracing I'll read through the latest kernel source and then check
in kallsyms if the symbol I want is present.

### Hands on: find and verify a symbol

In `kernel/cgroup/cgroup.c`:

```
/*
* The default hierarchy, reserved for the subsystems that are otherwise
* unattached - it never has more than a single cgroup, and all tasks are
* part of that cgroup.
*/
struct cgroup_root cgrp_dfl_root = { .cgrp.rstat_cpu = &cgrp_dfl_root_rstat_cpu };
EXPORT_SYMBOL_GPL(cgrp_dfl_root);
```

Now check `/proc/kallsyms`:

```
$ grep cgrp_dfl_root /proc/kallsyms
000000000001bfa0 A cgrp_dfl_root_cpu_stat
ffffffff821321a0 r __ksymtab_cgrp_dfl_root
ffffffff8214b9e9 r __kstrtab_cgrp_dfl_root
ffffffff8225fae0 D cgrp_dfl_root
```

Note that the output format is in `nm` format. See `man nm` for more.

### Hands on: See how often the cgroup2 filesystem is being used:

```
$ cat cgroup2.bt
#include <linux/kernfs.h>
#include <linux/fs.h>

kprobe:cgroup_file_open
{
  if (((kernfs_open_file*)arg0)->file->f_inode->i_sb->s_magic == 0x63677270) {
    printf("cgroup2 magic detected\n");
  }
}

$ sudo bpftrace cgroup2.bt
Attaching 1 probe...
cgroup2 magic detected
cgroup2 magic detected
cgroup2 magic detected
cgroup2 magic detected
cgroup2 magic detected
^C

```

---
## Exercises

### (kprobeme)

NOTE: before attempting the tasks in this section select the `kprobes` option from the `bpfhol` menu.

1. `kprobeme` reads a file once a second. Which file is it?
1. Is `kprobeme` opening a new file descriptor every time?
1. Is `kprobeme` leaking file descriptors? (HINT: use an associative map)

---
## Further Reading

* https://github.com/torvalds/linux/blob/master/Documentation/kprobes.txt
* https://www.kernel.org/doc/html/latest/trace/kprobetrace.html
