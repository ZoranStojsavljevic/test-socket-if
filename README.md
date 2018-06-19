## Client - Server code, with some focus on socket CAN implementation.
  The code itself is enhanced, just to pass information between Client and Server
  in the sense that this is the check-in of the Berkley implementation from the
  Year 1980's.
  
  There is one catch 22 in the Server code:
    In the function: void get_ip_address(int socket_fd, struct ifreq *ifr):
    
    The string name of the data link interface "enp0s31f6":
    
	  ifr->ifr_addr.sa_family = AF_INET;
	  =======> snprintf(ifr->ifr_name, IFNAMSIZ, "enp0s31f6"); <<=======
	  ioctl(socket_fd, SIOCGIFADDR, ifr);

  should change according to the CLI command: # ifconfig!
  
