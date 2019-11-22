# Final bcc tool

Now that all of the data reading was fixed up, and there was no more need to
degarble the keys in userspace, the final version of this tool could be
put together.

## Building the UI

This probably took most of the time. The other `*top.py` tools I saw didn't
really offer the interactive eperience that the orignal `mctop` in ruby did.

Most of the time here was spent re-acquinting myself with terminal concepts,
and and getting the `select` statement set up properyl for receiving user
input. I based most of this on the original `mctop` in ruby.

I decided to add a couple of fields, as I was capturing more data than the
orignal, and I changed how tray bar of the tool works entirely.

I analyzed the original screenshot and browsed through the
source code of mctop to see how it was building its UI and accepting input.

I was able to construct the corresponding header line that I would use later
in trying to mimic it in bpftrace. 

From this line in the original ruby mcop for instance:

```{.ruby include=src/mctop/lib/ui.rb startLine=128 endLine=134}
```

### Key entry

The usage of select wase based on the original ruby:

```{.ruby include=src/mctop/lib/ui.rb startLine=152 endLine=169}
```

In Python, without pulling in dependencies using termios along with select
helps to recreate the experience of using the original mctop:

```{.python include:src/bcc/tools/mctop.py  startLine=166 endLine=171}
```

And just ass ruby had a switch on the different inputs:

```{.ruby include=src/mctop/bin/mctop startLine=36 endLine=62}
```

So to was this almost directly ported to Python:

```{.python include:src/bcc/tools/mctop.py  startLine=172 endLine=194}
```
### Sorting

To sort the data, a lambda was defined for each sort mode:

```{.python include:src/bcc/tools/mctop.py  startLine=144 endLine=163}
```

### Dumping data

Since it would probably be usefull to be able to analyze the collected data,
the python mapping of the original eBPF map can be saved to a JSON file for
analysis when the map is cleared. This also allows for `mctop` to act as a sort
of `mckeydump` tool, saving the data that was traced in a session:

```{.python include:src/bcc/tools/mctop.py  startLine=196 endLine=206}
```

## Finishing touches

Since the goal of the tool is to share it, especially so fans of the original
`mctop` or `memkeys` command could have access a light-weight eBPF option, it
is definitely a goal to share this tool and get it into good enough shape for 
it to pass a pull request review[@bcc-contributing-tools].
