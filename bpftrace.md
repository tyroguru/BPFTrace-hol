# bpftrace

In this lab we will work through some of the key features of the bpftrace tracing technology and its language, which is known as `BpfScript`. This lab is not an exhaustive treatment of the language but rather the key concepts. Please refer to the [bpftrace manual](https://github.com/bpftrace/bpftrace/blob/master/man/adoc/bpftrace.adoc) for details on all language features.

**bpftrace** is specifically designed for tracing user and kernel software. Its primary purpose is to facilitate observation of software behaviour. As such, it provides a number of key language primitives that enable us to gain detailed insights into the real runtime behaviour of the code we write, which is rarely what we think it actually is!. In this section we will look at the key language primitives and some techniques which enable us to obtain fresh insights.

## Dynamic Tracing - So What!??

A key attribute of bpftrace is its *dynamic* nature. To understand the myriad complexities and nuances of the execution profile of a modern software stack, we tend to go through a cyclical process when reasoning about systemic behaviour (sometimes referred to as the 'Virtuous Circle'):

1. Form a hypothesis based upon our current understanding
1. Gather data / evidence to prove or disprove our hypothesis
1. Analyze data
1. Repeat until satisfaction is achieved

The "problem" with the above sequence is that modifying software to generate the trace data and re-running experiments tends to dominate the time (step 2). In production it is often impossible to install such debug binaries and very painful on anything but trivial development systems. In addition, we rarely capture the data that we need the first time around and it often takes many iterations to gather all the data we need to debug a problem.

bpftrace solves these problems by allowing us to dynamically modify our system to capture arbitrary data without modifying any code. As modifying the system is so easy to do, we can very quickly iterate through different hypotheses and gain novel insights about systemic behaviour in very short periods of time.

## Action Blocks

bpftrace scripts (written in the *BpfScript* tracing language) are made up of one or more *Action Blocks*. An action block contains 3 parts in the following order:

* **A probe**: this is a place of interest where we interrupt the executing thread. There are numerous probe types but examples include the location of a function (e.g, strcmp(3)), when a system call is executed (e.g, write(2)), an event such as a performance counter overflow event, or a periodic timer. The key point here is that this is somewhere where we can collect data.
* **An optional predicate** (sometimes called a *filter*). This is a logical condition which allows us to decide if we are interested in recording data for this event. For example, is the current process named 'bash' or is the file we are writing to located in `/tmp`. Predicates are contained in between two forward slash characters.
* **Actions to record data** . Actions are numerous and mostly capture data that we are interested in. Examples of such actions may be recording the contents of a buffer, capturing a stack trace, or simply printing the current time to stdout. All actions are contained in between a pair of curly braces.

An example action block looks like this:

```
tracepoint:syscalls:sys_enter_write   /* The probe */
/comm == "bash"/                      /* The predicate */
{
  @[args->fd] = sum(args->count);     /* An action */
}
```

Exercise: Although we haven't been formally introduced to any bpftrace details, can you guess what the above action block does?

### Starter Example

We'll start with the classic example of looking at system calls (see the [syscalls lab](syscalls.pdf) for further details).

1. First let's see what system calls are being made. Run this bpftrace invocation for 15-20 seconds.

```
# bpftrace -e 'tracepoint:syscalls:sys_enter_*{@calls[probe] = count();}'
Attaching 314 probes...

^C
<output elided>
@calls[tracepoint:syscalls:sys_enter_newfstatat]: 240998
@calls[tracepoint:syscalls:sys_enter_openat]: 249219
@calls[tracepoint:syscalls:sys_enter_read]: 283535
@calls[tracepoint:syscalls:sys_enter_futex]: 283681
@calls[tracepoint:syscalls:sys_enter_close]: 351776
```

Things to note:

* We use a wildcard (`*`) to pattern match all the system call entry probes.
* `@calls[]` declares a special type of associative array (known as a *map* or an *aggregation*). We didn't have to name the associative array as we only have one - we could have just used the `@` sign on its own (known as the `anonymous array`).
* We index the `@calls[]` array using the `probe` builtin variable. This expands to the name of the probe that has been fired (e.g, `tracepoint:syscalls:sys_enter_futex`).
* Each entry in a map can have one of a number of values or pre-defined functions associated with it. Here the `count()` function simply increments an associated counter every time the function is called and we therefore keep count of the number of times a probe has been hit.

**NOTE:** The order and number of system calls you see will be different to the output given above. For the sake of this example we will focus on `close()` calls.

**NOTE:** maps are a key data structure that you'll use very frequently!

2. Now let's iterate using the data we just acquired to drill down and discover who is making those `close(2)` syscalls! Again, let's give it 15-20 seconds before issuing a `<ctrl-c>`:

```
# bpftrace -e 'tracepoint:syscalls:sys_enter_close{@calls[comm] = count();}'
Attaching 1 probe...
^C
<output elided>
@calls[agent#native-ma]: 23337
@calls[systemd-userwor]: 24859
@calls[fb-oomd-cpp]: 31130
@calls[below]: 36805
@calls[FuncSched]: 43969
```

Things to note:

* We changed the probe specification as we are only interested in close calls now.
* Instead of indexing by the probe name we now index by the name of the process making the close syscall using the `comm` builtin variable.

The result of this tracing iteration tell us that a process named `FuncSched` is making the most `close` calls so we may want to drill down this process to see where in the code these calls are being made from:

```
# bpftrace -e 'tracepoint:syscalls:sys_enter_close /comm == "FuncSched"/
{
  @calls[ustack] = count();
}'
Attaching 1 probe...
^C
<output elided>
@calls[
    __GI___close+61
    facebook::pid_info::PidInfo::readEnvironmentForPid(int, std::filesystem::__cxx11::path const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&, std::unordered_map<std::basic_string_view<char, std::char_traits<char> >, std::basic_string_view<char, std::char_traits<char> >, std::hash<std::basic_string_view<char, std::char_traits<char> > >, std::equal_to<std::basic_string_view<char, std::char_traits<char> > >, std::allocator<std::pair<std::basic_string_view<char, std::char_traits<char> > const, std::basic_string_view<char, std::char_traits<char> > > > >&)+736
    facebook::pid_info::PidInfo::getServiceIDForPid(int, std::filesystem::__cxx11::path const&)+1302
    facebook::dynolog::threadmon::ThreadMonitor::getAllServiceIDs[abi:cxx11]()+883
    facebook::dynolog::SAHResolver::resolveAndWrite(std::optional<std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > > const&, std::set<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > const*)+745
    void folly::detail::function::call_<std::_Bind<void (facebook::dynolog::SAHResolver::*(facebook::dynolog::SAHResolver*, std::nullopt_t, decltype(nullptr)))(std::optional<std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > > const&, std::set<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > const*)>, true, false, void>(, folly::detail::function::Data&)+52
    std::thread::_State_impl<std::thread::_Invoker<std::tuple<folly::FunctionScheduler::start()::$_0> > >::_M_run() [clone .__uniq.292305056905277424697830817796292569046]+1046
    execute_native_thread_routine+21
    start_thread+569
    __clone3+44
]: 3101
```

Things to note:

* We now keep a count of the number of times a unique stack trace was seen for each close syscall by using the `ustack` builtin as the map key and using the `count()` map function for its value.

### Exercises

1. Write a script to keep count of the number of system calls each process makes. (In addition to count(), try using the sum() aggregating function and/or the `++` increment operator).


## Timers

We often want to periodically display data held in aggregations and this can be done with the `interval` probes which provide periodic interval timers. For example, to print the date and time every 10 seconds:

```
# bpftrace -e 'interval:s:10 { time("%c\n"); }'
Attaching 1 probe...
Thu Sep 26 10:18:35 2024
Thu Sep 26 10:18:45 2024
Thu Sep 26 10:18:55 2024
Thu Sep 26 10:19:05 2024
```

### Exercises

1. Expand the script written previously to print the per-process system call counts every 10 seconds.
1. Add the ability to only display the top 10 per process counts (hint: use the `print` action)
1. Delete all per-process syscall stats every 10 secs (hint: use the `clear` action);
1. Finally, exit the script after 3 iterations (or 30 seconds if you prefer it that way)


## pid's, tid's and names

Process and thread identifiers are something we come across a lot when trying to track behaviour of our code. It's important to understand exactly what is referred to here when dealing with multi-threaded processes:

- `pid`: The *process id* is constant for every thread in a process - this is the identifier given to the very first thread in the process and is referred to in Linux as the tgid (Thread Group Id).
- `tid`: every thread is given a *thread id* to uniquely identify it. This is confusingly referred to in Linux as the threads PID.
- `comm`: we've met this builtin variable previously and it provides the name of the current thread. NOTE: threads inherit the name from their parent but many set their own thread name so threads within the same process may well have different names.

### Exercise

Let's look at the `cppfbagentd` process as an example:

1. Count the syscalls made by each `<pid, tid, comm>` tuple for every thread in the main cppfbagentd process (use "`pgrep -f cppfbagentd`" to find the main process pid and predicate using that),
2. Target a particular tid discovered previously and keep a count of the individual syscalls it makes.

## Associative arrays and tracking threads

Sometimes we may want to track the behaviour of individual threads within a process and associative arrays are perfect for this. For example, we may want to time how long it took a thread to execute a specific function. As there may be many threads simultaneously executing this function we need to use something unique to the executing thread to identify it. We can use the `tid` as a key for an associative array to store a timestamp of when we enter a function. Here we time how long it takes a thread to execute a write() syscall:

```
  tracepoint:syscalls:sys_enter_write
  {
    @[tid] = nsecs;
  }

  tracepoint:syscalls:sys_exit_write
  /@[tid]/
  {
    $time_taken = nsecs - @[tid];
    delete(@[tid])
  }
```

Things to note:

* The `nsecs` builtin variable gives us nanosecond timestamp
* The predicate on the return probe ensures this thread has actually been through the entry probe (we could have started tracing whilst this thread was already in this function!).
* The `$` notation indicates that we have declared a *scratch* variable that only has scope within this action block. Here the variable `$time_taken` stores the time taken in the write() syscall.


### Exercise

1. Pick a thread from a process on your system and also one of the system calls that it makes. Write a script to time the calls and print the result using `printf`.
1. Use the `hist()` aggregating function to track the range of times taken by this syscall.
1. Now add the `max()` and `min()` functions in to track the lowest to highest times.
1. Can you think of how you might dump the stack of a thread when it hits a new highest time value? Implement it.

Note that the `interval` based probes we have used previously only fire on a single CPU which is what we want for a periodic timer. However, if we want to sample activity on all CPU's at a fixed interval period we need to use the `profile` probe.

### Exercise

1. A `profile` probe is the same format as the `interval` probe that we have seen previously. Write a script which uses a 10 millisecond `profile` probe (profile:ms:10)  to count the number of times a non-root thread (uid != 0) was running when the probe fired. (Hints: key the map with the `cpu` builtin variable and you'll also need the `uid` builtin variable. Bonus points for use of the `if` statement instead of a predicate (it's not any better here but just provides variation!).

Now that we've covered some of the basic building blocks of bpftrace, we'll continue the voyage of discovery by looking at the fundamental interface between userland code and the kernel: the [system call](syscalls.md).

## Further Reading

 * [bpftrace manual](https://github.com/bpftrace/bpftrace/blob/master/man/adoc/bpftrace.adoc)
