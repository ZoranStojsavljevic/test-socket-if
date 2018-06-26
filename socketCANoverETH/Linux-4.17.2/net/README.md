After carefully read of not so rich documentation, I got the idea how this canethgw - CAN ETH GateWay does work.

It is CAN over classical UDP approach, where UDP is used to carry on application CAN messages to the other side of the UDP/IT protocol stack, using at the end ETH as normal media. Initial varsion routes all CAN frames to ETH side and all received UDP datagrams to CAN messages. Not sure if the current version has added filtering capabilities.
