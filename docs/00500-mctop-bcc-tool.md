# BCC

To make a fully-featured port of mctop, bpftrace wouldn't quite fit the bill,
as it doesn't have quite the same flexibility as python when it comes to
post-processing data.

## USDT example from reference guide overview

Explain bit by bit what it is doing

## other BCC tools 

## tcptop.py

## ucalls.py

## Being able to read the data

Could read key, but not the data size - what?!

explain reading the elf notes, put a table for the systemtap page with the
type descriptions.

Show the code diffs, debugging this with printk and catting trace pipe

## Garbled keys

Reading full buffer

## Duplicate keys

Why is it showing the same key?

## Read to null term

## Diving into memcached code

https://github.com/memcached/memcached/blob/master/memcached_dtrace.d#L214



## Degarbling in userspace

# Debug

```
sudo cat /sys/kernel/debug/tracing/trace_pipe
```
