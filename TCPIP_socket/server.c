/****************** SERVER CODE ****************/
// https://www.binarytides.com/server-client-example-c-sockets-linux/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

// From: https://stackoverflow.com/questions/1570511/c-code-to-get-the-ip-address
void get_ip_address(int socket_fd, struct ifreq *ifr) {
	// socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	ifr->ifr_addr.sa_family = AF_INET;
	snprintf(ifr->ifr_name, IFNAMSIZ, "eth1");
	ioctl(socket_fd, SIOCGIFADDR, ifr);

	/* and more importantly */
 	printf("Server IP addr: %s\n", inet_ntoa(((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr));
	printf("Server Port is: %d\n", ntohs(((struct sockaddr_in *)&ifr->ifr_addr)->sin_port));

	// close(socket_fd);
	// return &ifr;
}

int main() {
	int welcomeSocket, newSocket;
	char buffer[1024]; //  bzero(&buffer, sizeof(buffer));
	struct sockaddr_in serverAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;

	bzero(&buffer, sizeof(buffer));

	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol IPPROTO_IP (IP in this case) */
	// welcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	/* 1) Internet domain 2) Stream socket 3) IPPROTO_TCP (TCP in this case) */
	welcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	/*---- Configure settings of the server address struct ----*/
	/* Address family = Internet */
	serverAddr.sin_family = AF_INET;

	/* Set port number, using htons function to use proper byte order */
	serverAddr.sin_port = htons(7891);

	/* Set IP address to all data link interfaces */
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	/* Set IP address to host address */
	//	serverAddr.sin_addr.s_addr = inet_addr("192.168.78.28");
	/* Set all bits of the padding field to 0 */
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	int read_size;
	while(1) {
		/*---- Bind the address struct to the socket ----*/
		bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

		int port = (int) ntohs(serverAddr.sin_port);
		printf("Port is: %d\n", (int) ntohs(serverAddr.sin_port));

		/*---- Listen on the socket, with 5 max connection requests queued ----*/
		if(listen(welcomeSocket, 5)==0) printf("Listening\n");
		else printf("Error\n");

		/*---- Accept call creates a new socket for the incoming connection ----*/
		addr_size = sizeof serverStorage;
		newSocket = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);
		memset(&buffer[0], 0, sizeof(buffer));

		while(1) {
			if ((read_size = recv(newSocket, buffer, 64, 0)) > 0 ) {
				if ('x' == buffer[0]) break;
				/*---- Print the received message ----*/
				printf("Data received from client: %s\n", buffer);
				/* Send message back to client/to the socket of the incoming connection */
				/*---- Get the Client IP address ----*/
				struct sockaddr_in m_addr;
				socklen_t len = sizeof m_addr;
				getpeername(newSocket, (struct sockaddr*)&m_addr, &len);
				strcpy(buffer, "Client IP addr: ");
				strcat(buffer, inet_ntoa(m_addr.sin_addr));
				printf("Client IP addr: %s\n", inet_ntoa(m_addr.sin_addr));
				printf("Client Port is: %d\n", ntohs(m_addr.sin_port));
				struct ifreq ifr;
				get_ip_address(newSocket, &ifr);
				// strcpy(buffer, inet_ntoa(m_addr.sin_addr));
				send(newSocket, buffer, 64, 0);
			}
		}
	}

	return 0;
}
