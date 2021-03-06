# Probing memcached with bpftrace uprobes

One of the reasons we were able to deploy bpftrace so quickly to solve this
issue was because we have distributed eBPF tools in production via a custom
toolbox image since autumn of 2018, and have had `bpftrace` deployed to
production along with the standard bcc tools.

At Shopify, `kubectl-trace` is standard issue to anyone with production
access. This makes it easy for developers to probe their applications,
and the system faculties that support them. Developing this sort of tool
library allows for easily applying purpose-built analysis tools to
investigate production issues.

This brings into reach tools that would otherwise be too scary or inaccessible,
like kernel `kprobes` and `uprobes`. `bpftrace`, in particular, allows for
simple and concise probe definitions, and is great for prototyping more complex
tools, and poking around to find useful data sources.

For this issue, `bpftrace` has the ability to target any ELF binary with
`uprobes` and read method calls and returns for an application like Memcached.
This was the first entry-point into investigating the Memcached key access
patterns.

## memcached sources

Camilo Lopez [@camilo-github] came up with the idea to attach a uprobe to the
`process_command` function in Memcached. In the Memcached source code, the
signature in `memcached.c` shows the argument types and order:

```{.c include=src/memcached/memcached.c startLine=5756 endLine=5756}
```

This shows us that the second argument (`arg1` if indexed from 0) is
the command string, which contains the key.

Now that we know the signature, we can verify that we can find this symbol in
the Memcached binary:

```
objdump-tT /proc/PID/root/usr/local/bin/memcached | grep process_command -B2 -A2

```
[^1]

Which shows us that it is indeed a symbol we can access:

```{.gnuassembler include=src/objdump.txt}
```

This is how `bpftrace` will target the probe, by resolving the address of this
symbol in the code region of the target process' memory space.

## uprobe prototype

To probe read the commands issued to Memcached, we can target the binary
directly[^2], and insert a breakpoint at this address. When the breakpoint is
hit, our eBPF probe is fired, and bpftrace can read the data from it.

The simplest solution and first step towards a more sophisticated tool
is to just read the command and print it which can easily be done as a
`bpftrace` one-liner:

```awk
bpftrace -e 'uprobe:/proc/PID/root/usr/local/bin/memcached:process_command { printf("%s\n", str(arg1)) }'
```
[^6]

Then running a test command on Memcached generates probe output! This shows
that `bpftrace` can read data from user-space using the kernel's `uprobe`
faculties.

```
Attaching 1 probe...
set memtier-3652115 0 60 4
```

Now, to quickly turn this into a relatively useful tool, each time a key is hit
it can be incremented in a dictionary. Using the `++` operator to count each
time the command is called:

```{.awk include=src/uprobe-v1.bt}
```

When this exits, it will print a sorted map of the commands, which should
correspond to the most frequently accessed keys.

With a working uprobe prototype, attention now turns to getting better
quality data. `bpftrace` doesn't really have the faculty to parse strings
at the moment and this is inherently pretty inefficient, and thus not
something ideal to do each time a probe is called, so it is better if arguments
are passed of a known type.

The problem of building more flexible tools is better solved by the use of the
USDT tracepoint protocol for defining static tracepoints. Fortunately, this has
already been established in many packages by the popular use of Dtrace
on other Unix platforms like Solaris, BSD, and their derivatives, such as
Darwin. Systemtap has provided Linux compatibility, which is what `bpftrace`
and `bcc` are able to leverage.

[^1]: Using docker or crictl, we can find the container process and inspect its
    children to find the memcached process. This method then uses the `/root`
    handle to access the process's mount namespace, and read the exact instance
    of the memcached binary we want to probe.

[^2]: The initial prototype of the uprobe tool targeted the memcached binary
    directly, as while we were using a recent version of bpftrace (0.9.2), which
    ships with Ubuntu Eoan, it was linked with libbcc 0.8.0, which didn't have
    all of the USDT functionality and namespace support to read containerized
    processes correctly. For this reason
[^6]: This is not the ideal syntax and is a regression, container tracing is a
    bit working with USDT probes, as are uprobes. Specifying the full path from
    the /proc hierarchy seems to work well enough though.
