# Issues developing bcc tool

In moving from the `bpftrace` prototype to a fully-fledged bcc-based python
tool, inevitably I ran into some issues. `bpftrace` does a lot of smart stuff
under the hood, and basically does the equivalent of writing these C-based
segments of the eBPF probes for you, using LLVM IR (intermediate
representation).

In moving to writing the raw C to generate the eBPF code myself, I ran into a
couple of hiccups as I hit rough edges that weren't as friendly as the
faculties that `bpftrace` provides in its higher-level tracing language.

Going through these here will, I hope, give some insight into the ways that I
was able to find and squash bugs in order to develop a fully functioning tool,
so that others can learn from these mistakes.

## Debugging

To start off, To be able to print data in a way that can be readily used in
debugging scenarios, we can use the builtin `bpf_trace_printk` which is a
printf-like interface. To read these values out of the kernel:

```
sudo cat /sys/kernel/debug/tracing/trace_pipe
```

## Being able to read the data

// FIXME this needs to be filled in way more

Could read key, but not the data size - what?!

explain reading the elf notes, put a table for the systemtap page with the
type descriptions.

```{.gnuassembler include=src/elfnotes.txt startLine=104 endLine=108}
```

```{.gnuassembler include=src/elfnotes.txt startLine=24 endLine=28}
```


## Duplicate keys

Why is it showing the same key?

(solution was to use read_str), FIXME

## Garbled keys

Reading full buffer


Originally I thought this might be a bug, so I filed an upstream issue
[@memcached-dtrace-issue]. Despite the argument being of type `char *`, which
elsewhere in the dtrace probe definitions meant string data, in turned out that
in this instance, it wasn't guaranteed to be a null terminated string!

Where the probe is called, it is using the macro `ITEM_key` to get the value
that is fired to dtrace:

```{.c include=src/memcached/memcached.c startLine=1378 endLine=1378}
```

This macro is just getting the address of the start of the data segment, and
clearly isn't copying a string into a null-terminated buffer:

```{.c include=src/memcached/memcached.h startLine=116 endLine=116}
```

So this meant that when we do the `bpf_probe_read` in bcc, it will read the
full size of the buffer object for its read, and can blow past the actual
length of the key data! If using `bpf_probe_read_str`, it never finds a null
byte, and so will also just read the whole buffer.

// FIXME put printk output here

This lead to the keys being stored in the eBPF map being possible non-unique,
as they would be based on whatever arbitrary data is after the key in the
buffer that is read.

Luckily, this behavior seems to have actually been intentional for memcached.
Not performing a string copy is more efficient, which is why the probe just
submits the buffer and the length of the data to read.

Unfortunately though, When I try to use to use this parameter it throws an eBPF
verifier error! I'll get more into that later, but for the time being to
proceed with developing the script, I just decided to fix this problem in
userspace with python.

### Degarbling in userspace

Without a solution to read the correct length in the C eBPF probe, I decided to
just pass `keylen` to userspace on my struct, and let python merge keys where
there happened to be collisions.

Python workaround to combine the keys in userspace:

```python

```

FIXME put summary of the python hackery here

### Convincing the verifier of safety

Now that I had a working script, I decided to try and fix the garbled keys at
the right layer. Thanks to Bas Smit [@fbs] who pointed out to me that this was
actually a solved problem in bpftrace, I had some renewed hope that there
**must** be a way to get the eBPF verifier to accept a non-const length read.

Iknowing that this works in bpftraceI thought it would make sense to take a
look at how this is handled. Looking at the bpftrace code generation:

```{.cpp include=src/bpftrace/src/ast/codegen_llvm.cpp startLine=413 endLine=441}
```

We can see that it generates the LLVM IR for doing a comparison between they
size parameter given, and the maximum size. This is sufficient for it to pass
the eBPF verifier in the linux kernel.

I attempted to do something similar in my probe definition, as described in
iovisor/bcc#1260 [@bcc-variable-read-issue-comment].

That didn't work unfortunately, it threw this eBPF verifier error:

// FIXME show error

digging into the linux kernel, I could see that this was the code being called:

```{.c include=src/linux/kernel/bpf/verifier.c startLine=2933 endLine=2954}
```

So without being an expert in the eBPF verifier or the linux kernel, I figured
that this further re-enforced that the issue was because the length I was
providing for the read was non-const.

There was another comment on that bcc which referred to [@bpf-variable-memory]
a mechanism which could be used to demonstrate safety for a non-const length
value to the verifier. In the commit message, this C snippet is used:

```c
int len;
char buf[BUFSIZE]; /* BUFSIZE is 128 */

if (some_condition)
  len = 42;
else
  len = 84;

some_helper(..., buf, len & (BUFSIZE - 1));
```

That showed that a bitwise & with a const value was enough to convince the
verifier that this was safe! Of course, this only really work for sure if the
const value as a hex mask with all bits activated, like `0xFF`

In the memcached source, we can see that `KEY_MAX_LENGTH` is `250`. This is
close enough to 255 that a mask of `0xFF` should work fine:

```{.c include=src/memcached/memcached.h startLine=39 endLine=40}
```

// FIXME show updated probe code
