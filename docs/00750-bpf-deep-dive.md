# eBPF deep dive

This section gets into the eBPF code disassembly in order to explain how
to structure probes to ensure they will be accepted by the kernel's BPF
verifier.

## Verifier error with variable read

With a working replacement of all of the basic `mctop` functionality, the
priority became to try and fix the garbled keys at the right layer: in the
eBPF probe. Bas Smit [@fbs] pointed out on IRC that non-const probe reads for
string data already a problem `bpftrace` had solved.

This gave some renewed hope that there **must** be a way to get the eBPF
verifier to accept a non-const length read.

Knowing that this works in `bpftrace`, it would make sense to take a look at how
this is handled there. This is the relevant LLVM IR generation procedure from
`bpftrace`:

```{.cpp include=src/bpftrace/src/ast/codegen_llvm.cpp startLine=413 endLine=441}
```

This generates the LLVM IR for doing a comparison between the size parameter
given, and the maximum size. This is sufficient for it to pass the eBPF
verification that this is a safe read and can run inside the in-kernel
BPF virtual machine.

Taking inspiration from an existing issue for this in `bcc`, the probe
definition, as described in iovisor/bcc#1260 [@bcc-variable-read-issue-comment]
to include a logical assertion that the `keysize` must be smaller than the
buffer size via a ternary.

This didn't work unfortunately, and it threw this eBPF verifier error:

```{.gnuassembler include=src/ebpf-error.txt startLine=136 endLine=142}
```

As will be shown later, this message is more helpful than it initially seems,
but at the time these values of -80 and 255  didn't seem significant, and it
wasn't clear what was meant by an invalid stack offset, as this code was
generated and difficult to associate back to the C code which resulted in it.

## Safe Code Generation

A comment[@bpf-variable-memory] on iovisor/bcc#1260, provided a hint towards a
mechanism which could be used to demonstrate safety for passing a non-const
length value to the probe read. In the commit message, this C snippet is used:

```{.c include=src/kernel-safety.c}
```

That showed that a bitwise AND with a const value was enough to convince the
verifier that this was safe! Of course, this only really be easy if the const
value as a hex mask with all bits set, like `0xFF`.

In the Memcached source, we can see that `KEY_MAX_LENGTH` is `250`. This is
close enough to 255 that a mask of `0xFF` could be applied:

```{.c include=src/memcached/memcached.h startLine=39 endLine=40}
```

By just setting the buffer size to 255, the maximum that will fit in a single
byte, the verifier is now able to determine that no matter what value is read
from `keylen` into `keysize`, it will be safe, and that a buffer overflow
cannot be possible.

The binary representation of 0xFF (255 decimal) is `1111 1111`. To test this
theory, the most significant bit can be flipped to 0, to get `0111 1111`.
Back to hexadecimal, this is 0x7F, and in decimal this is 127. By manually
comparing the `keysize` with this mask via a bitwise AND, it works and is
accepted by the verifier! If, however, the size of the buffer is dropped to
just 126, there is the familiar verifier error once again.

The reason why this happens is visible in the disassembly of the generated eBPF
program:

```{.gnuassembler include=src/crashing.disasm startLine=65 endLine=70}
```

By convention [@bpf-register-architecture], `R1` is used for the first argument
to the call of `bpf_probe_read` (built-in function "4"), and `R2` is used for
the second argument. `R6` is used as a temporary register, to store the value
of `R10`, which is the frame pointer.

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

The disassembly shows the buffer is initialized right at the start, putting
the struct initialization at the bottom of the stack. In the crashing version
there is a `uint16_t` and a `uint32_t` near the start of the stack:

```{.gnuassembler include=src/crashing.disasm startLine=5 endLine=22}
```

But in the non-crashing version, there is also a `uint8_t`:

```{.gnuassembler include=src/noncrashing.disasm startLine=5 endLine=23}
```

The difference is subtle, but comparing the space allocated on the stack, the 
crashing version allocates 15 `uint64_t` + 1 `uint32_t` + 1 `uint16_t`.
Converting this to bytes, this becomes (15 * 8 + 1 * 4 + 1 * 2) = 126 bytes
allocated.

In the non-crashing version, it is 15 `uint64_t` + 1 `uint32_t` + 1 `uint16_t`
+ 1 `uint8_t`. This works out to 127 bytes. So that verifier message for the
crashing program:

```gnuassembler
60: (85) call bpf_probe_read#4
invalid indirect read from stack off -128+126 size 127
```

Is complaining that the first argument, `R1`, which is set relative to the
frame pointer, is not of sufficient size to be certain that the value read in
`R2` (guaranteed by the bitwise AND operation to be no more than 127).

To summarize, there were two ways to solve this issue - either increase the
buffer size to 255 so that there was no way that the `uint8_t` container used
by `keysize` could possibly overflow it, or a bitwise AND the `keysize` value
with a hex-mask that is sufficient to prove it cannot be a buffer overflow.

This might seem like a pain, but this extra logic is the cost of safety. This
code will be running within the kernel context, and needs to pass the
verifier's pat-down. In the meantime, `libbpf` continues to improve to make
this sort of explicit proof of safety less necessary.
