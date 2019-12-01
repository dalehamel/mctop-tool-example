# bpftrace script for mcsnoop

With a basic test lab now set up to try out USDT probes on Memcached, it
would now be easier to investigate how to pull out cleaner and more
complete data from the Dtrace probes.

In the Dtrace probe definitions file:

```{.c include=src/memcached/memcached_dtrace.d startLine=205 endLine=214}
```

This `command__set` probe negates the need to parse this the command string,
and it looks like all of the other commands also have Dtrace probes.

These definitions are generated into header macros that are callable from the
Memcached source code. This is what the calls to emit data to a probe look
like in Memcached:

```{.c include=src/memcached/memcached.c startLine=1358 endLine=1386}
```

This can be tested by sending a test 'SET' command to a Memcached
instance. By piping printf into netcat, [@memcached-cheatsheet] standard shell
tools can send test commands in the Memcached string protocol:

```{.bash include=src/printf-test.sh}
```

By reading arg3 to get the probe size, a bpftrace script could be
written that provided similar output to mctop, at least for the SET
command:

```{.awk include=src/mcsnoop-orig.bt}
```

But this wasn't really a `top`-like tool, it just prints results as it gets
data. To see how this might be done, Brendan Gregg's examples from
his new book's [@bpf-perf-tools-book] repo[@bpf-tools-book-repo], has 
slabratetop.bt:

```{.awk include=src/bpf-perf-tools-book/originals/Ch14_Kernel/slabratetop.bt startLine=16 endLine=35}
```

This showed how to build a top-like tool in `bpftrace`, but also the
limitations of doing so. You can basically just print the map data out on a
recurring interval.

So for a UI, this was about the limit of what `bpftrace` could easily
provide. It is great for analyzing map data, but not so great at
producing interactive top-like UIs yet, as that involves some
sophisticated post-processing of the map data.

Ultimately, the most complete working version of this `bpftrace` prototype
is something more like a sniffer, so a name like `mcsnoop`, is more
appropriate.

A full version of the latest source for `mcsnoop`[^18] is available in the
repository for this report [@this-doc-github]:

```{.awk include=src/mcsnoop-working.bt}
```

[^18]: this solves the problem I would later have in my `bcc` version of
       treating `void *` byte buffers properly, which I'll be covering in more
       detail later.
