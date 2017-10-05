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

## Storage Server

The Storage Server basically maps down to a `smf` wal. For a given log it
exposes one or more Storage Units. Each Storage Unit contains multiple extents,
each extent contains a contiguous range of log positions. The global projection
maps ranges -> extents -> units. Replication will group a set of units up per range.

In CORFU there is a global log, and each position maps down to a local position on a 
 (one or more) Storage Server(s).

The map is called the Projection Map, and this map is redistributed during reconfiguration.

The projection map can change during Storage Unit failure, and also when writes move past 
the active range.


