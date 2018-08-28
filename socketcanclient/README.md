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
