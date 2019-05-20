## Kernel Dynamic Probes

In this lab you will experiment with tracing kernel probes. `kprobes`, short
for *k*ernel probes, are dynamically instrumented probes that do not have overhead
when disabled and minimal overhead when enabled. The details of the implementation
are a bit hairy, but the overall concept is rather straight forward: when a `kprobe`
is enabled, the kernel dynamically inserts an `INT3` instruction at the specified
address. The original instruction is saved and later single stepped.
When the code is executed and the trap is generated, the `kprobe` interrupt service
routine is run and execution is redirected to the registered `kprobe` handler. Normal
execution flow resumes after the handler.

### Examine available probe points

Modulo a few exceptions, most symbols are available to be `kprobe`d. We'll first
go over how to verify a symbol is available.

There are two ways to find an appropriate symbol to trace.

* [https://github.com/torvalds/linux](https://github.com/torvalds/linux)
* `bpftrace -l kprobe:*`

### https://github.com/torvalds/linux

Before you trace the kernel, it's probably useful to take a look at the source and
figure out the most appropriate function(s). Exported symbols are usually good
candidates (as they are probably important). Symbols are usually exported with
macros `EXPORT_SYMBOL` and `EXPORT_SYMBOL_GPL`.

### bpftrace -l

`bpftrace -l` lists all available probes, `kprobe`s included, where a
probe is an instrumentation point for capturing event data. `bpftrace -l`
takes an optional search parameter. Wildcarded search parameters are supported.
To filter just kprobes, use `kprobe:*` as the search parameter.

Some things to be wary of:
* `static` functions may be inlined by the compiler. Inlined functions are not
  tracked down by `bpftrace` and traceable. If you've installed a `kprobe` and your
  handlers aren't being hit, you might be tracing an inlined function. Inlined
  functions may appear in `bpftrace -l` listing.

### kprobe/kretprobe and their arguments

A key feature of `kprobe`s is that we get access to the arguments via reserved
keywords `arg0`, `arg1`, `...`,  `argN`. For example, the kernel function `vfs_write` is defined as:

```
ssize_t vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
```

and therefore the arguments are available to us as:

```
  arg0 - struct file*
  arg1 - const char*
  arg2 - ssize_t count
  arg3 - loff_t *pos
```

However, a big caveat with accessing parameters is that the Linux kernel doesn't currently have any type information exposed to BPF that bpftrace can use. Therefore we must `#include` the appropriate header files to access the required definitions. For example, to access a `file*` typed argument we need to `#include linux/fs.h> to obtain the structure definition. This should hopefully be fixed in the longer term with the [BTF project](https://facebookmicrosites.github.io/bpf/blog/2018/11/14/btf-enhancement.html) which aims to put type information in the kernel that is always accessible.


Although not listed in `bpftrace -l`, every `kprobe` has a corresponding `kretprobe`.
`kretprobes` are exactly the same as `kprobe`s with the difference being `kretprobe`s
are fired _after_ a function returns. Because of this, we get access to the return
value in reserved keyword `retval`.

### Hands on: find and verify a symbol

In `kernel/cgroup/cgroup.c`:

```
/* subsystems visibly enabled on a cgroup */
static u16 cgroup_control(struct cgroup *cgrp)
{
        struct cgroup *parent = cgroup_parent(cgrp);
        u16 root_ss_mask = cgrp->root->subsys_mask;
        ...
}
```

Now check `bpftrace -l`:

```
# bpftrace -l | grep cgroup_control
kprobe:cgroup_control
kprobe:cgroup_controllers_show
```

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

# bpftrace cgroup2.bt
Attaching 1 probe...
cgroup2 magic detected
cgroup2 magic detected
cgroup2 magic detected
cgroup2 magic detected
cgroup2 magic detected
^C

```

### Hands on: navigating kernel structs

First install the dwarves tools:
```
# yum install dwarves
```

In the previous exercise, we reached deep inside a bunch of structs. It might seem
hard to discover the right members, but there are fairly structured approaches you
can take.

Take for example the previous function, `cgroup_file_open`:
```
static int cgroup_file_open(struct kernfs_open_file *of)
{
        struct cftype *cft = of->kn->priv;

        if (cft->open)
                return cft->open(of);
        return 0;
}
```

We can discover the members of `struct kernfs_open_file` like so:
```
$ pahole -C kernfs_open_file
struct kernfs_open_file {
        struct kernfs_node * kn;                         /*     0     8 */
        struct file        * file;                       /*     8     8 */
        struct seq_file    * seq_file;                   /*    16     8 */
        void *                     priv;                 /*    24     8 */
        struct mutex       mutex;                        /*    32    32 */
        /* --- cacheline 1 boundary (64 bytes) --- */
        struct mutex       prealloc_mutex;               /*    64    32 */
        int                        event;                /*    96     4 */
...
```

To explore deeper into `struct file`:
```
$ pahole -C file
struct file {
        union {
                struct llist_node fu_llist;              /*     0     8 */
                struct callback_head fu_rcuhead __attribute__((__aligned__(8))); /*     0    16 */
        } f_u __attribute__((__aligned__(8)));                                           /*     0    16 */
        struct path        f_path;                       /*    16    16 */
        struct inode       * f_inode;                    /*    32     8 */
...
```

And so forth. Note that `pahole` might not work out-of-the-box on non-devservers.
It appears `pahole` needs a non-compressed linux image (ie `/boot/vmlinux-*` vs
`/boot/vmlinuz-*`) to look up symbols.

---
## Exercises

### Monitoring cgroup2 fs

1. Extend `cgroup2.bt` to record, over the duration of a bpftrace program, which
   processes accessed which control files and how many times.

### kprobeme

NOTE: before attempting the tasks in this section select the `kprobes` option from the `bpfhol` menu.

1. `kprobeme` `open(2)`s and `read(2)`s a file once a second. Which file is it?
1. Is `kprobeme` opening a new file descriptor every time?
1. Is `kprobeme` leaking file descriptors? (HINT: use an associative map)


After this excursion in to the kernel we're going back up into userland with [uprobes](uprobe.pdf).

---
## Further Reading

* https://github.com/torvalds/linux/blob/master/Documentation/kprobes.txt
* https://www.kernel.org/doc/html/latest/trace/kprobetrace.html
* https://www.kernel.org/doc/ols/2007/ols2007v2-pages-35-44.pdf
