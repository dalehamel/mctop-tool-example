# Flash Sales

In the Commerce business, there is a special class of merchants that run
"flash-sales". This is characterized by a huge number of visitors to a
storefront, followed (hopefully) by a lot of transactions to purchase whatever
the newly-released or on-sale item is. These sorts of issues are especially
notable for the company which employs me, Shopify.

Success in a flash sale, unsurprisingly, depend heavily on being able to
efficiently serve cached data. If a cache isn't performing well, the sale won't
go well. Much of the contention in a flash sale is on the database. There are
several caching strategies in place that protect requests from hammering the
MySQL instance for a Shopify Pod [^10] of shops. By sharing access to a cache
across a pool of web workers, all web workers within a Shopify Pod to benefit.

Despite optimization efforts, in some sales, there can be performance issues.
Following on an investigation of a sale that didn't go well, we decided to
perform some hot-key analysis on a test shop using a load testing tool. During
these load tests, we developed some instrumentation with `bpftrace` to gain
some insight into the cache access pattern under scenarios we saw in the 
original issue.

## War Games

To make sure that we are testing our systems at scale, platform engineering
teams at Shopify set up "Red team / Blue team" exercises, where the "Red" team
tries to devise pathological scenarios using our internal load-testing tools,
used to simulate flash-sale application flows against the platform.

Meanwhile, the other "Blue" monitors the system and mitigates or documents any
issues that may arise.

During one such exercise, my colleague Bassam Mansoob [@bassam] detected
that there were a few instances where a specific Rails Cache ring would be
overloaded under very high request rates. This reflected conditions we had seen
in real production incidents. Problems were first detected with our
higher-level statsd application monitoring:

![](img/request-queueing.png)

We could also see a large spike in the rate of GET/SET operations in this span:

![](img/set-rate.png)

![](img/get-rate.png)

To pinpoint the problem, we looked to eBPF tools for detecting the hot keys on
the production memcached instance we were exercising in our Red/Blue exercise.

### Hot key detection with bpftrace

We used `bpftrace` to probe the memcached process that would be hit by our
load-testing tool. For one cache we found one extremely hot key using our first
uprobe-based prototype[^3]:

```
@command[gets podYYYrails:NN::feature_rollout:percentages]: 6579978
@command[delete podYYY:rails:NN::jobs-KEY ...]: 2854
@command[delete podYYY:rails:NN::jobs-KEY ...]: 3572
@command[gets podYYY:rails:NN::shop-KEY ...]: 5638
@command[set podYYY:rails:NN::KEY 1 30 13961]: 9266
```

And in our identity cache, used for checking if feature flags for new code are
enabled, we found keys that were being hit very frequently:

```
@command[gets podXXX::M:blob:Feature::FEATURE_KEY:SHOP_KEY_1]: 67772
@command[gets podXXX::M:blob:Feature::FEATURE_KEY:SHOP_KEY_N]: 67777
@command[gets podXXX::M:blob:Feature::FEATURE_KEY:SHOP_KEY_M]: 6779
```

Having gained a quick view into what keys ere especially hot, we could direct
our mitigation efforts towards investigating the code-paths that were
interacting with them.

## Hot key mitigation

Since these keys do not change very frequently, we decided to introduce an
in-memory cache at the application layer inside of rails itself. With a TTL of
a full minute, it would hit memcached much less frequently.

The change was simple, but results were remarkable. Without the in-memory cache
we see large spikes on both memcached, and mcrouter (the proxy we use to access
it):

## Performance Results

During these hot-spotting events during real or simulated flash sales, we could
see the application impact without the cache:

![](img/without-cache.png)

And with the in-memory cache, we saw a substantial reduction in latency:

![](img/with-cache.png)

As for throughput, without the extra caching layer:

![](img/without-cache-throughput.png)

And the improvements with the in-memory cache added:

![](img/with-cache-throughput.png)

So quick-and simple `bpftrace` one-liner was able to get us pretty far towards
resolving this problem!

Following this incident, the idea of making it easier to perform this type of 
investigation with a bespoke tool came about. One of my colleagues[^4] pointed
me towards `mctop`, and suggested I try to re-implement it in eBPF. This is
what the remainder of this report will focus on.

[^3]: covered later on
[^4]: Jason Hiltz-Laforge and Scott Francis, put the idea in my head. Jason had
      suggested it to Scott, attempting to "nerd-snipe"[@xkcd-356] him, but Scott
      successfully deflected that onto me.
[^10]:  a "Shopify Pod" is a distinct concept from a Kubernetes Pod, and it is
      an unfortunate and confusing naming collision. A Shopify Pod is a 
      contained set of resources, built around the concept of MySQL sharding.
