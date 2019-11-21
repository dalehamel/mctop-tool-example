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

## Feature flags key optimization

It was quite clear from other instrumentation that certain rings were hot
spotting, so we wanted to identify which keys specifically. For this, we used
bpftrace to probe the memcached containers/processes in question.

For one cache we found:

```
@command[gets podYYYrails:NN::feature_rollout:percentages]: 6579978
@command[delete podYYY:rails:NN::jobs-slo-empty-queue-timestamp-pod140-r]: 2854
@command[delete podYYY:rails:NN::jobs-empty-queue-timestamp-pod140-realt]: 3572
@command[gets podYYY:rails:NN::shop:20930625677:ThemeFiles:20930625677:/]: 5638
@command[set podYYY:rails:NN::feature_rollout:percentages 1 30 13961]: 9266
```

feature_rollout:percentages size is about 13.9KB

And in another cache:

```
@command[gets podXXX:IDC:M:blob:Feature:3320508158388131129:14282421]: 67772
@command[gets podXXX:IDC:M:blob:Feature:3320508158388131129:152943427708]: 67777
@command[gets podXXX:IDC:M:blob:Feature:3320508158388131129:152943460476]: 6779
```
Since these keys do not change much, we decided to introduce in-memory caching
inside of rails itself, so that we can cache these keys for much longer,
raising a their TTL to minute.

The results were pronounced. Without the cache we see large spikes on both
memcached, and mcrouter (the proxy we use to access it):

![](img/without-cache.png)

And with the cache, we saw a substantial reduction:

![](img/with-cache.png)

So a quick little `bpftrace` one liner was able to get us pretty far! But what
if we could make a bespoke tool to help us identify hot keys and other aspects
of cache-key access at a glance?

