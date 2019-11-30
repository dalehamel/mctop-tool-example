# Memcached Static Tracepoints

If an application supports USDT tracepoints already, it can be much easier to
trace it. It was a relief that when we went looking, we found dtrace probe
points in the `memcached` source.

The presence of this established pattern for comprehensive dtrace tracepoints
significantly simplified building a lightweight and simple tool. Examining the
`memcached` source, we can see exactly how these probes are invoked, and that
processing of these tracepoints at all is conditional on them being attached
to.

```{.c include=src/memcached/memcached.c startLine=1358 endLine=1386}
```

Unfortunately, when I checked a production `memcached` instance with `tplist`
or `bpftrace -l 'usdt:* -p $(pidof memcached)`, I didn't see any probes. This
meant that I would need to find a way to modify our `memcached` image to add
`dtrace` probes.

The build of `memcached` I was working with is based on alpine, which doesn't
have the systemtap compatibility headers. However, it was building `memcached`
from source already, which significantly simplified building a modified
`memcached` image with `dtrace` probes enabled.
 
The `Dockerfile` [@dockerfile] that used was based on a production configuration
which has been simplified for this analysis. The relevant addition to add
dtrace probes was this snippet:

```{.bash include=src/docker/Dockerfile startLine=9 endLine=14}
```

Though the package is being pulled from ubuntu, all we need is a few files.
This package just installs the `sys/sdt.h` header, and a stub command of
`dtrace` that can be used to convert a dtrace file into a generated header,
providing the necessary macros to add tracepoints. The Debian archive is
extracted, and the `/usr/bin/dtrace` shell stub and headers are copied into
the docker image at standard paths.

Then on the configure line for `memcached`, just adding `--enable-dtrace` was
sufficient:

```{.bash include=src/docker/Dockerfile startLine=54 endLine=60}
```

Then the image is built with `Docker build . -t memcached-dtrace` in this
directory, producing a `memcached` image with dtrace probes.

During the configure process, I can see that it finds the dtrace stub:

```
...
checking for dtrace... /usr/bin/dtrace
...
```

Later on it generates a header `memcached_dtrace.h`, which is conditionally
included when dtrace probes are enabled: [^9]

```bash
/usr/bin/dtrace -h -s memcached_dtrace.d
sed -e 's,void \*,const void \*,g' memcached_dtrace.h | \
            sed -e 's,char \*,const char \*,g' | tr '\t' ' ' > mmc_dtrace.tmp
mv mmc_dtrace.tmp memcached_dtrace.h
```

This generated header defines the macros which are actually called in the
source code of `memcached`, for instance:


```{.c include=src/memcached_dtrace.h startLine=93 endLine=95}
```

So it seems like the dtrace support has been built into `memcached`. Now that
the image has been built, this can be verified against a running process
instance. To start a test instance the docker commands to bind to localhost on
the standard `memcached` port are:

```
docker run --name memcached-dtrace -p 11211:11211 memcached-dtrace
```

Or, alternatively, use an image already built:

```
docker run --name memcached-dtrace -p 11211:11211 quay.io/dalehamel/memcached-dtrace:latest
```

To probe it, we'll need to get the process ID of `memcached`:

```
MEMCACHED_PID=$(docker inspect --format '{{.State.Pid}}' memcached-dtrace)
```

Now I can run `tplist` from bcc, or use bpftrace[^8] to list the USDT
tracepoints:

```
tplist -p ${MEMCACHED_PID}
```

Shows These tracepoints[^16]:
```
/usr/local/bin/memcached memcached:conn__create
/usr/local/bin/memcached memcached:conn__allocate
/usr/local/bin/memcached memcached:conn__destroy
/usr/local/bin/memcached memcached:conn__release
/usr/local/bin/memcached memcached:process__command__end
/usr/local/bin/memcached memcached:command__add
/usr/local/bin/memcached memcached:command__replace
/usr/local/bin/memcached memcached:command__append
/usr/local/bin/memcached memcached:command__prepend
/usr/local/bin/memcached memcached:command__set
/usr/local/bin/memcached memcached:command__cas
/usr/local/bin/memcached memcached:command__touch
/usr/local/bin/memcached memcached:command__get
/usr/local/bin/memcached memcached:process__command__start
/usr/local/bin/memcached memcached:command__delete
/usr/local/bin/memcached memcached:command__incr
/usr/local/bin/memcached memcached:command__decr
/usr/local/bin/memcached memcached:slabs__slabclass__allocate__failed
/usr/local/bin/memcached memcached:slabs__slabclass__allocate
/usr/local/bin/memcached memcached:slabs__allocate__failed
/usr/local/bin/memcached memcached:slabs__allocate
/usr/local/bin/memcached memcached:slabs__free
/usr/local/bin/memcached memcached:item__link
/usr/local/bin/memcached memcached:item__unlink
/usr/local/bin/memcached memcached:item__remove
/usr/local/bin/memcached memcached:item__update
/usr/local/bin/memcached memcached:item__replace
/usr/local/bin/memcached memcached:assoc__find
/usr/local/bin/memcached memcached:assoc__insert
/usr/local/bin/memcached memcached:assoc__delete
/usr/local/bin/memcached memcached:conn__dispatch
```

This showed that probes had been recognized on the binary, and so the had been
compiled in successfully, even though there was no available OS package. This
shows the versatility and ease with which these probes can be applied to
existing application suites.

To test this out, I can send a test 'SET' command to my `memcached` instance.
I chose one based off the benchmarking tool `memtier_benchmark`, which I'll use
later. A simple invocation [@memcached-cheatsheet] based on standard shell
tools is:

```
printf "set memtier-3652115 0 60 4\r\ndata\r\n" | nc localhost 11211
```

Now that I have USDT support, I can rebuild my original uprobe example. Checking
the dtrace file again, I see the signature for the `process__command` probe. It
just so happened that the function my colleague Camilo [@camilo-github]
selected was also a USDT probe point!

```{.c include=src/memcached/memcached_dtrace.d startLine=168 endLine=174}
```

Based on this, I built an initial translation of the uprobe tool to use USDT.


```{.awk include=src/mcsnoop-usdt.bt}
```

But this tool is still very limited, and so I wanted to try and expand it
further to provide more functionality, closer to being on-par with the original
`mctop` tool.

[^8]: there is actually a bug right now where this isn't working, this will be
      fixed in a future bpftrace / bcc release.
[^9]: on a production instance, I had to further modify the dtrace setup in
      order to disable semaphores, see https://github.com/iovisor/bcc/issues/2230
[^16]: These entries correspond to the data read from `readelf --notes`
      elsewhere in this report, as that is where these entries are read from.
