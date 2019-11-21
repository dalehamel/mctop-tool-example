# bpftrace script for mcsnoop

With a basic test lab now set up, I could now look into how I might get cleaner
data out of the other dtrace probes.

Looking again at the dtrace file, I saw that all of the commands had dtrace
probes defined

```{.c include=src/memcached/memcached_dtrace.d startLine=205 endLine=214}
```

`command__set` probe negates the need to parse this, and all of the other
commands have dtrace probes.

In the memcached source, I could see where these probse were being
invoked. Earlier when the dtrace header was defined, the macros that in the
source here were used to add the dtrace probepoints to the source.

```{.c include=src/memcached/memcached.c startLine=1358 endLine=1386}
```

So, based on this set out to try and replicate the (once) popular mctop tool's 
functionality. I analyzed the original screenshot and browsed through the
source code of mctop to see how it was building its UI and accepting input.

From this line in the original ruby mcop for instance:

```{.ruby include=src/mctop/lib/ui.rb startLine=128 endLine=134}
```

I was able to construct the corresponding header line that I would use later
in trying to mimic it in bpftrace.

I checked out some of Brendan Gregg's examples from his now book's repo, and
came across #FIXME top, which I based my script on.

I decided to just probe the `set` command to start with.

```{.awk include=src/mcsnoop-orig.bt}
```

Unfortunately at this point I hit the limit of what bpftrace could do for me.
It is great for analyzing map data, but not so great at producing interactive
top-like UIs yet, as that involves some sophisticated post-processing of the
map data. For this reason, a complete `mctop` tool is currently out of reach of
bpftrace, so I set my eyes on a bcc tool to do the same.

Ultimately, the most complete working version of this bpftrace prototype is
something I'd rather call `mcsnoop`, as it snoops memcached key access using
usdt probes.

This is the current most working version, and solves the problem I would later
have in my BCC version of treating `void *` byte buffers properly, which I'll
be covering in more detail later. The TL;DR of this is that memcached (and
probably lots of other OG dtrace stuff) provides access via a 2-tuple o args,
the first being the start of the buffer address, and the second being the
length of the string data in the buffer. For this reason, the following script
uses bpftrace functionality of `str(buffer, len)` signature to properly read
keys from memcached.

```{.awk include=src/mcsnoop-working.bt}
```
