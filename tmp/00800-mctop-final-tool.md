# Final bcc tool
<audio controls="1"> <source src="audio/mp3/00800-mctop-final-tool.md.plain.mp3" type="audio/mpeg"></source> </audio>

Now that all of the data reading was fixed up, and there was no more need to
de-garble the keys in userspace, the final version of this tool could be
put together.

## DISCLAIMER

This tool has been designed primarily against benchmark workloads, but has not
seen extensive production testing outside of basic testing. In order to run
`mctop`, Linux Kernel v4.20 or later is needed, but 5.3 or later is
recommended.

## UI Re-Design

This probably took most of the time. The other `*top.py` tools I saw didn't
really offer the interactive experience that the original `mctop` in Ruby did.

Most of the time here was spent reacquainting myself with TTY concepts, and
getting the `select` statement set up properly for receiving user input. I
based the scaffold of this on the original `mctop` in Ruby, and copied its
design patterns.

I decided to add a couple of fields, as I was capturing more data than the
original, and I changed how tray bar of the tool works entirely. Beyond just
sorting keys by various attributes, specific keys could be analyzed.

## Feature Implementation

### Key entry

The usage of select was based on the original Ruby:

```{.ruby include=src/mctop/lib/ui.rb startLine=152 endLine=169}
```

In Python, without pulling in dependencies, the `termios` library  along with
`select` can be used to recreate the experience of using the original `mctop`:

```{.python include=src/bcc/tools/mctop.py  startLine=166 endLine=171}
```

And just as Ruby had a switch on the different inputs:

```{.ruby include=src/mctop/bin/mctop startLine=36 endLine=62}
```

So too was this almost directly ported to Python:

```{.python include=src/bcc/tools/mctop.py  startLine=172 endLine=194}
```

The concept is just a giant if-ladder, as Python has no case statements. This
matches on the letters, and can run a function or update a global variable as
the specific case requires. This got complicated as I added more keys to allow
for navigation of the sorted key data.

### Sorting

To sort the data, a lambda was defined for each sort mode:

```{.python include=src/bcc/tools/mctop.py  startLine=144 endLine=163}
```

This is called on the map for each period of the refresh interval, so the
ordering of keys displayed may change each second, should the rank of a key
differ from the previous interval.

### Dumping data

Since it would probably be useful to be able to analyze the collected data,
the Python mapping of the original eBPF map can be saved to a JSON file for
analysis when the map is cleared. This also allows for `mctop` to act as a sort
of `memcached-dump` tool (à la `tcpdump`), saving the data for archival purposes
or offline analysis.

```{.python include=src/bcc/tools/mctop.py  startLine=196 endLine=206}
```

This should allow for a simple pipeline of Memcached metrics into other
centralized logging systems.

## View Modes

The current/traditional UI for `mctop` was limited in that it couldn't drill
down into patterns, and there was no way to navigate the data that was being
selected aside from to sort it.

### Streaming / NoClear

This design is important to maintain, as it allows for metrics to be collected
from line-based logging systems that understand how to parse `mctop` output.

In this mode, `mctop` behaviors similar to `mcsnoop`.

### Interactive

This is built around a TTY-interactive experience, using ANSI escape
sequences to construct a basic UI. The UI uses vim-like bindings, and is meant
for keyboard navigation that should feel natural to any vim user.

Outside of being interactive, `mctop` maintains the original sort-functionality
of its namesake.

`mctop` has different visual modes, that correspond to different probes to
collect data for a specific key and analyse it.

#### Navigation

To navigate, the `j` and `k` keys can be used to move up or down a keys, and
the selected key is displayed in the footer bar.

The footer bar also now shows the number of pages, segmented by the `maxrows`
argument. To easily navigate this buffer, `u` and `d` can be used to navigate
up and down a page in this buffer.

Finally, to jump to the end of the buffer, `G`, and to the start of the sorted
 key list, `g`. 

As this control sequence is extremely common in command line tools, the hope is
that the navigation keys will feel natural to users of similar tools.

#### Command latency

To be able to add a new data source and expand on the functionality of the
`mctop` predecessor, the latency commands hitting each key could be measured
and displayed in aggregate.

This additional data could also be used to plug into `bcc`'s histogram map type
and print function, showing an informative `lg2` representation of the latency
for commands hitting the key.

#### Printing Histogram

Printing a histogram of latency data entails recompiling the `eBPF` source to
have the static key to collect latency data embedded in the eBPF source.

An inline `match_key` function is used to iterate through the buffer to compare
until it finds the key in full or finds a mismatching character and returns
early. This bounded loop is permitted in eBPF, but may be wasteful processing
at large key sizes.

When a trace on a Memcached command is executed, it stores the `lastkey` in a
map [^12]. In another probe on `process__command__end`, this is accessed and
compared with the hard-coded and selected key from the UI. When there is a
match, the computed latency data is added to the histogram.

Upon entering histogram mode, the selected data will be immediately
displayed on the same refresh interval. This shows the real-time
variations in Memcached latency, in buckets of doubling size.

Switching to histogram mode will detach and replace running probes, and discard
the collected data, replacing the eBPF probes with a function that is targeted
to a specific cache key.[^13]

#### Inspect Key

## Finishing touches and final tool

Since the goal of the tool is to share it, especially so that fans of the
original `mctop` or `memkeys` command could have access a light-weight eBPF
option, it is definitely a goal to share this tool and get it into good enough
shape for it to pass a pull request review [@bcc-contributing-tools].

For this reason, this report was prepared to supplement the material around the
`mctop` tool included in the pull request.

This script is submitted in its entirety:

```{.python include=src/bcc/tools/mctop.py}
```

[^12]: this is indexed by connection ID right now, but I think that thread id
       or perhaps a composition of connection and thread id should be used, to
       ensure that this representation is compatible with memcached's threading
       model.
[^13]: this is due to the need to get a lock on the uprobe addresses, and it
       seems there is no way to hot-patch eBPF programs to encode the selected
       key.
