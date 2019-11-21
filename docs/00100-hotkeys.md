# hot keys and mctop

The topic of hot keys in memcached is well researched, and tools have existed
to support this ecosystem long since before eBPF was on the scene.

The mctop tool was originally developed by etsy [@etsy-mctop], who wrote an
informative blog post [mctop-blog-post ]on the topic that originally drove them
to develop the tool. This concept was developed further by tumbler in a similar
tool, called "memkeys" [@tumblr-memkeys].

These tools both produced a top-like interface focussing on memcached key
access, with primitive abilities to sort each column. Detecting hot keys can
inform application decisions on how to utilize caching patterns under load.


## mctop

To fast forward to the end, the tool I produced was a port of the original
`mctop` tool:

![](./img/mctop.gif)
