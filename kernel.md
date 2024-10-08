## Kernel Tracing

In this lab you will experiment with tracing the Linux kernel using four different
probe types:

- *kprobe*: dynamically instrumented kernel function probes that do
not have overhead when disabled and minimal overhead when enabled. 
- *kfunc*: provides mostly the same functionality as a 'kprobe'
but with the big advantage of having fully typed arguments using the kernel's
[BTF type system](https://docs.kernel.org/bpf/btf.html). *kfunc* as a probe name is not to be confused with the BPF
kernel functions known as *kfuncs* which are [regular kernel functions that are
exposed for use by bpf programs](https://docs.kernel.org/bpf/kfuncs.html) . *kfunc*
when used as a name for a probe is a synonym for *fentry* kernel functions. The
probe name of *kfunc* probably originated in bcc but that's not important really for now. 
- *tracepoint*: statically defined probe sites in the Linux kernel source code. Tracepoints
are more stable than dynamic probes both in terms of probe naming and also in their
arguments.
- *rawtracepoint*: essentially identical in terms of functionality to a *tracepoint* probe. The only difference is that rawtracepoint offers access to raw arguments instead of typed arguments.


### A side note: navigating kernel type information

When writing tracing scripts we often need to reach into structures to extract
structure members that we are interested in at that specific point in execution.
This lab assumes that the kernel you are running on has type information available via the BTF type subsystem. To verify
BTF is available on your system:

```
# bpftrace --info |& grep -i btf
  btf: yes
  module btf: yes
```

Of course you could use the kernel source code but the BTF on your system provides
the **exact** type definitions for the objects found on your system so you
should use that if possible.

### A side note: available probe points

A significant amount of kernel functions are available to be traced but not all of them. The primary reason why a function may not be available is because it has been inlined by the compiler and, currently, bpftrace cannot expose inlined functions. (Work is currently ongoing to expose inlined functions).

## kfunc probes

To list all available kfunc probes beginning with the pattern `vfs_` and displaying their input and return parameter types we can use a wildcard search:

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

Let's take the *vfs_unlink* probe shown above and see if we can extract the name of a
file being removed (unlinked from its parent directory). The directory entry being
unlinked is stored under the `dentry` parameter:

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
using the parameter name they have in the source code, e.g, `args.dentry` in this case to access the 'struct dentry` parameter.

Let's now see what files are being unlinked. Along with the file we print the name of the userland thread issuing the unlink:

```
# cat vfs_unlink.bt
kfunc:vfs_unlink
{
  printf("%s %s\n", comm, str(args.dentry->d_name.name));
}

# bpftrace ./vfs_unlink.bt
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

## Exercises

1. Expand the above script to print the parent directory for the file being unlinked. Hint: xxx

Top tip: We can use the `print()` BpfScript function to print out members in an object without explicitly specifying them. For example, the arguments for a `vfs_write()` call are:

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


## kretfunc probes

The typed return argument from a function is available through the `retval` variable in a `kretfunc` probe. The following example displays how easy it is to use with the kernel `fget()` function. This function returns a 'struct file' for a given file descriptor so we can use that to extract filenames:

```
# bpftrace -lv 'kretfunc:fget'
kretfunc:vmlinux:fget
    unsigned int fd
    struct file * retval

# cat fget.bt
kretfunc:fget
/retval/
{
  if (strcontains(str(retval->f_path.dentry->d_name.name), "crack")) {
    printf("%s (fd%d):  %s\n", comm, args.fd, str(retval->f_path.dentry->d_name.name));
  }
}

# bpftrace ./fget.bt
Attaching 1 probe...
sudo (fd8):  libcrack.so.2.9.0
sudo (fd8):  libcrack.so.2.9.0
sudo (fd8):  libcrack.so.2.9.0
sudo (fd8):  libcrack.so.2.9.0
```


### kprobe/kretprobe and their arguments

XXX Example with kprobe:func+offset and register extraction preferably with inline function. Talk about how to access per instruction offsets and give example. That's the only real reason why you would use them I think over kfuncs.

`kprobe' probes arguments are not typed and therefore, for most use cases, the newer `kfunc` probes that we have described above should be used. One area that the kprobe has unique functionality is its ability to probe individiual kernel instruction sites and not just function entry and return locations as with `kfunc` probes.


## Static tracepoints

Tracepoint probes are static probes: they aren't generated dynamically and they are defined in the kernel source itself. The advantage of this over dynamic probes is that they tend to provide a more stable interface.

On a typical system we have in the order of more than 2000 static tracepoints. Give them an eyeball for yourself by inspecting the output of ` bpftrace -l 'tracepoint:*`.

In order to understand how we can use tracepoints we'll look at a couple of them which are designed to be used in a pair in order to understand kernel lock contention:

```
# sudo bpftrace -lv 'tracepoint:lock:contention*'
tracepoint:lock:contention_begin
    void * lock_addr
    unsigned int flags
tracepoint:lock:contention_end
    void * lock_addr
    int ret
```

The semantics of a probe should be encoded into its name and that is definitely the case here as we have one probe that fires when a contention event for a synchronisation primitive starts and one for when it ends. Let's develop a script that tracks contention times on locks. Start by recording the time when a lock contention event begins:

```
tracepoint:lock:contention_begin
{
  @[tid, args.lock_addr] = nsecs;
}

```

Things to note:

* When a `contention_begin` probe fires we record the nanosecond timestamp when the event occurred. Note that we use the thread id (`tid`) together with the lock address to create a unique key for the aggregation. **Question**: What would be the problem with just using the lock address as a key?

Now expand the script to process when a contention event ends:

```
tracepoint:lock:contention_end
/@[tid, args.lock_addr] != 0/
{
  $lock_duration = nsecs - @[tid, args.lock_addr];
  if ($lock_duration > 1000*100) { /* block duration threshold time -  100 microsecs */
    printf("contended time: %lld\n", $lock_duration);
  }
  delete(@[tid, args.lock_addr]);
}
```

Things to note:

* The predicate ensures that we only process `contention_end` probes if we have first gone through the `contention_begin` probe.
* The lock duration is stored in a scratch variable. If the block duration was > 100 microsec lock address and blocking time is printed.
* We delete the entry in the map that was just processed using the `delete()` builtin function.

---

## Exercises

Expand the lock contention script to include the following functionality:

1. Make the block duration threshold time (currently set to 100 microsecs) into a parameter passed into the script.
1. For locks that exceed the threshold duration, instead of printing information, store the blocking time into a map named `long_block_times` and use the hist() action and indexed using the lock addressed.
1. Add an END probe where you print out the `@long_block_times` map but not the anonymous map used to calculate the results.
1. For the extra keen, use the flag definitions for `args.flags` in `include/trace/events/lock.h` and  modify the script to print which type of lock is being dealt with.


The following 3 articles contain everything you need to know about Linux's static tracepoints:

* https://lwn.net/Articles/379903/
* https://lwn.net/Articles/381064/
* https://lwn.net/Articles/383362/

## Exercises

NOTE: before attempting the tasks in this section select the `kernel` option from the `bpfhol` menu.

1. The `kprobeme` process `open(2)`s and `read(2)`s a file once a second. Which file is it?
1. Is `kprobeme` opening a new file descriptor every time?
1. Is `kprobeme` leaking file descriptors? (HINT: use an associative map)


After this excursion in to the kernel we're going back up into userland with [uprobes](uprobe.pdf).

---

## Further Reading

* https://github.com/torvalds/linux/blob/master/Documentation/kprobes.txt
* https://www.kernel.org/doc/html/latest/trace/kprobetrace.html
* https://www.kernel.org/doc/ols/2007/ols2007v2-pages-35-44.pdf
