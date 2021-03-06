# Testing mctop tool

Initial basic testing of the `mctop` and `mcsnoop.bt` tools were made easier
by `printf` to write commands to test tracing. At these lower call frequencies
though, errors such as were encountered are not immediately obvious. It wasn't
until `memtier_benchmark` was first used to generate load was it completely
clear what the cause of the garbled key reads were.

Now that `mctop` has been cleaned up, and keys are stored properly this tool
can be used to demonstrate how `mctop` works, and show that it can keep up with
tracing requests to Memcached.

## memtier benchmark

The `memtier_benchmark` tool can be used to generate load to the test
Memcached instance that I built earlier, with dtrace probes enabled.

Rather than having to print to `nc`, this allows for rapidly firing off a large
number of commands, showing that the tool is behaving as expected. This also
gives a lot more data, for more interesting exploration of the tool, allowing
for sorting on real data, and testing out dumping real data to a JSON file.

A simple invocation of the tool:

```
memtier_benchmark --server localhost --port 11211 -P memcache_text  --key-pattern=G:G
```

// FIXME dive into other options, show output in mctop
