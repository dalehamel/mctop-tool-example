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

The original eBPF trace function was based on the sample code. I just read
the dtrace spec for `command__set`, to determine the argument ordering for the
and type information:

```{.c include=src/mctop-basic/tools/mctop.py startLine=67 endLine=90}
```

This basic probe was printing data for the key! But it wasn't reading anything
for the size parameter that I was trying to read, which I needed in order to
replicate the key size feature of the original `mctop`.

The calls to `bpf_usdt_readarg` are reading the parameter into a 64 bit
container. Sometimes this is for literal values, and sometimes it is for
addresses. Reading literal values is easy, they are simply copied into the
address passed in as the third argument, as the `&` operator is used for. This
is why the `u64 keystr = 0, bytecount = 0;` is above, to declare the sizes of
these storage containers as 64 bits, unsigned.

In bpftrace, almost all storage is done in 64 bit uints like this, and it is
a pretty normal standard to just use a large container like this that is a
size of a machine word on modern microprocessors. This is because typing
information is handled differently.

As it turns out, for reading types properly, it is best with bcc to match the
storage class to the argument type you are trying to read, otherwise the
result you get on a type mismatch when you try to do the probe read may be 0.

To fix this problem, which is something I have encountered before
[@usdt-report-doc] in my work on Ruby USDT tracing, I turned to the systemtap
wiki page for an explanation on the elf note format, which I was familiar with
from working with `libstapsdt`.

The command `readelf --notes` can be used to show the probe addresses that are
added by the `systemtap` dtrace compatibility headers, supplying `sys/sdt.h` to
Linux. The output in this case shows:

```{.gnuassembler include=src/elfnotes.txt startLine=49 endLine=53}
```

From this we can look closer at the argument signature:

```
Arguments: -4@%edx 8@%rsi 1@%cl -4@%eax 8@-24(%rbp)
```

Using the table from the systemtap wiki:

| Arg code | Storage Class    |
|----------|:-----------------|
| 1        | 8 bits unsigned  |
|-1        | 8 bits signed    |
| 2        | 16 bits unsigned |
|-2        | 16 bits signed   |
| 4        | 32 bits unsigned |
|-4        | 32 bits signed   |
| 8        | 64 bits unsigned |
|-8        | 64 bits signed   |

[@stap-wiki-ust]

We can decode this as:

| Arguments     | -4@%edx | 8@%rsi   |1@%cl    |-4@%eax  |8@-24(%rbp)|
|---------------|---------|----------|---------|---------|-----------|
| Storage Class |`int32_t`|`uint64_t`|`uint8_t`|`int32_t`|`uint64_t` |

So, we can take it that the 4th argument to `command__set`'s probe call is
actually meant to be stored in a signed 32-bit int!

Adjusting the code accordingly, data can be read by the probe:

```{.c include=src/mctop-read-bytes/tools/mctop.py startLine=77 endLine=96}
```

Note that only the `bytecount` variable needed to be changed to int32, as it
only matters for the read - the struct member used to store this can remain
u64, as the copy operation will copy into a larger storage class without
truncation.

## Duplicate keys?

Now that probe data could be read, the UI could be replicated.

In my initial testing, I thought I was going crazy as i was seeing the same key
printed multiple times, as I was iterating over a map where this is supposed to
be hashed to the same slot. Upon closer examination, I realized that the keys
must not be the same, even though they looked to be when they were printed.

I had mostly been testing with one request at a time, but once I started to
script sending requests to my test memcached instance, the pattern became much
more obvious.

In the case of the string, we can see that is meant to be a pointer
from its `const char *` type signature, though earlier in `process__command`,
it is `const void *`. Both signatures often refer to byte arrays of either
binary or string data. This is for argument. In either case, it is necessary
to read this data into a buffer. For this we declare a buffer inside of a
struct:

```{.c include=src/mctop-basic/tools/mctop.py startLine=63 endLine=65}
```

And then the struct is likewise initialized:

```{.c include=src/mctop-basic/tools/mctop.py startLine=77 endLine=77}
```

And finally the data is copied into the character buffer `keystr` on the
`keyhit_t` struct:

```{.c include=src/mctop-basic/tools/mctop.py startLine=83 endLine=83}
```

I also tried to use the bcc function `bpf_probe_read_str`, which I saw from the
documentation was able to read a buffer of a fixed size until it found a null
byte, which seemed perfect for this circumstance.[^14]


## Nope. Garbled keys.

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
length of the key data! It turns out that if using `bpf_probe_read_str`, it
never finds a null byte, and so will also just read the whole buffer. So it
turns out, memcached doesn't necessary store keys as null terminated strings.
This is why it passes the argument `keylen`. Using the same process as above,
I determined that the `keylen` argument was actually stored as a `uint_8`, and
was able to get the key length easily enough.

Unfortunately, I wasn't actually able to use the `keylen`, as I got a verifier
error if I tried to pass it, as it was determined to not be a const value, and
thus potentially unsafe. I decided to just soldier on and pass the `keylen`
on into userspace via the struct, and wrote a `degarbling` method, that could
be used to trim the keys read from the map and combine them to as they were
meant to be:

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


```{.python include=src/mctop-garbled/tools/mctop.py startLine=148 endLine=170}
```

This just added or replaced values as necessary, using a timestamp to take the
more recent between the two if values were being replaced.

This was sufficient to finish the prototype, while a solution to the verifier
issue could be worked on.

### Convincing the verifier of safety

With a working replacement of all of the basic `mctop` functionality, I decided
to try and fix the garbled keys at the right layer. Thanks to Bas Smit [@fbs]
who pointed out to me that this was actually a solved problem in bpftrace, I
had some renewed hope that there **must** be a way to get the eBPF verifier to
accept a non-const length read.

Knowing that this works in bpftrace, it would make sense to take a look at how
this is handled. This is the relevant LLVM IR generation from `bpftrace`:

```{.cpp include=src/bpftrace/src/ast/codegen_llvm.cpp startLine=413 endLine=441}
```

We can see that it generates the LLVM IR for doing a comparison between they
size parameter given, and the maximum size. This is sufficient for it to pass
the eBPF verifier in the linux kernel.

I attempted to do something similar in my probe definition, as described in
iovisor/bcc#1260 [@bcc-variable-read-issue-comment].

That didn't work unfortunately, it threw this eBPF verifier error:

```{.gnuassembler include=src/ebpf-error.txt startLine=136 endLine=142}
```

The corresponding code in the linux eBPF verifier shows where this is emitted
from:

```{.c include=src/linux/kernel/bpf/verifier.c startLine=2933 endLine=2954}
```

So without being an expert in the eBPF verifier or the linux kernel, this
result re-enforced that the issue was because the length being provided for
the probe read was non-const.

Another comment[@bpf-variable-memory] on iovisor/bcc#1260, provided a hint
towards  a mechanism which could be used to demonstrate safety for a non-const
length value to the verifier. In the commit message, this C snippet is used:

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
verifier that this was safe! Of course, this only really be easy if the const
value as a hex mask with all bits activated, like `0xFF`

In the memcached source, we can see that `KEY_MAX_LENGTH` is `250`. This is
close enough to 255 that a mask of `0xFF` would be able to mask:

```{.c include=src/memcached/memcached.h startLine=39 endLine=40}
```
So this meant that the mask of `0xFF` or `1111 1111` in binary would be able
to verify with a bitwise `&` operation that a given `uint8_t` would be less
than or equal to 255. This would change the unknown value read from the probe,
to one that the verifier has compared with a mask to assert it is of a bounded
size.

This lead to the final probe code looking like:

```{.c include=src/bcc/tools/mctop.py  startLine=130 endLine=130}
```

As indicated, this passed the verifier!

// FIXME show the change in eBPF generated code and explain it.

## Different signatures for USDT args

During most of my testing, I tested only against the `SET` command, because
this was convenient. Once I had a more fully-featured tool, I turned my
attention to tracing the other commands. As I new they all had the same
signature from the `dtrace` probe definition, it was assumed to be an easy task
of just iterating over all of the commands to trace.

There was a nasty little surprise when I implemented `GET`. As it turns out,
it has **more than one argument signature**:

```{.gnuassembler include=src/elfsignatures.txt startLine=28 endLine=33}
```

This manifested as the `GET` requests returning 0 for `keylen` frequently, as
it could be stored in either a `uint8_t` **OR** `a uint64_t`.

To get around this, checking if the value was 0 and trying to read again with
a different (larger) storage class resulted in actually reading a value.


[^14]: Dormando [@dormando] mentioned in [@memcached-dtrace-issue]
