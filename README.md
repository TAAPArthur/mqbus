# MQ-Bus
## Goal
Provide a Dbus-free way for IPC using message queues.

More generally, this provides a way to develop applications as if they just
wrote to/read from stdin/out. And if the app even needs to be a daemon, just
pipe it to/from one of the provided scrips

## Install
```
make
make install
```

## Examples
### Multiple senders one receivers
```
$ echo a | mqsend test
$ echo b | mqsend test
$ echo c | mqsend test
$ mqreceive test
a
b
c
```

### Multiple receivers, single sender
```
$ mqbus-receive test &
$ mqbus-receive test &
$ mqbus-receive test &
$ echo event | mqbus-send test
event
event
event
```
### Bidirectional communication
```
mqreceive bar_to_foo | foo | mqsend foo_to_bar &
mqreceive foo_to_bar | bar | mqsend bar_to_foo
```

## mq vs mqbus

There are 4 programs this repo
mqsend, mqreceive mqbus-send and mqbus-receive

### mq
The mq family supports a multi-producer-single-consumer (MPCS) model. Many
clients can send messages to a single client. These scripts are just wrappers
around the message queue interface.

### mqbus
The mqbus family supports single-producer-multi-consumer (SPMC). Many clients
can receive messages from a single one. This is a bit of a hack and barely uses
message queues. The sender checks a directory denoted by the message queue
name, notices all named pipes in that directory and will forward all messages
to all such pipes. Anytime a new receive is spawned, it will created a named
pipe and place it in the directory and then send a message to the sender to
check for its pipe.

### Comparison
| Description                           | mq | mqbus | Notes |
|---------------------------------------|----|-------|-------|
| May block if there's no receiver      | Y  | N     | For mq, there is a fixed size queue of messages and it'll block if its full |
| Receiver may see old messages         | Y  | N     | For mqbus, receivers will only see messages that occur after it was connected |
| Are all messages sent to all receivers| N  | Y     | If there are multiple mq-receives, messages will be split between them |
| Guaranteed message delivery           | N  | N     | Message may be lost or sent to only some clients on crash |
