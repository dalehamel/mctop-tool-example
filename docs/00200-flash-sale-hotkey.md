# Flash Sales

In the commerce-hosting business, there is a special class of merchants that
run "flash-sales". This is characterized by a huge number of visitors to a web
storefront, followed (hopefully) by a lot of transactions to purchase whatever
the newly-released or on-sale item is. These sorts of issues are especially
notable for the employer of the author of this report, Shopify.

Success in a flash sale, unsurprisingly, depends heavily on being able to
efficiently serve cached data. If a cache isn't performing well, the sale won't
go well. Much of the contention in a flash sale is on the database. There are
several caching strategies in place that protect requests from hammering the
MySQL database instance for a Shopify Pod [^10] of shops. By sharing access to
a cache across a pool of web workers, all web workers within a Shopify Pod
benefit from this large pool of surge capacity.

Despite optimization efforts, in some sales, there can be performance issues.
Following on an investigation of a sale that didn't go so well, we decided to
perform some hot-key analysis on a (not real) test shop using a load testing
tool. During these load tests, we developed some instrumentation with
`bpftrace` to gain insight into the cache access patterns.

## War Games

To make sure that we are testing our systems at scale, platform engineering
teams at Shopify set up "Red team / Blue team" exercises, where the "Red team"
tries to devise pathological scenarios using our internal load-testing tools,
used to simulate flash-sale application flows against the platform.

Meanwhile, the other "Blue team" monitors the system to investigate and
mitigate any issues that may arise.

During one such exercise, my colleague Bassam Mansoob [@bassam] detected
that there were a few instances where a specific Rails cache-ring would be
overloaded under very high request rates. This reflected conditions we had seen
in real production incidents. Problems were first detected with our
higher-level statsd application monitoring:

![](img/request-queueing.png)

We could also see a large spike in the rate of GET/SET operations in this span:

![](img/set-rate.png)

![](img/get-rate.png)

To pinpoint the problem, we looked to eBPF tools for detecting the hot keys on
the production Memcached instance we were examining in our Red/Blue exercise.

### Hot key detection with bpftrace

We used `bpftrace` to probe the Memcached process that was targeted by our
load-testing tool. For one cache we quickly found one extremely hot key using
our first uprobe-based prototype[^3]:


```{.awk include=src/bpftrace-rails-keys.txt}
```

It seemed like the cache entry used to determine the ratio of for a particular
feature that should be enabled was a very hot key, as the same command was
being hit at dramatically higher rates than other keys.

In our identity cache, used here for checking if feature flags for new code are
enabled, we found keys that were being hit very frequently:

```{.awk include=src/bpftrace-feature-keys.txt}
```

Having gained a quick view into what keys were especially hot, we could
direct our mitigation efforts towards investigating the code-paths that
were interacting with these keys.

## Hot key mitigation

Since these keys do not change very frequently, we decided to introduce
an in-memory cache at the application layer inside of Rails itself. With
a TTL of a full minute, it would hit Memcached much less frequently.

The change was simple, but the results were remarkable. Without the
in-memory cache, there were large spikes on both Memcached, and the Mcrouter
proxy.

## Performance Results

During these hot-spotting events from real or simulated flash sales,
the impact without the cache is easy to spot:

![](img/without-cache.png)

And with the in-memory cache, there was a substantial reduction in latency:

![](img/with-cache.png)

As for throughput, without the extra caching layer throughput to Memcached
spiked:

![](img/without-cache-throughput.png)

And with the improvements from the in-memory cache, throughput was much lower
as the new cache was not busted very frequently:

![](img/with-cache-throughput.png)

So a quick-and simple `bpftrace` one-liner was able to get pretty far towards
resolving this problem!

Following this incident, the idea of making it easier to perform this type of 
investigation with a bespoke tool came about[^4], and it was suggested to try
and re-implement `mctop` in eBPF. This is what the remainder of this report
will focus on.

[^4]: Jason Hiltz-Laforge and Scott Francis, put the idea in my head. Jason had
      suggested it to Scott, attempting to "nerd-snipe"[@xkcd-356] him, but Scott
      successfully deflected that onto me.
[^10]:  a "Shopify Pod" is a distinct concept from a Kubernetes Pod, and it is
      an unfortunate and confusing naming collision. A Shopify Pod is a 
      contained set of resources, built around the concept of MySQL sharding.
