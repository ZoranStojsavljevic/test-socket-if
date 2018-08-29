### socketcancl setup

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
To make socketcandcl (socketcand client), the following is required:
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
