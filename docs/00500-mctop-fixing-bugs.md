# Issues developing bcc tool

## Being able to read the data

Could read key, but not the data size - what?!

explain reading the elf notes, put a table for the systemtap page with the
type descriptions.

Show the code diffs, debugging this with printk and catting trace pipe

## Duplicate keys

Why is it showing the same key?

(solution was to use read_str), FIXME

## Garbled keys

Reading full buffer


Python workaround to combine the keys in userspace:

```python

```

### Degarbling in userspace

FIXME put summary of the python hackery here

### Convincing the verifier of safety

Since this works in bpftrace, as it can do variable length-reads, I thought it
would make sense to take a look at how this is handled. Looking at the bpftrace
code generation:

```{.cpp include=src/bpftrace/src/ast/codegen_llvm.cpp startLine=413 endLine=441}
```

We can see that it generates the LLVM IR for doing a comparison between they
size parameter given, and the maximum size. This is sufficient for it to pass
the eBPF verifier in the linux kernel.

I attempted to do something similar in my probe definition, as described in
bcc issue 1260 // FIXME show example and link to this issue

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



## Diving into memcached code

https://github.com/memcached/memcached/blob/master/memcached_dtrace.d#L214



# Debug

```
sudo cat /sys/kernel/debug/tracing/trace_pipe
```
