# bpftrace uprobe memcached

We have had access to bpf tools in production via a custom toolbox image for
about a year now, and have `bpftrace` deployed to production, and distribute
`kubectl-trace` with our standard development tools. This makes it easy for
developers to probe their applications, and the system faculties that support
them.

These tools, along with the bcc suite of tools, allow for easier access to
otherwise complicated things, like kernel kprobes and uprobes. `bpftrace`, in
particular, allows for simple and concise probe definitions, and is great for
prototyping more complex tools, and poking around to find useful data sources.

## memcached sources

My colleague Camilo Lopez [@camilo-github] came up with the idea to attach a
uprobe to the `process__command` funtion in memcached. In the memcached source
code, we can its signature in `memcached.c`:

```{.c include=src/memcached/memcached.c startLine=5756 endLine=5756}
```

This shows us that the second argument (`arg1` if indexd from 0) is the command
string, which contains the key.

Now that we know the signature, we can verify thatwe  can find this symbol in
the memcached binary (substitute the specific pid and path if you're attempting
to reproduce this):

```
objdump-tT /proc/129701/root/usr/local/bin/memcached | grep process_command

```
[^1]

Which shows us that it is indeed a symbol we can access:
```.gnuassembler
...
00000000000155a5 l     F .text  0000000000000337 process_lru_command
00000000000158dc l     F .text  00000000000003b8 process_extstore_command
0000000000015c94 l     F .text  00000000000012ac process_command <--- Target
000000000001799a l     F .text  000000000000019d try_read_udp
0000000000017b37 l     F .text  00000000000002c7 try_read_network
...
```

Now we know that we can target this symbol with `bpftrace`.

## uprobe prototype

To probe read the commands issued to memcached, we can target the binary
directly[^2], and insert a breakpoint at this address. When the breakpoint is
hit, our eBPF probe is fired, and then `bpftrace` can read the data from it.

The simplest possible would just read the command and print it, which can
easily be done as a `bpftrace` one-liner:

```awk
bpftrace -e 'uprobe:/proc/896719/root/usr/local/bin/memcached:process_command { printf("%s\n", str(arg1)) }'
```

Then, if I issue a test command, I can  see the output!:

```
Attaching 1 probe...
set memtier-3652115 0 60 4
```

Great! Now, to quickly turn this into a useful tool, I can add the keys to a
dictionary, using the `++` operator to count each time the command is called:

```{.awk include=src/uprobe-v1.bt}
```

When this exits, it will print a map of the top run commands, which should
correspond to the most frequently accessed keys.

With a working uprobe prototype, my attention now turns to getting better
quality data. `bpftrace` doesn't really have the faculty to parse strings at
the moment, so it is better if arguments are passed of a known type. This
problem is solved by the USDT tracepoint protocol, established by the popular
use of dtrace.

[^1]: Using docker or crictl, we can find the container process and inspect its
    children to find the memcached process. This method then uses the `/root`
    handle to access the process's mount namespace, and read the exact instance
    of the memcached binary we want to probe.

[^2]: The initial prototype of the uprobe tool targetted the memcached binary
    directly, as while we were using a recent version of bpftrace (0.9.2), which
    ships with Ubuntu Eoan, it was linked with libbcc 0.8.0, which didn't have
    all of the USDT functionality and namespace support to read containerized
    processes correctly. For this reason
