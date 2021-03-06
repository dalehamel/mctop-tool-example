# Getting started on a bcc tool

To make a fully-featured port of `mctop`, `bpftrace` wouldn't quite fit the
bill, as it doesn't have quite the same flexibility as Python when it comes to
post-processing data.

## USDT example

From the `bcc` reference guide [@bcc-usdt-reference], an example program
snippet is provided showing how to read data from a USDT argument:

```{.c include=src/bcc/docs/reference_guide.md startLine=235 endLine=242}
```

It just declares a 64 bit integer to store an address, and 128-byte character
array to store a path, presumably a string.

`bpf_usdt_readarg` is used to read the argument, and is called to store the
literal value of an integer for `addr`, and this value happens to be a pointer
to where the string for `path` is stored in the memory space. This is handled
by the next call.

`bpf_probe_read` reads a fixed number of bytes, starting from the pointer
address.

With these basics, the tool could be translated to C (for the probes) and
Python (for the UI and post-processing / deriving second-order values).

## Examining some real tools

"When in Rome, do as the Romans do"

To get an idea of how USDT probes were used in real-world scripts, existing
bcc tools are a good source inspiration and to gain better understanding of
how to port the `bpftrace` script to `bcc.

### mysqld_qslower.py

The first example I looked at was one for instrumenting MySQL. This goes to
show just how much of a swiss-army-knife USDT can be - the same tools can be
used to debug Memcached and MySQL!

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
points at one location:

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
being done in `bcc`, and a jumping off point for a new tool.

### slabratetop.py

UI / UX design isn't my forte, and apparently imitation is the sincerest form
of flattery. To start with, I looked through the `\*top.py` `top`-like tools for
one to base the structure of my program on. A fine example was
`slabratetop.py`, which happens to be the Python version of the `bpftrace`
script that was showed earlier. The design of its main control loop and
argument parsing were the main concepts borrowed:

```{.python include=src/bcc/tools/slabratetop.py startLine=112 endLine=143}
```

This was then blended with the `select` approach used by the Ruby `mctop` in
order to receive keyboard input, which will be covered in more detail in the
UI section of this document.
