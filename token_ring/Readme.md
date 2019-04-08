# Token Ring

It is a token-ring simulation written using UNIX sockets and pthreads.

## Running

If you want to see logs

`python3 ./logger.py`

To build a token ring, start the first client

```
./client ABC 3001 [your IP] 3001 true udp
ABC: opening input socket...
ABC: opening output socket...
ABC: opening multicast socket...
> 
```

Connect other clients to the ring 

```
$ ./client XYZ 4001 [your IP] 3001 false udp
$ ./client DEF 5001 [your IP] 4001 false udp
```

You should see `-- INIT --` messages.
You can communicate

`> XYZ: hello XYZ`

And see how the token travels

`> XYZ <- ABC: hello XYZ`
