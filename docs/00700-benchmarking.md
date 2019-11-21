# Test workloads

## docker

```
docker run -p 11211:11211/tcp  6081e0ff51df
```

## print to socket

```
printf "set memtier-3652115 0 60 4\r\ndata\r\n" | nc localhost 11211
```

## cat file to socket

Generate a test file file by scripting changes to this template:

```
printf "set memtier-7520341 0 60 7\r\ndataddx\r\n" >> testfile
```

Then cat it to the socket instead:

```
cat testfile | nc localhost 11211
```

## memtier benchmark

```
memtier_benchmark --server localhost --port 11211 -P memcache_text  --key-pattern=G:G
```

-- dive into tuning more load generation
