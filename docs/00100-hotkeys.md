# hot keys and mctop

The topic of hot keys in Memcached has been well-studied, and tools have
existed to support this ecosystem since long before eBPF was on the scene.

An investigation into a cache hot-spotting problem lead to a eBPF-based
redevelopment of the original `libpcap`-based `mctop` tool.

This report is verbose, and attempts to assume no advanced knowledge of eBPF,
the `ELF` format, or Memcached itself. The referenced works can hopefully fill
what gaps this report leaves.

## mctop

The `mctop` tool was originally developed by etsy [@etsy-mctop], and the author
 wrote an informative blog post [@mctop-blog-post] on the topic that motivated
the development of the original tool. This concept was developed further by
Tumblr in a similar tool, `memkeys` [@tumblr-memkeys].

These tools both produced a top-like interface focussing on Memcached
key access, with basic abilities to sort the data by column. Awareness
of hot keys can inform application decisions of how best to utilize
caching patterns under heavy load.

This is a screen capture of the redeveloped `mctop` tool built with eBPF and
USDT tracing:

![](./img/mctop.gif)

Where other tools in this area use `libpcap`, the theory is [^19] that using
eBPF should offer performance advantages, as neither full or partial packet
captures are necessary. Beyond this, the eBPF approach also has the advantage
of inherently working both with the text-based and binary-based protocols, as
no protocol interpretation is required.

[^19]: While this makes sense rationally, until it has been proven
       through a rigorous and scientific series of tests, measuring the
       overhead of both approaches under various conditions, it may not be
       the case. One possible drawback of the eBPF based approach is it
       causes some overhead as probes fire software-interrupts when
       triggered, which may not be the case with tcpdump, even if it is
       doing more processing.

