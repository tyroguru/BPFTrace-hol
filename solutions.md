## Solutions to Lab Exercises

Quite often there will a number of possible solutions to the lab exercises. Below are suggested solutions (yours may well be better!).

### The bpftrace language

1. This action block keeps track of the number of bytes written to each file descriptor for the hhvm process (more accurately, any thread with the thread name of "hhvm").

1.  Write a script to keep count of the number of system calls each process makes:

```
tracepoint:syscalls:sys_enter_*
{
  @[comm] = count();
}
```
or
```
tracepoint:syscalls:sys_enter_*
{
  @[comm] = sum(1);
}
```
or
```
tracepoint:syscalls:sys_enter_*
{
  @[comm]++;
}
```

1. Expand the above script to display the per-process system call counts every 10 seconds:
```
tracepoint:syscalls:sys_enter_*
{
  @[comm]++;
}

interval:s:10
{
  print(@);
}
```


1. Delete all per-process syscall stats every 10 secs:

```
tracepoint:syscalls:sys_enter_*
{
  @[comm]++;
}

interval:s:10
{
  print(@);
  clear(@);
}
```

1. Finally, exit the script after 3 iterations:

The simplest way is as above with the addition of the following action block:

```
interval:s:30
{
  exit();
}
```

Alternatively, for the more adventurous we can use a global variable to count the number of times we have hit the 10 second interval timer:

```
BEGIN
{
  @i = 0;
}

tracepoint:syscalls:sys_enter_*
{
  @[comm]++;
}

interval:s:10
{
  @i++;
  print(@);
  clear(@);

  if (@i == 3)
  {
    exit();
  }
}
```

1. Count the syscalls made by each `<pid, tid,comm>` tuple for every thread in the cppfbagentd process:

```
tracepoint:syscalls:sys_enter_*
/pid == $1/
{
  @[pid,tid,comm] = count();
}
```

Note: The `$1` is a *positional parameter* which allows us to pass the pid of cppfbagentd as a parameter to the script. You could just hard code the pid of the process if you wanted.

1. Target a particular tid discovered previously and keep a count of the individual syscalls it makes:

```
tracepoint:syscalls:sys_enter_*
/pid == $1 && tid == $2/
{
  @[probe] = count();
}
```

1. Pick a thread from cppfbagentd and also one of the system calls that it makes. Write a script to time the calls and print the result using `printf`.

t:syscalls:sys_enter_write
/tid == $1/
{
        @ts[tid] = nsecs;
}

t:syscalls:sys_exit_write
/@ts[tid]/
{
        printf("time taken for write: %lld nsecs\n", nsecs - @ts[tid]);
        @[tid] = 0;
}


1. Use the `hist()` aggregating function to track the range of times taken by this syscall.

The is the same as the solution above but with the printf() replaced:

t:syscalls:sys_enter_write
/tid == 1414349/
{
        @ts[tid] = nsecs;
}

t:syscalls:sys_exit_write
/@ts[tid]/
{
        @histwrite[probe] = hist(nsecs - @ts[tid]);
        @[tid] = 0;
}


1. Now add the `max()` and `min()` functions in to track the lowest to highest times.

t:syscalls:sys_enter_write
/tid == 1414349/
{
        @ts[tid] = nsecs;
}

t:syscalls:sys_exit_write
/@ts[tid]/
{
        $ts = nsecs - @ts[tid];
        @h[probe] = hist($ts);
        @maxwrite[probe] = max($ts);
        @minwrite[probe] = min($ts);
        @[tid] = 0;
}


1. Can you think of how you might dump the stack of a thread when it hits a new highest time value? Implement it.

BEGIN
{
        @curmaxval[tid] = 0;
}

t:syscalls:sys_enter_write
/tid == 1414349/
{
        @ts[tid] = nsecs;
        kstack;
}

t:syscalls:sys_exit_write
/@ts[tid]/
{
        $ts = nsecs - @ts[tid];
        $maxval = @curmaxval[tid];

        if ($ts > $maxval)
        {
                @curmaxval[tid] = $ts;
                printf("New max value hit: %llu nsecs\n", $ts);
                printf("%s\n", ustack());
        }

        @h[probe] = hist($ts);
        @maxwrite[probe] = max($ts);
        @minwrite[probe] = min($ts);
        @[tid] = 0;
}


1. Write a script which uses a 10 millisecond `profile` probe (profile:ms:10)  to count the number of times a non-root thread (uid != 0) was running.

profile:ms:10
{
  if (uid != 0)
  {
    @[cpu, comm]++;
  }
}
