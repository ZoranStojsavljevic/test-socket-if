## Client - Server code, with some focus on socketCAN implementation

  The code itself is enhanced, just to pass information between Client and Server
  in the sense that this is the check-in of the Berkley implementation from the
  Y1980s.

  Both flavours were tried:

  	/* 1) Default protocol IPPROTO_IP (IP in this case) */
  	welcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	/* 2) IPPROTO_TCP (TCP in this case) */
	welcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  For 3) UDP (IPPROTO_UDP), there is another code to be designed (simplier one),
  since UDP pprotocol is connectionless oriented!

  There is one catch 22 in the Server code:
    In the function: void get_ip_address(int socket_fd, struct ifreq *ifr):

    The string name of the data link interface "enp0s31f6":

	  ifr->ifr_addr.sa_family = AF_INET;
	  =======> snprintf(ifr->ifr_name, IFNAMSIZ, "enp0s31f6"); <<=======
	  ioctl(socket_fd, SIOCGIFADDR, ifr);

  should change according to the CLI command: # ifconfig!
