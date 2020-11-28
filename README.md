# MQ-Bus
## Goal
Provide a Dbus-free way for IPC using message queues.

## Install
make install

## Examples
Multiple senders one receivers
```
$ echo a | mqsend test
$ echo b | mqsend test
$ echo c | mqsend test
$ mqreceive test
a
b
c
```

Multiple receivers, single sender
```
$ mqbus-receive test &
$ mqbus-receive test &
$ mqbus-receive test &
$ echo event | mqbus-send test
event
event
event
```
