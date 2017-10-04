# ionic

## Brief

`ionic` is a global log, which clients can append-to and read-from.
It is based on the CORFU design, with extensions that leverage WAN-Paxos 
allowing it to scale beyond the data-center. It relies on the `smf` project 
to provide much in the way of high-speed RPC and local log writing. 
One might say `ionic` is just a special application of `smf` and `ionic` itself 
is built to support many applications layered on top of the log abstraction.

`ionic` is comprised of three systems:

* Storage Server
* Token Server
* Client


