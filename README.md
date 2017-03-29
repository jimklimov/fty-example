# Contributor procedure for adding new agent with example

## Procedure

1. Create a repository with new agent anywhere outside the [42ITy
   organization](https://github.com/42ITy).
   Add the following line to the description to avoid early commits:
   `PLEASE DO NOT FORK this repository yet as it will get merged into the
   42ITy organization and may be renamed`.
2. Follow [42ITy Contributor guide](http://42ITy.org/contributing.html).
3. Make sure that your codebase conforms to at least the Minimal Requirements
   listed below; it is of course preferable to score the Bonus Points as well,
   if only to automate verification that the minimal requirements are met (and
   stay met, as the codebase evolves).
4. Create an issue on [FTY repository on GitHub](https://github.com/42ITy/FTY)
   pointing at your repository and providing some information on your new agent,
   especially notifyng if it needs any special dependencies not already pulled
   by existing components (and suggest what corresponding run-time and development
   package names might be in Debian/Ubuntu, and in RHEL/CentOS/OpenSUSE worlds).
5. As soon as you have the approval, your repository with new agent will be
   moved under [42ITy organization](https://github.com/42ITy).
6. Once your new agent has been moved to a repository under the
   [42ITy organization](https://github.com/42ITy), please delete your own
   repository and fork the one from the [42ITy](https://github.com/42ITy)
   to continue with your contributions to the ecosystem.

Reminder for 42ITy project maintainers:
* After cloning the new repository, make sure to rescan Travis CI for the
  [42ITy organization](https://travis-ci.org/profile/42ity) and enable testing
  of the newly added component.
* After cloning the repo, make sure to add the submodule, build recipe entry in
  the `Makefile`, and perhaps additional required Ubuntu packages in `.travis.yml`,
  to 42ITy/FTY build framework.

## Creating new agent 

### Minimal Requirements

* Must be buildable by standard
  [autotools](https://en.wikipedia.org/wiki/GNU_Build_System) procedure, .i.e:
`./autogen.sh && ./configure && ./make`   

* The following `make` targets must work
  * `check` - (unit) tests
  * `memcheck` - same as above, but ran under `valgrind` to make sure no memory
    leaks are introduced
  * `install` - make sure all expected files are installable

* Working debian packaging under `packaging/debian` directory 

### Bonus-point Requirements

* Target for `make distcheck` should pass as well, as the test for clarity of
  your `autotools` integration.
* Travis CI tests (`default` and `valgrind`) should be defined and pass green
  "honestly". You may disable the `cmake` tests and not commit the pre-generated
  `CMake*` and `*.cmake` files, as our project does not currently plan to use
  that tool-chain; you can even add them to the `.gitignore` list of your new
  component. You can verify that tests pass before contributing the new agent
  by enabling Travis CI integration for your original development repository.

### Prerequisities

There are some prerequisities that are not mandatory but will make your (and
our) life so much easier. Most of these are documented in the
[42ITy Contributor guide](http://42ity.org/contributing.html).

A very helpful tool is [zproject](https://github.com/zeromq/zproject), since
it will generate everything from Requirements sections for you, even packaging
metadata and self-testing capabilities.

## Example agent

The example agent has been written to show how to:

* connect to malamute messaging broker
* use STREAM pattern to receive messages from system streams
* use STREAM pattern to publish to system streams
* use MAILBOX pattern to receive  messages from other peers
* use MAILBOX pattern to send messages to other peers

Any new agent will want to do at least one of those things. That is, any new
agent will want to either feed something to the system or listen to system and
react to some events, or entities, or both.

The example itself doesn't make much real-world sense, but demonstrates these
goals perfectly:

* fty-example agent is subscribed to ASSETS stream (to receive asset messages,
  i.e. when anything is being done with assets.)
* fty-example agent publishes to ALERTS stream (to publish alert messages)
* when an asset message arrives on ASSETS stream using STREAM pattern,
  fty-example agent will extract name of asset and publish an alert message to
  ALERTS stream using STREAM pattern
* when fty-example agent receives a message from any peer using MAILBOX pattern
  which is one string frame HELLO, it will reply back to the same exact peer
  with one string frame message WORLD.

There are tests inside the repo showing how we can easily test this functionality
using just the agent interface. 
