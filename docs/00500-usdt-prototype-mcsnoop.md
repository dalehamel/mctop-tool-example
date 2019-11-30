# bpftrace script for mcsnoop

With a basic test lab now set up to try out USDT probes on `memcached`, it
would now be could easier to investigate how to pull out cleaner data from the
`dtrace` probes.

In the `dtrace` file, I saw that all of the commands memcached supports had
dtrace probes defined, for instance:

```{.c include=src/memcached/memcached_dtrace.d startLine=205 endLine=214}
```

This `command__set` probe negates the need to parse this the command string,
and it looks like all of the other commands also have dtrace probes.

These definitions are generated into header macros that are callable from the
`memcached` source code. This is what the calls to emit data to a probe look
like in `memcached`:

```{.c include=src/memcached/memcached.c startLine=1358 endLine=1386}
```

A `bpftrace` script that provided similar output to `mctop`, at least for the
`SET` command, looks like this then:

```{.awk include=src/mcsnoop-orig.bt}
```

But this wasn't really a `top`-like tool, it just prints results as it gets
data. To see how this might be done, I checked out some of Brendan Gregg's
examples from his now book's repo, and came across `slabratetop.bt`:

```{.awk include=src/bpf-perf-tools-book/originals/Ch14_Kernel/slabratetop.bt startLine=16 endLine=35}
```

This showed how to build a top-like tool in `bpftrace`, but also the
limitations of doing so. You can basically just print the map data out on a
recurring interval.

So, at this point I hit the limit of what `bpftrace` could do for me.
It is great for analyzing map data, but not so great at producing interactive
top-like UIs yet, as that involves some sophisticated post-processing of the
map data. For this reason, a complete `mctop` tool is currently out of reach of
bpftrace, so I set my eyes on a `bcc` tool to do the same.

Ultimately, the most complete working version of this bpftrace prototype is
something I'd rather call `mcsnoop`, as it snoops `memcached` key access using
USDT probes.

This is the current most functional version of the `bpftrace` utility, when
 I stopped development on it[^18]:

```{.awk include=src/mcsnoop-working.bt}
```

[^18]: this solves the problem I would later have in my `bcc` version of
       treating `void *` byte buffers properly, which I'll be covering in more
       detail later.
