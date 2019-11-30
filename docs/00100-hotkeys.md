# hot keys and mctop

The topic of hot keys in `memcached` is well-researched, and tools have existed
to support this ecosystem long since before eBPF was on the scene.

An investigation into a cache hot-spotting lead to a eBPF-based redevelopment of
the original original `libpcap`-based `mctop` tool.

This report is verbose, and attempts to assume no advanced knowledge of `eBPF`,
the `ELF` format, or `memcached` itself. An extensive bibliography covers more
works that can hopefully fill what gaps this report leaves.

## mctop

The `mctop` tool was originally developed by etsy [@etsy-mctop], and the author
 wrote an informative blog post [@mctop-blog-post] on the topic that originally
the motivations that dove them to develop the tool. This concept was developed
further by tumbler in a similar tool, `memkeys` [@tumblr-memkeys].

These tools both produced a `top`-like interface focussing on `memcached` key
access, with basic abilities to sort the data by column. Awareness of hot keys
can inform application decisions of how best to to utilize caching patterns
under heavy load.

This is a screen capture of the redeveloped `mctop` built on eBPF and USDT
tracing:

![](./img/mctop.gif)
