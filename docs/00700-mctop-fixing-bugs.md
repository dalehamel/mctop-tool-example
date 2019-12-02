# Issues porting to bcc

In moving from the `bpftrace` prototype to a fully-fledged `bcc`-based Python
tool, inevitably there were some issues. `bpftrace` does a lot of smart stuff
under the hood, and basically does the equivalent of writing these C-based
segments of the eBPF probes for you, using LLVM IR (intermediate
representation).

In moving to writing the raw C to generate the eBPF code, there were a couple
of hiccups as in the form of rough edges that aren't as friendly as the
faculties that `bpftrace` provides in its higher-level tracing language.

## Debugging

To start off, To be able to print data in a way that can be readily used in
debugging scenarios, we can use the builtin `bpf_trace_printk` which is a
printf-like interface. To read these values out of the kernel:

```
sudo cat /sys/kernel/debug/tracing/trace_pipe
```

## Being able to read the data

The original eBPF trace function was based on the sample code. I just read
the Dtrace spec for `command__set`, to determine the argument ordering for the
and type information:

```{.c include=src/mctop-basic/tools/mctop.py startLine=67 endLine=90}
```

This basic probe was printing data for the key! But it wasn't reading anything
for the size parameter that I was trying to read, which I needed in order to
replicate the key size feature of the original `mctop`.

The calls to `bpf_usdt_readarg` are reading the parameter into a 64 bit
container. Sometimes this is for literal values, and sometimes it is for
addresses. Reading literal values is easy and efficient, they are simply
copied into the address passed in as the third argument, as the `&` operator is
used for. This is why the `u64 keystr = 0, bytecount = 0;` is in the code, to
declare the sizes of these storage containers as 64 bits, unsigned.

In `bpftrace`, almost all storage is done in 64 bit unsigned integers like
this, and it is a pretty normal standard to just use a container that is
the size of a machine word on modern microprocessors. This is because typing
information is handled differently.

As it turns out, for reading types properly, it is best with bcc to match the
storage class to the argument type you are trying to read, otherwise the
result you get on a type mismatch when you try to do the probe read may be 0.

To fix this problem, which is something encountered before
[@usdt-report-doc] in my work on Ruby USDT tracing, I turned to the Systemtap
wiki page [@stap-wiki-ust] for an explanation on the elf note format, which I
was familiar with from working with `libstapsdt`.

The command `readelf --notes` can be used to show the probe addresses that are
added by the `systemtap` dtrace compatibility headers, supplying `sys/sdt.h` to
Linux. The output in this case shows:

```{.gnuassembler include=src/elfnotes.txt startLine=49 endLine=53}
```

The argument signature token to the left[^17] of the @ symbol is what can be used
to decode the type.

```
Arguments: -4@%edx 8@%rsi 1@%cl -4@%eax 8@-24(%rbp)
```

Using the table from the systemtap wiki:

| Arg code | Description      | Storage Class |
|----------|:-----------------| ------------- |
| 1 ...    | 8 bits unsigned. | `uint8_t` ....|
|-1 ...    | 8 bits signed.   | `int8_t`  ....|
| 2 ...    | 16 bits unsigned.| `uint16_t`....|
|-2 ...    | 16 bits signed.  | `int16_t` ....|
| 4 ...    | 32 bits unsigned.| `uint32_t`....|
|-4 ...    | 32 bits signed.  | `int32_t` ....|
| 8 ...    | 64 bits unsigned.| `uint64_t`....|
|-8 ...    | 64 bits signed.  | `int64_t` ....|

[@stap-wiki-ust]

We can decode this as:

| Arg index | Args from ELF notes |  Storage Class  |
|-----------|---------------------|-----------------|
|    0 .... | -4@...              | `int32_t`  ...  |
|    1 .... |  8@...              | `uint64_t` ...  |
|    2 .... |  1@...              | `uint8_t`  ...  |
|    3 .... | -4@...              | `int32_t`  ...  |
|    4 .... |  8@...              | `uint64_t` ...  |

So, we can take it that the 4th argument to `command__set`'s probe call is
actually meant to be stored in a signed 32-bit int!

Adjusting the code accordingly to match the proper storage class, data can now
be read by the probe:

```{.c include=src/mctop-read-bytes/tools/mctop.py startLine=77 endLine=96}
```

Note that only the `bytecount` variable needed to be changed to `int32_t`, as
it only matters for the read - the struct member used to store this can remain
u64, as the copy operation will pull this smaller type into a larger storage
class without truncation.

## Duplicate keys?

Now that probe data could be read, the UI could be replicated.

In initial testing, there was a confusing bug where the same key was printed
multiple times. It was iterating over a map where these apparently identical
keys were supposed to be hashed to the same slot.

After finding no bug in the displays code, it seemed that the keys must
actually not be the same, even though they looked to be when they were printed
to the screen. Some other unintended data must have been making it into the 
string buffer, and "garbling the keys".

Early tests were mostly with one request at a time, but once this was scripted
to increase the call rate and vary the keys, the pattern became much more
obvious.

In the case of the string, its `const char *` signature, which would hint at
a string though is possibly a byte array. The earlier use of
`process__command`, had `const void *` in its signature, which would be
standard for indicating arbitrary data.

Both signatures often refer to byte arrays of either binary or string data
though, so the context of the data received here depends on the context that
the probe is calling it from.

In either case, it is necessary to read this data into a buffer. For this a
buffer is declared inside of a struct for `keystr`:

```{.c include=src/mctop-basic/tools/mctop.py startLine=63 endLine=65}
```

This allows for the members of this struct to be easily mapped as members of
Python objects. Inside the probe, the struct is initialized to 0:

```{.c include=src/mctop-basic/tools/mctop.py startLine=77 endLine=77}
```

This is used to received the data is copied into the character buffer `keystr`
on the `keyhit_t` struct:

```{.c include=src/mctop-basic/tools/mctop.py startLine=83 endLine=83}
```

An attempt was made to use `bcc` function `bpf_probe_read_str`  the
documentation indicates is able to read a buffer of a fixed size until it
finds a null-byte, which seemed to be a fitting solution the problem.[^14]

This worked more reliably and there were fewer errors, but when benchmarking
was done at much higher call rates, it became clear that some of the payload
must be making it into the buffer for the key read. This indicated that it was
reading too many bytes, and collecting data from the adjacent memory space.

## Memcached key read quirks

Originally I thought this might be a bug, so I filed an upstream issue
[@memcached-dtrace-issue]. Despite the argument being of type `const char *`,
in this instance, it wasn't guaranteed to be a null terminated string.

Where the probe is called, it is using the macro `ITEM_key` to get the value
that is passed to the Dtrace macro:

```{.c include=src/memcached/memcached.c startLine=1378 endLine=1378}
```

This macro is just getting the address of the start of the data segment, and
clearly isn't copying a string into a null-terminated buffer:

```{.c include=src/memcached/memcached.h startLine=116 endLine=116}
```

So this meant that when we do the `bpf_probe_read_str` in `bcc`, it will read
the full size of the buffer object for its read, and can blow past the actual
length of the key data! It turns out that if using `bpf_probe_read_str`, it
never finds a null byte, and so will also just read the whole buffer.

As I learned, Memcached doesn't necessarily store keys as null terminated
strings, or even string data at all - it is arbitrary binary bytes. This is why
it passes the argument `keylen` in the USDT probe, so that the correct size of
the key can be read. Using the same process as above, I determined that the
`keylen` argument was actually stored as a `uint8_t`, and was able to get the
key length easily enough. This was stored as `keysize`.

### De-garbling in Userspace

Unfortunately, using this read `keysize` wasn't trivial, as it produced a
verifier error if it was passed as an argument to the probe read function.
This seemed to be because it was not a const or provably safe value.

To prevent this from blocking the development of the rest of the tool, the
`keysize` value was passed into userspace by adding it as a field to the
value-data struct. This would enable de-garbling this data in Python.

This meant that the same key could be hashed to multiple slots, as they would
include whatever arbitrary data is after the key in the buffer that is read.

In hindsight, this behavior of passing a buffer and a length to read seems to
have been intentional for Memcached. Not performing a string copy is more
efficient, which is why the probe just submits the buffer and the length of the
data to read.


To resolve this, a Python workaround was used to combine the keys in userspace:


```{.python include=src/mctop-garbled/tools/mctop.py startLine=148 endLine=170}
```

This just added or replaced values as necessary, using a timestamp to take the
more recent between the two if values were being replaced.

This was sufficient to finish the prototype, while a solution to the verifier
issue could be worked on.

### Different signatures for USDT args

Most of the development was done by testing only with the `SET` command,
because this was the most convenient. Once there was a more fully-featured
tool, attention turned to tracing the other commands. As they all had the same
signatures according to their Dtrace probe definitions, it was assumed to be an
easy task to iterate over all of the commands to be traced.

There was a nasty little surprise right away when implementing `GET`. As it
turns out, it can have **more than one argument signature**, depending where
it is called from:

```{.gnuassembler include=src/elfsignatures.txt startLine=28 endLine=33}
```

This manifested as the `GET` requests frequently returning 0 for `keylen`, as
it could be stored in either a `uint8_t` **OR** `a uint64_t`.

To get around this, checking if the value was 0 and trying to read again with
a different (larger) storage class resulted in actually reading a value.

[^14]: Dormando [@dormando] mentioned in [@memcached-dtrace-issue]

[^17]: The bit after the @ symbol seems to be the register to read this from.
       It also looks like it is able to specify offsets relative to the frame
       pointer, so this probably is based on the platform calling convention,
       denoting the offset in the stack and size to read.
