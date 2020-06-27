# Memcached Static Tracepoints
<audio controls="1"> <source src="audio/mp3/00400-memcached-dtrace.md.plain.mp3" type="audio/mpeg"></source> </audio>

If an application supports USDT tracepoints already, then no
modification of the source code is necessary. Fortunately, Memcached
already includes Dtrace probes and strategic spots within the codebase.

The presence of this established pattern for comprehensive Dtrace
tracepoints significantly simplified building a lightweight and simple
tool. This section of the Memcached source, shows how these probes are
invoked:

```{.c include=src/memcached/memcached.c startLine=1358 endLine=1386}
```

Unfortunately, when a production Memcached instance was checked with `tplist`
or `bpftrace -l 'usdt:* -p $(pidof memcached)`, no probes were shown. This
meant there would be a need to modify the Memcached image to add Dtrace probes.

The `Dockerfile` [@dockerfile] that was used is based on a production
configuration which has been simplified for this analysis. The relevant
addition to add Dtrace probes was this snippet:

```{.bash include=src/docker/Dockerfile startLine=9 endLine=14}
```
Though the package is being pulled from Ubuntu, only a few text files
are needed from it. This package just installs the `sys/sdt.h` header, and
a stub command of Dtrace that can be used to convert a dtrace file into
a generated header, providing the necessary macros to add tracepoints.
The Debian archive is extracted, and the `/usr/bin/dtrace` shell stub and
headers are copied into the docker image at standard paths.

Then on the configure line for Memcached, just adding `--enable-dtrace` was
sufficient:

```{.bash include=src/docker/Dockerfile startLine=54 endLine=61}
```

The image can be built with `Docker build . -t memcached-dtrace` in this
directory, producing a Memcached image with dtrace probes.

During the configure process, this output indicates it finds the Dtrace stub:

```
...
checking for dtrace... /usr/bin/dtrace
...
```

Later on it generates a header `memcached_dtrace.h`, which is conditionally
included when Dtrace probes are enabled: [^9]

```{.bash include=src/dtrace-generate.txt}
```

This generated header defines the macros which are already called in the
source code of Memcached:


```{.c include=src/memcached_dtrace.h startLine=93 endLine=95}
```

So it seems like the dtrace support has been built into Memcached. Now that
the image has been built, this can be verified against a running process
instance. To start a test instance the docker commands to bind to localhost on
the standard Memcached port are:

```
docker run --name memcached-dtrace -p 11211:11211 memcached-dtrace
```

Or, alternatively, use an image already built:

```
docker run --name memcached-dtrace -p 11211:11211 quay.io/dalehamel/memcached-dtrace:latest
```

To probe it, we'll need to get the process ID of Memcached:

```
MEMCACHED_PID=$(docker inspect --format '{{.State.Pid}}' memcached-dtrace)
```

Now I can run `tplist` from bcc, or use bpftrace[^8] to list the USDT
tracepoints:

```
tplist -p ${MEMCACHED_PID}
```

Shows These tracepoints[^16]:

```{.bash include=src/tracepoints.txt}
```

This showed that probes had been recognized on the ELF binary, and so had
been compiled-in successfully, even though there was no available OS
package. This shows the ease with which these probes can be applied to
existing application suites.

With USDT support now confirmed, a probe can be built that targets the
`process__command` probe, reading arguments based on the probe signature.

```{.c include=src/memcached/memcached_dtrace.d startLine=168 endLine=174}
```

The uprobe tool from earlier can be rewritten to target this static tracepoint:

```{.awk include=src/mcsnoop-usdt.bt}
```

This serves as a minimal proof of concept that the same tool can be built with
a USDT probe, but is nowhere near parity for the data that the original `mctop`
tool could provide.

[^8]: there is a bug right now where this isn't working for
      containerized processes, this will be fixed in a future bpftrace /
      bcc release. // FIXME file bug
[^9]: on a production instance, I had to further modify the dtrace setup in
      order to disable semaphores, see https://github.com/iovisor/bcc/issues/2230
[^16]: These entries correspond to the data read from `readelf --notes`
      elsewhere in this report, as that is where these entries are read from.
