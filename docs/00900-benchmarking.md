# Testing mctop tool

To show that `mctop` can handle heavy load, I decided to try hooking it up to
the same load testing tool we use to stress test our memcached and redis
configurations.

If you want to try it out for yourself, you can pull down the memcached test
image used here with:

// FIXME

Or build it yourself from the Dockerfile. // FIXME bib ref to dockerfile in this repo

## memtier benchmark

The `memtier_benchmark` tool can be used to generate load to the test memcached
instance that I built earlier, with dtrace probes enabled.

Rather than having to print to netcat, this allows for rappidly firing off a
large number of commands, showing that the tool is behaving as expected. This
also gives a lot more data, for more interesting exploration of the tool,
allowing for sorting on real data, and testing out dumping real data to a JSON
file.

A simple invocation of the tool:

```
memtier_benchmark --server localhost --port 11211 -P memcache_text  --key-pattern=G:G
```

// FIXME dive into other options, show output in mctop
