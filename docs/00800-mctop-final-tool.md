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

// FIXME show original mctop ruby select usage


The usage of select wase based on the original ruby:

```{.ruby include=src/mctop/lib/ui.rb startLine=152 endLine=169}
```

```{.ruby include=src/mctop/bin/mctop startLine=36 endLine=62}
```



### Sorting

// Show the sorting code

### Dumping data

// Dumping the data code, and use-case

## Finishing touches

Since my goal is to share this tool, especially so fans of the original `mctop`
or `memkeys` command could have access to it, I wanted it to be in good enough
shape to pass a pull request review. There are strict guidelines for the bcc
repo, so I set to work on them. [@bcc-contributing-tools]

This entailed writing the manpage and sample usage.
