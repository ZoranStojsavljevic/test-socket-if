#### Based upon dschanoeh/socketcand
```
  https://github.com/dschanoeh/socketcand
  https://github.com/dschanoeh/socketcand/blob/master/socketcandcl.c
```
#### Common socketcandcl (client) & socketcand (server) setup

To set socketCAN-Fd framework beneath Linux kernel, please, do as root:
```
  lsmod | grep can
  modprobe can
  modprobe can_raw
  modprobe can-bcm
  modprobe can-dev
  modprobe can-gw
  modprobe vcan
  lsmod | grep can
```
To set the socketCAN-Fd framework, the following should be done (also as root):
```
  ip link add dev vcan0 type vcan
  ip link set vcan0 mtu 72
  ip link set dev vcan0 up
  ifconfig
```
#### To make socketcand (server), the following is required:
```
$ cat /etc/socketcand.conf 
  # The network interface the socketcand will bind to
  # listen = "eth0";

  # The port the socketcand is listening on
  port = 28601;

  # List of busses the daemon shall provide access to
  # Multiple busses must be separated with ',' and whitespace
  # is not allowed. eg "vcan0,vcan1"
  busses = "vcan0";

  # Description of the service. This will show up in the discovery beacon
  description = "socketcand";
```
And, after:
```
$ socketcand -v -i vcan0 -p 28601 -l eth1&
$ nc 192.168.1.4 28601
```
#### To make socketcandcl (socketcand client), the following is required:
```
  $ gcc socketcandcl.c -o socketcandcl
  $ ./socketcandcl -v -i vcan0 -p 28601 -s stretch
```
##### Add a new frame for transmission #####
This command adds a new frame to the BCM queue. An interval can be configured to have the frame sent cyclic.

Examples:

Send the CAN frame 123#1122334455667788 every second

    < add 1 0 123 8 11 22 33 44 55 66 77 88 >

Send the CAN frame 123#1122334455667788 every 10 usecs

    < add 0 10 123 8 11 22 33 44 55 66 77 88 >

Send the CAN frame 123#42424242 every 20 msecs

    < add 0 20000 123 4 42 42 42 42 >
