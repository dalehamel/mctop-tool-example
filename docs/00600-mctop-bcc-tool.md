# Getting started on a bcc tool

To make a fully-featured port of `mctop`, `bpftrace` wouldn't quite fit the
bill, as it doesn't have quite the same flexibility as python when it comes to
post-processing data.

## USDT example from reference guide overview

Explain bit by bit what it is doing

FIXME link to reference guide in bibliography

```{.c include=src/bcc/docs/reference_guide.md startLine=235 endLine=242}
```

## Examining some real tools

"When in Rome, do as the Romans do"

To get an idea of how USDT probes were used in real-world scripts, I decided to
check out a couple of the existing bcc tools, to gain inspiration and a better
understanding of how to port my bpftrace script to bcc.

### mysqld_qslower.py

The first example I looked at was one for instrumenting MySQL. This goes to
show just how much of a swiss-army-knife USDT can be - the same tools can be
used to debug `memcached` and MySQL!

The C code segment of this script showed a real invocation of the methods to
read USDT argument data, and how to set up a map to store structured data:

```{.c include=src/bcc/tools/mysqld_qslower.py startLine=44 endLine=68}
```
### ucalls.py

Another great example that I spent a lot of time dissecting was ucalls.py,
which is the script that powers `rubycalls` and other language-specific USDT
tools in bcc. It does a little bit of meta-programming, in that it swaps out
function calls and arguments to match that of the runtime for the target
language. This allows for it to share the same control logic, regardless of
which language is being traced. For instance, for Ruby it sets the probe
points here:

```{.python include=src/bcc/tools/lib/ucalls.py startLine=91 endLine=96}
```

Then later, in the C code, it uses these to replace `READ_CLASS` and
`READ_METHOD` when it is building out the probe function:


```{.c include=src/bcc/tools/lib/ucalls.py startLine=151 endLine=163}
```

There are several other tools in this suite, targeting Ruby, Python, Java, PhP,
 tcl, and Perl. Some tools are specific to a given language, as support does
vary somewhat depending on what probes the runtime maintainers choose to
expose.

These scripts provided a wealth of examples for how USDT tracing was already
being done in `bcc`, and a starting point.

### slabratetop.py

UI / UX design isn't my forte, and apparently imitation is the sincerest form
of flattery. To start with, I looked through the `*top.py` `top`-like tools for
one to base the structure of my program on. A fine example was
`slabratetop.py`, which happens to be the Python version of the `bpftrace`
script that I showed earlier. The design of its main control loop and argument
parsing were the main concepts borrowed:

```{.python include=src/bcc/tools/slabratetop.py startLine=112 endLine=143}
```

This was then blended with the `select` approach used by the Ruby `mctop` in
order to receive keyboard input, which I'll be covering in more detail later.
