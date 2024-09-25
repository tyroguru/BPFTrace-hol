## Kernel Tracing

In this lab you will experiment with tracing the Linux kernel using four different
probe types:

- *kprobe*, short for *k*ernel probes, are dynamically instrumented probes that do
not have overhead when disabled and minimal overhead when enabled. 
- *kfunc*, this provides essentially the same functionality as a 'kprobe'
but with the big advantage of having fully typed arguments using the kernel's
BTF type system. (*kfunc* as a probe name is not to be confused with the BPF
kernel functions known as *kfuncs*. These are regular kernel functions that are
exposed for use by bpf programs (https://docs.kernel.org/bpf/kfuncs.html) . *kfunc*
when used as a name for a probe is a synonym for *fentry* kernel functions. The
probe name of *kfunc* probably originated in bcc but that's not important really for now. 
- *tracepoint*, these are statically defined probe sites in the kernel. Tracepoints
are more stable than dynamic probes both in terms of probe naming but also in there
arguments.
- *rawtracepoint*, these are essentially identical in terms of functionality to a *tracepoint* probe. The only difference is that rawtracepoint offers raw arguments to the tracepoint while tracepoint provide named arguments that are typed.


### Examine available probe points

A significant amount of kernel functions are available to be traced but not all of them. The primary reason why a function may not be available is because it has been inlined by the compiler and, currently, bpftrace cannot expose inlined functions. (Work is currently ongoing to expose inlined functions).

### bpftrace -l

`bpftrace -l` lists all available dynamic and static kernel probes, where a
probe is an instrumentation point for capturing event data. `bpftrace -l`
takes an optional search parameter. Wildcarded search parameters are supported.

For example, to list all available kernel functions beginning with the pattern `vfs_` and displaying their input and return parameter types:

```
# bpftrace -lv 'kfunc:vfs_*'
<chop>
kfunc:vmlinux:vfs_unlink
    struct user_namespace * mnt_userns
    struct inode * dir
    struct dentry * dentry
    struct inode ** delegated_inode
    int retval
kfunc:vmlinux:vfs_ustat
    dev_t dev
    struct kstatfs * sbuf
    int retval
kfunc:vmlinux:vfs_utimes
    const struct path * path
    struct timespec64 * times
    int retval
kfunc:vmlinux:vfs_write
    struct file * file
    const char __attribute__((btf_type_tag("user"))) * buf
    size_t count
    loff_t * pos
    ssize_t retval
<chop>
```

Let's take the *vfs_unlink* shown above and see if we can extract the name of a
file being removed (unlinked from its parent directory). The directory entry being
unlinked is stored under the `dentry` parameter and to print this structure we use
the `-lv` option to bpftrace:

```
$ bpftrace -lv 'struct dentry'
struct dentry {
        unsigned int d_flags;
        seqcount_spinlock_t d_seq;
        struct hlist_bl_node d_hash;
        struct dentry *d_parent;
        struct qstr d_name;
        struct inode *d_inode;
        unsigned char d_iname[32];
        struct lockref d_lockref;
        const struct dentry_operations *d_op;
        struct super_block *d_sb;
        unsigned long d_time;
        void *d_fsdata;
        union {
                struct list_head d_lru;
                wait_queue_head_t *d_wait;
        };
        struct list_head d_child;
        struct list_head d_subdirs;
        union {
                struct hlist_node d_alias;
                struct hlist_bl_node d_in_lookup_hash;
                struct callback_head d_rcu;
        } d_u;
};
```

The `d_name` member (of type `struct qstr`) will usually hold the name of the directory entry represented
by this `struct dentry`. A 'struct qstr` looks like:

```
$ bpftrace -lv 'struct qstr'
struct qstr {
        union {
                struct {
                        u32 hash;
                        u32 len;
                };
                u64 hash_len;
        };
        const unsigned char *name;
};
```

Using the types shown above we can simply dereference the members from the initial `dentry` argument to extract the name.  Note: a kfunc probes arguments are presented in the *args* variable and we access them
using the parameter name they have in the source code, e.g., `args.dentry` in this case to access the 'struct dentry` parameter.

Top tip: We can use the `print()` bpfscript function to print out member in an object without explicitly specifying them. For example, the arguments for a `vfs_write()` call are:

```
$ bpftrace -lv 'kfunc:vfs_write'
kfunc:vmlinux:vfs_write
    struct file * file
    const char __attribute__((btf_type_tag("user"))) * buf
    size_t count
    loff_t * pos
    ssize_t retval

$ bpftrace -e  'kfunc:vfs_write{print(args)}'
Attaching 1 probe...
{ .file = 0xffff88a79ffa6900, .buf = 0x7f77b8ff9730, .count = 8, .pos = 0xffffc900268f7ef8 }
{ .file = 0xffff88a79ffa7d00, .buf = 0x7f77b8ff9730, .count = 8, .pos = 0xffffc900268f7ef8 }
{ .file = 0xffff88a79ffa7900, .buf = 0x7f77b8ff9730, .count = 8, .pos = 0xffffc900268f7ef8 }
{ .file = 0xffff88a79ffa6e00, .buf = 0x7f77b8ff9730, .count = 8, .pos = 0xffffc900268f7ef8 }
{ .file = 0xffff88a79ffa7400, .buf = 0x7f77b8ff9730, .count = 8, .pos = 0xffffc900268f7ef8 }
{ .file = 0xffff88a79ffa6600, .buf = 0x7f77b8ff9730, .count = 8, .pos = 0xffffc900268f7ef8 }
{ .file = 0xffff889faebd4600, .buf = 0x7ffeb7dc0d90, .count = 24, .pos = 0xffffc9004c92bef8 }
{ .file = 0xffff889faebd4600, .buf = 0x55b25a8b7988, .count = 4096, .pos = 0xffffc9004c92bef8 }
{ .file = 0xffff889faebd4600, .buf = 0x7ffeb7dc0d90, .count = 24, .pos = 0xffffc9004c92bef8 }
{ .file = 0xffff889faebd4600, .buf = 0x55b25a837948, .count = 4096, .pos = 0xffffc9004c92bef8 }
{ .file = 0xffff889faebd4600, .buf = 0x7ffeb7dc0d00, .count = 24, .pos = 0xffffc9004c92bef8 }
{ .file = 0xffff889faebd4600, .buf = 0x55b25a9cdc38, .count = 4096, .pos = 0xffffc9004c92bef8 }
```

We can also use print to display the contents of whole objects. Foe example, to dump the `struct file` that is passed as the first entry to a `vfs_write()` call:

```
$ bpftrace -e  'kfunc:vfs_write{print(*args.file)}'
Attaching 1 probe...
{ .f_u = , .f_path = { .mnt = 0xffff88810b488020, .dentry = 0xffff888109431b00 }, .f_inode = 0xffff8881079a1bd0, .f_op = 0xffffffff82214268, .f_lock = { .rlock = { .raw_lock = { .val = , .locked = 0, .pending = 0, .locked_pending = 0, .tail = 0 } } }, .f_count = , .f_flags = 32770, .f_mode = 917535, .f_pos_lock = { .owner = , .wait_lock = { .raw_lock = { .val = , .locked = 0, .pending = 0, .locked_pending = 0, .tail = 0 } }, .osq = { .tail =  }, .wait_list = { .next = 0xffff8886da8fdd58, .prev = 0xffff8886da8fdd58 } }, .f_pos = 0, .f_owner = { .lock = , .pid = 0x0, .pid_type = 0, .uid = , .euid = , .signum = 0 }, .f_cred = 0xffff88bb3dc1c480, .f_ra = { .start = 0, .size = 0, .async_size = 0, .ra_pages = 0, .mmap_miss = 0, .prev_pos = -1 }, .f_version = 0, .f_security = 0x0, .private_data = 0x0, .f_ep = 0x0, .f_mapping = 0xffff8881079a1d48, .f_wb_err = 0, .f_sb_err = 0 }
```

Back to our `vfs_unlink` example, let's now see what files are being unlinked. Along with the file we print the name of the userland thread issuing the unlink:

```
$ cat vfs_unlink.bt
kfunc:vfs_unlink
{
  printf("%s %s\n", comm, str(args.dentry->d_name.name));
}

$ bpftrace ./vfs_unlink.bt
Attaching 1 probe...
systemd-journal system@3274dec3739849b1a6f31a597646aa6c-0000000018d30c31-000622..
chefctl chef.cur.out
systemd-journal 8:1979497868
dnf rpmdb_lock.pid
dnf download_lock.pid
dnf metadata_lock.pid
carbon-global-s libmcrouter.fb-servicerouter.fci.startup_options.temp-CCH0sUZHt..
carbon-global-s libmcrouter.fb-servicerouter.fci.stats.temp-KlxDTQnDtz
carbon-global-s libmcrouter.fb-servicerouter.fci.config_sources_info.temp-vEnIl..
systemd-network .#22ffe719f631a96c6
systemd invocation:fbwallet_fetch.service
carbon-global-s libmcrouter.hhvm.web.stats.temp-D3NkRc8hqb
carbon-global-s libmcrouter.hhvm.web.config_sources_info.temp-0IDMBUC0nr
blkid blkid.tab.old
```

Challenge: Expand the above script to print the parent directory for the file being unlinked.


XXX kprobe - talk about how to access per instruction offsets and give example. That's the only real reason why you would use them I think over kfuncs.

### tracing return values with kretval

The typed return argument from a function is available through the `retval` variable in a `kretfunc` probe. An example lifted straight from the bpftrace docs displays how easy it is to use with the kernel fget() function. This function returns a 'struct file' for a given file descriptor so we can use that to 

```
$ bpftrace -lv 'kretfunc:fget'
kretfunc:vmlinux:fget
    unsigned int fd
    struct file * retval

$ cat fget.bt
kretfunc:fget
/retval/
{
  if (strcontains(str(retval->f_path.dentry->d_name.name), "crack")) {
    printf("%s (fd%d):  %s\n", comm, args.fd, str(retval->f_path.dentry->d_name.name));
  }
}

$ bpftrace ./fget.bt
Attaching 1 probe...
sudo (fd8):  libcrack.so.2.9.0
sudo (fd8):  libcrack.so.2.9.0
sudo (fd8):  libcrack.so.2.9.0
sudo (fd8):  libcrack.so.2.9.0
```


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

When writing tracing scripts we often need to reach into structures to extract
structure members that we are interested in at that specific point in execution.
A previous version of this lab demonstrated how to use the `pahole` tool to look
at the type definitions and I've preserved that information at the bottom of this
section for posterity [ XXX - link] . This lab assumes that the kernel you are
running on has type information available via the BTF type subsystem. To verify
BTF is available on your system:

```
 bpftrace --info |& grep -i btf
  btf: yes
  module btf: yes
```

Of course you could use the kernel source code but the BTF on your system is
the **exact** type definitions for the objects found on your system so you
should use that if possible.



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
