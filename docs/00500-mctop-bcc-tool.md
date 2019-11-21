# BCC

To make a fully-featured port of mctop, bpftrace wouldn't quite fit the bill,
as it doesn't have quite the same flexibility as python when it comes to
post-processing data.

## USDT example from reference guide overview

Explain bit by bit what it is doing

FIXME link to reference guide in bibliography

```{.c include=src/bcc/docs/reference_guide.md startLine=234 endLine=241}
```



## other BCC tools 

## slabratetop.py


FIXME 
```{.awk include=src/bpf-perf-tools-book/originals/Ch14_Kernel/slabratetop.bt startLine=39 endLine=40}
```

## ucalls.py

To see an example of using USDT probes in the wild, I checked out ucalls.py,
which is the script that powers `rubycalls` and other USDT tools in bcc. In
does a little bit of meta programming, so that it can share the same code
across languages. For instance, for ruby it sets the probe points here:

```{.python include=src/bcc/tools/lib/ucalls.py startLine=77 endLine=82}
```

Then later, in the C code, it uses these to replace `READ_CLASS` and
`READ_METHOD` when it is building out the probe function:


```{.c include=src/bcc/tools/lib/ucalls.py startLine=138 endLine=160}
```

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

Bitwise trick going off `KEY_MAX_LENGTH`

```{.c include=src/memcached/memcached.h startLine=39 endLine=40}
```


## Read to null term

## Diving into memcached code

https://github.com/memcached/memcached/blob/master/memcached_dtrace.d#L214



## Degarbling in userspace

# Debug

```
sudo cat /sys/kernel/debug/tracing/trace_pipe
```
