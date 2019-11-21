# Memcached Static Tracepoints

Looking through the memcached source, I was able to quickly see that there were
dtrace probes! This meant it would be very likely i'd be able to pull some
useful data out of memcached. Both `bpftrace` and `dtrace` can probe USDT
tracepoints, so lots of applications have support for dtarce builtin

```{.c include=src/memcached/memcached.c startLine=1358 endLine=1386}
```

Unfortunately, when I checked a production memcached instance with `tplist`
`bpftrace -l 'usdt:* -p $(pidof memcached)`, I didn't see any probes. This meant
 that I would need to hijack our alpine build, which thankfully was in a
Dockerfile and already from source, and add dtrace probes to it. If it had been
ubuntu, I would have just used the package manager. This one happened to be
alpine, but luckily the necessary package isn't actually binary! I was able to
grab and extract the systemtap compatibility libraries provided by ubuntu in
alpine linux trivially, so I imagine any linux distribution can follow this
process.

If you're interested, this package just installs the `sys/sdt.h` header, and a
stub command of `dtrace` that can be used to convert a dtrace file into a
generated header, providing the necessary macros to add tracepoints. The debian
archive is extracted, and the `/usr/bin/dtrace` shell stub and headers are
copied into the docker image at standard paths.

The Dockerfile that I used was based on a work one, which I've simplified for
release. The relevant addition to add dtrace probes was this snippet:

```{.c include=src/Dockerfile startLine=9 endLine=14}
```

Then on the configure line for memcached, I added `--enable-dtrace`:

```{.c include=src/Dockerfile startLine=54 endLine=60}

After this simply running `Docker build . -t memcached-dtrace` in this directory produces a
memcached image, 

During the configure process, I can see that it finds the dtrace stub:

...
checking for dtrace... /usr/bin/dtrace
...
```

Then it generates a header `memcached_dtrace.h`, which is conditionally
included when dtrace probes are enabled.

```bash
/usr/bin/dtrace -h -s memcached_dtrace.d
sed -e 's,void \*,const void \*,g' memcached_dtrace.h | \
            sed -e 's,char \*,const char \*,g' | tr '\t' ' ' > mmc_dtrace.tmp
mv mmc_dtrace.tmp memcached_dtrace.h
```

And later in the build process the shell stub is invoked again to process the
object files:

```bash
/usr/bin/dtrace  -G -o memcached_debug_dtrace.o -s ./memcached_dtrace.d memcached_debug-memcached.o memcached_debug-hash.o memcached_debug-jenkins_hash.o memcached_debug-murmur3_hash.o memcached_debug-slabs.o memcached_debug-items.o memcached_debug-assoc.o memcached_debug-thread.o memcached_debug-daemon.o memcached_debug-stats.o memcached_debug-util.o memcached_debug-bipbuffer.o memcached_debug-logger.o memcached_debug-crawler.o memcached_debug-itoa_ljust.o memcached_debug-slab_automove.o memcached_debug-authfile.o memcached_debug-restart.o memcached_debug-cache.o     memcached_debug-sasl_defs.o memcached_debug-extstore.o memcached_debug-crc32c.o memcached_debug-storage.o memcached_debug-slab_automove_extstore.o
```

So I can see that the dtrace support has been built into memcached. Now that the
image has been built, I  can start a memcached instance with I can try probing:

```
docker run -p 11211:11211 memcached-dtrace
```

To get the pid of that, I'll use:

```
MEMCACHED_PID=$(docker inspect --format '{{.State.Pid}}' memcached-dtrace)
```

Now I can run `tplist` from bcc, or use bpftrace to list the USDT tracepoints:

```
bpftrace -l 'usdt:*' -p ${MEMCACHED_PID}
```

I can now send a test 'set' command to my memcached instance. I chose one based
off the benchmarking tool `memtier`, which i'll use later. A simple invocation
based on standard shell tools is:

```
printf "set memtier-3652115 0 60 4\r\ndata\r\n" | nc localhost 11211
```

Now that I have usdt support, I can rebuild my original uprobe example. Checking
the dtrace file again, I see the signature for the `process__command` probe.

It just so happened that the function my colleague Camilo selected was also a
USDT probe point!

```{.c include=src/memcached/memcached_dtrace.d startLine=168 endLine=174}
```

Based on this, I built an initial translation of the uprobe tool to use USDT.


```{.awk include=src/mcsnoop-usdt.bt}
```

