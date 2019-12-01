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

In `bpftrace`, almost all storage is done in 64 bit unsigned integers like
this, and it is a pretty normal standard to just use a large container
like this that is the size of a machine word on modern microprocessors.
This is because typing information is handled differently.

As it turns out, for reading types properly, it is best with bcc to match the
storage class to the argument type you are trying to read, otherwise the
result you get on a type mismatch when you try to do the probe read may be 0.

To fix this problem, which is something I have encountered before
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

In my initial testing, I thought I was going crazy as I was seeing the same key
printed multiple times, as I was iterating over a map where this is supposed to
be hashed to the same slot. After looking for a bug in the display code, I
realized that the keys must not be the same, even though they looked to be when
they were printed to the screen: some other unintended data must have been
making it into the string buffer, and "garbling the keys".

I had mostly been testing with one request at a time, but once I started to
script sending requests to the test Memcached instance, the pattern became
much more obvious.

In the case of the string, it is meant to be a pointer from its `const char *`
type signature, though earlier in `process__command`, it is `const void *`.
Both signatures often refer to byte arrays of either binary or string data.
This is for argument. In either case, it is necessary to read this data into a
buffer. For this we declare a buffer inside of a struct:

```{.c include=src/mctop-basic/tools/mctop.py startLine=63 endLine=65}
```

And then the struct is likewise initialized:

```{.c include=src/mctop-basic/tools/mctop.py startLine=77 endLine=77}
```

And finally the data is copied into the character buffer `keystr` on the
`keyhit_t` struct:

```{.c include=src/mctop-basic/tools/mctop.py startLine=83 endLine=83}
```

I also tried to use the `bcc` function `bpf_probe_read_str`, which I saw from
the documentation was able to read a buffer of a fixed size until it found a
null byte, which seemed perfect for this circumstance.[^14]

It seemed to work some of the time, but when I benchmarked it I could see that
some of the payload data was clearly making it into the key - it was reading
too many bytes, and collecting data from the adjacent memory space.

## Nope. Garbled keys.

Originally I thought this might be a bug, so I filed an upstream issue
[@memcached-dtrace-issue]. Despite the argument being of type `char *`, which
elsewhere in the dtrace probe definitions meant string data, in turned out that
in this instance, it wasn't guaranteed to be a null terminated string!

Where the probe is called, it is using the macro `ITEM_key` to get the value
that is fired to `dtrace`:

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

As I learned, Memcached doesn't necessary store keys as null terminated
strings, or even string data at all - it is arbitrary binary bytes. This is why
it passes the argument `keylen` in the USDT probe, so that the correct size of
the key can be read. Using the same process as above, I determined that the
`keylen` argument was actually stored as a `uint_8`, and was able to get the
key length easily enough.

### Degarbling in Userspace

Unfortunately, I wasn't actually able to use the `keylen`, as I got a verifier
error if I tried to pass it, as it was determined to not be a const or provably
safe value. To prevent this from blocking the development of the rest of the
tool, I decided to pass the `keylen` value into userspace by adding it as a
field to the value-data struct, and wrote a `degarbling` method in Python.

This meant that the same key could be hashed to multiple slots, as they would
include whatever arbitrary data is after the key in the buffer that is read.

In hindsight, this behavior of passing a buffer and a length to read seems to
have been intentional for Memcached. Not performing a string copy is more
efficient, which is why the probe just submits the buffer and the length of the
data to read.


Python workaround to combine the keys in userspace:


```{.python include=src/mctop-garbled/tools/mctop.py startLine=148 endLine=170}
```

This just added or replaced values as necessary, using a timestamp to take the
more recent between the two if values were being replaced.

This was sufficient to finish the prototype, while a solution to the verifier
issue could be worked on.

### Convincing the verifier of safety

With a working replacement of all of the basic `mctop` functionality, the priority
became to try and fix the garbled keys at the right layer. Thanks to Bas Smit
[@fbs] who pointed out to me that this was actually a solved problem in
bpftrace, I had some renewed hope that there **must** be a way to get the eBPF
verifier to accept a non-const length read.

Knowing that this works in bpftrace, it would make sense to take a look at how
this is handled there, as that was a codebase I was already familiar with. This
is the relevant LLVM IR generation from `bpftrace`:

```{.cpp include=src/bpftrace/src/ast/codegen_llvm.cpp startLine=413 endLine=441}
```

We can see that it generates the LLVM IR for doing a comparison between the
size parameter given, and the maximum size. This is sufficient for it to pass
the eBPF verification of a safe read and run inside in-kernel BPF virtual
machine.

Looking into an existing issue for this in `bcc`, I attempted to do something
similar in my probe definition, as described in iovisor/bcc#1260
[@bcc-variable-read-issue-comment].

This didn't work unfortunately, it threw this eBPF verifier error:

```{.gnuassembler include=src/ebpf-error.txt startLine=136 endLine=142}
```

As will be shown later, this message is more helpful than it seems, but at the
time I ignored these values of -80 and 255 as I couldn't tell their
significance, or what was meant by an invalid stack offset.

A comment[@bpf-variable-memory] on iovisor/bcc#1260, provided a hint towards a
mechanism which could be used to demonstrate safety for a non-const length
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
verifier that this was safe! Of course, this only really be easy if the const
value as a hex mask with all bits activated, like `0xFF`

In the Memcached source, we can see that `KEY_MAX_LENGTH` is `250`. This is
close enough to 255 that a mask of `0xFF` would be able to mask:

```{.c include=src/memcached/memcached.h startLine=39 endLine=40}
```

By just setting the buffer size to 255, the maximum that will fit in a single
byte, the verifier is now able to determine that no matter what value is read
from `keysize`, it will be safe, and that a buffer overflow cannot be possible.

This change permits the verifier to allow the read of `keysize` without any
extra checking required, as there is no possible way it can cause a buffer
overflow.

The binary representation of 0xFF (255 decimal) is `1111 1111`. To test this
theory, I decided to flip that last bit to 0, to get `0111 1111`. Back to
hexadecimal, this is 0x7F, and in decimal this is 127. By manually masking the
`keysize` with this mask, it works! If, however, I drop the size of the buffer
to 126, I get a verifier error once again.

The reason why this happens is visible in the disassembly of the generated eBPF
program:

```{.gnuassembler include=src/crashing.disasm startLine=65 endLine=70}
```

By convention [@bpf-register-architecture], `R1` is used for the first argument
to the call of `bpf_probe_read` (builtin function "4"), and `R2` is used for the
second argument. `R6` is used as a temporary register, to store the value of
`R10`, which is the frame pointer. From earlier in the disassembly code, we can
see where the byte array is initialized.

| Register | x86 reg | Description                   |
|----------|---------|-------------------------------|
| R0       | rax     |    return value from function |
| R1       | rdi     |    1st argument               |
| R2       | rsi     |    2nd argument               |
| R3       | rdx     |    3rd argument               |
| R4       | rcx     |    4th argument               |
| R5       | r8      |    5th argument               |
| R6       | rbx     |    callee saved               |
| R7       | r13     |    callee saved               |
| R8       | r14     |    callee saved               |
| R9       | r15     |    callee saved               |
| R10      | rbp     |    frame pointer              |


In the crashing version in looks like this:

```{.gnuassembler include=src/crashing.disasm startLine=5 endLine=22}
```

But in the non-crashing version in looks like this:

```{.gnuassembler include=src/noncrashing.disasm startLine=5 endLine=23}
```

The difference is subtle, but we can see that the crashing version
allocates 15 u64 + 1 u32 + 1 u16. Converting this to bytes, we get (15 * 8 +
1 * 4 + 1 * 2) = 126 bytes allocated.

In the non-crashing version, it is 15 u64 + 1 u32 + 1 u16 + 1 u8. In bytes,
this works out to 127 bytes. So that verifier message for the crashing program:

```gnuassembler
60: (85) call bpf_probe_read#4
invalid indirect read from stack off -128+126 size 127
```

Is complaining about the first argument, `R1`, which is set from `R10`, not
being of sufficient size to be certain that the value read in `R2` (guaranteed
by the bitwise AND operation to be no more than 127).

To summarize, there were two ways to solve this issue - either increase the
buffer size to 255 so that there was no way that the `uint8_t` container used
by `keysize` could possible overflow it, or bitwise AND the `keysize` value
with a hex-mask that is sufficient to prove it cannot be a buffer overflow.

This might seem like a pain, but this extra logic is the cost of safety - this
code will be running within the kernel context, and needs to pass the
verifier's pat-down, as `libbpf` improves.

## Different signatures for USDT args

During most of my testing, I tested only against the `SET` command, because
this was convenient. Once I had a more fully-featured tool, I turned my
attention to tracing the other commands. As I knew they all had the same
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
[^17]: The bit after the @ symbol seems to be the register to read this from.
       It also looks like it is able to specify offsets relative to the frame
       pointer, so this probably is based on the platform calling convention,
       denoting the offset in the stack and size to read.
