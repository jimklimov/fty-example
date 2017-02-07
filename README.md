# Contributor procedure for adding new agent with example

## Procedure

1. Create a repository with new agent anywhere outside the [42ITy
   organization](https://github.com/42Ity)
2. Follow [Policy on 42ITy community interaction and governance](to be done by @aquette)
3. As soon as you have the approval, your repository with new agent will be
   moved under [42ITy organization](https://github.com/42Ity)

## Creating new agent 

### Requirements

* Must be buildable by standard
  [autotools](https://en.wikipedia.org/wiki/GNU_Build_System) procedure, .i.e:
`./autogen.sh && ./configure && ./make`   

* The following `make` targets must work
  * `check` - (unit) tests
  * `memcheck` - same as above, but ran under valgrind to make sure no memory
    leaks are introduced
  * `install` 
* Working debian packaging under `packaging/debian` directory 

### Prerequisities

There are some prerequisities that are not mandatory but will make your life so much easier.
(Note to self: need to collect all Arnaud's links ...; think it would be much
better to merge various documents that we have...)

Most important of this is [zproject](https://github.com/zeromq/zproject), since
it  will generate everything from Requirements section for you, even packaging.

## Example agent

The example agent has been written to show how to

* connect to malamute messaging broker
* use STREAM pattern to receive messages from system streams
* use STREAM pattern to publish to system streams
* use MAILBOX pattern to receive  messages from other peers
* use MAILBOX pattern to send messages to other peers

Any new agent will want to do at least one of those things. I.e. any new agent
will want to either feed something to the system or listen to system and react on some events/entities or both.

The example itself doesn't make much real-world sense, but demonstrates these goals perfectly:

* fty-example agent is subscribed to ASSETS stream (to receive asset messages,
  i.e. when anything is being done with assets.)
* fty-example agent publishes to ALERTS stream (to publish alert messages)
* when an asset message arrives on ASSETS stream using STREAM pattern,
  fty-example agent will extract name of asset and publish an alert message to
ALERTS stream using STREAM pattern
* when fty-example agent receives a message from any peer using MAILBOX pattern
  which is one string frame HELLO, it will reply back to the same exact peer
with one string frame message WORLD.

There are tests inside the repo showing how we can easily test this functionality using just the agent interface. 






