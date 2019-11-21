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
