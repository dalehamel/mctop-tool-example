# Flash Sales

My employer is in the Commerce business, and caters specifically to
"flash-selling" merchants. This is characterized by a huge number of visitors
to their storefront, followed (hopefully) by a lot of transactions to purchase
whatever the newly-released or on-sale item is.

These sorts of scenarios, unsurprisingly, depend heavily on being able to
efficiently serve cache data. If a cache isn't performing well, the sale won't
go well, and a lot of people will be very upset. That's not cool. It is much
cooler if the sale goes well, and the cache performs its job and protects the
downstream app, then we should have no trouble handling the throughput.

In some sales though, there can be performance issues. Following on an
investigation of a sale that didn't go well, we decided to perform some hot-key
analysis on a test shop using a load testing tool. During these load tests, we
developed some instrumentation with `bpftrace` to gain some insight into the
cache access pattern under scenarios we saw in the original issue.

Much of the contention in a flash sale is on the database. We have several
caching strategies in place that protect requests from hammering the MySQL
instance for a Shopify Pod of shops. To share access to a cache across a pool
of workers also allows for all workers within a Shopify Pod to benefit.

## War Games

To make sure that we are testing our systems at scale, platform engineering
teams at Shopify set up a "Red team / Blue team" exercise, where the "Red" team
tries to devise pathological scenarios using our internal load-testing tool
used to simulate flash-sale application flows against the platform.

Meanwhile, the other "Blue" monitors the system and mitigates or documents any
issues that may arise.

During one one such exercise, my colleague Bassam Mansoob [@bassam] detected
that there were a few instances where a specific Rails Cache ring would be
overloaded, under very high RPM, which reflected conditions we had seen in in
real production incidents.

Problems were first detected with our higher-level statsd application
monitoring:

![](img/request-queueing.png)

We could also see a large spike in the rate of operations:

![](img/set-rate.png)

![](img/get-rate.png)

To pinpoint the problem, we looked to eBPF for detecting the hot keys on the
production memcached instance we were exercising in our Red/Blue exercise.

### Hot key detection with bpftrace

bpftrace to probe the memcached containers/processes in question.

For one cache we found one extremely hot key using our first uprobe-based
prototype[^3]:

```
@command[gets podYYYrails:NN::feature_rollout:percentages]: 6579978
@command[delete podYYY:rails:NN::jobs-KEY ...]: 2854
@command[delete podYYY:rails:NN::jobs-KEY ...]: 3572
@command[gets podYYY:rails:NN::shop-KEY ...]: 5638
@command[set podYYY:rails:NN::KEY 1 30 13961]: 9266
```

And in our identity cache used for checking if feature flags for new code are
enabled:

```
@command[gets podXXX::M:blob:Feature::FEATURE_KEY:SHOP_KEY_1]: 67772
@command[gets podXXX::M:blob:Feature::FEATURE_KEY:SHOP_KEY_N]: 67777
@command[gets podXXX::M:blob:Feature::FEATURE_KEY:SHOP_KEY_M]: 6779
```

Having gained a quick view into what were especially hot, we could direct our
attention to investigating the codepaths that were interacting with them.

## Hot key mitigation

Since these keys do not change very frequently, we decided to introduce an
in-memory cache at the application layer inside of rails itself. With a TTL of
a fully minute, it will hit memcached much less frequently.

The change was simple, but results were pronounced. Without the in-memory cache
we see large spikes on both memcached, and mcrouter (the proxy we use to access it):

## Performance Results

During these hotspotting events, we could see the effect without the cache:

![](img/without-cache.png)

And with the in-memory cache, we saw a substantial reduction in latency:

![](img/with-cache.png)

As for throughput, without the extra caching layer:

![](img/without-cache-throughput.png)

And the improvements with the in-memory cache added:

![](img/with-cache-throughput.png)

So a quick little `bpftrace` one liner was able to get us pretty far towards
resolving this problem!

From this incident, the idea of making it easier to perform this type of 
investigation with a bespoke tool came about. One of my colleagues[^4] pointed me
towards `mctop`, and suggested I try to re-implement it in eBPF.

[^3]: covered later on
[^4]: Jason Hiltz-Laforge and Scott Francis, put the idea in my head. Jason had
suggested it to Scott, attempting to "nerd-snipe"[@xkcd-356] him, but Scott
successfully deflected that onto me.
