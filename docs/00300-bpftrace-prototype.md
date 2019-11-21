# bpftrace on memcached

## memcached sources

 My colleague Camilo Lopez [@camilo-github] came up with the idea to attach a
uprobe to the `process__command` funtion in memcached.

In the memcached source code, we can see the signature for this method:

```{.c include=src/memcached/memcached.c startLine=5756 endLine=5756}
```

This shows us that the second argument (`arg1` if indexd from 0) is the command
string.

Now that we know the signature, we can see if we can find this symbol in the
memcached binary:

```
objdump-tT /proc/129701/root/usr/local/bin/memcached | grep process_command

```

Which shows us that it is indeed a symbol we can access:
```.gnuassembler
...
00000000000155a5 l     F .text  0000000000000337 process_lru_command
00000000000158dc l     F .text  00000000000003b8 process_extstore_command
0000000000015c94 l     F .text  00000000000012ac process_command
000000000001799a l     F .text  000000000000019d try_read_udp
0000000000017b37 l     F .text  00000000000002c7 try_read_network
...
```

From this, we can build a uprobe prototype of trying to access

## uprobe prototype

Initial prototype of tool

```{.awk include=src/uprobe-v1.bt}
```

