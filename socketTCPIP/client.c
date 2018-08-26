/****************** CLIENT CODE ****************/
// https://www.binarytides.com/server-client-example-c-sockets-linux/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(){
    int clientSocket;
    char buffer[1024];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;

    bzero(&buffer, sizeof(buffer));

    /*---- Create the socket. The three arguments are: ----*/
    /* 1) Internet domain 2) Stream socket 3) Default protocol IPPROTO_IP (IP in this case) */
    // clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    /* 1) Internet domain 2) Stream socket 3) IPPROTO_TCP (TCP in this case) */
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    /* 1) Internet domain 2) Stream socket 3) IPPROTO_UDP (UDP in this case) */
    // clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_UDP);

    /*---- Configure settings of the server address struct ----*/
    /* Address family = Internet */
    serverAddr.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
    serverAddr.sin_port = htons(7891);
    /* Set IP address to localhost */
    //	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    /* Set IP address to distant server */
    // serverAddr.sin_addr.s_addr = inet_addr("192.168.178.28");
    // serverAddr.sin_addr.s_addr = inet_addr("172.16.71.51");
    serverAddr.sin_addr.s_addr = inet_addr("192.168.1.4");

    /* Set all bits of the padding field to 0 */
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    /*---- Connect the socket to the remote server using the address struct ----*/
    // addr_size = sizeof serverAddr;
    if (connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
	printf("connect failed. Error");
	return 1;
    }

    printf("Client/Server connected!\n");

    int i = 0;

    // Keep communicating with server
    while(i < 7) {
      char asc_i[8];
	// Prepare some data
	// itoa(i, buffer, 10);
	sprintf(asc_i, "%d", i);
	strcpy(buffer, " Server IP addr: [");
	strcat(buffer, asc_i);
	strcat(buffer, "] ");
	strcat(buffer, inet_ntoa(serverAddr.sin_addr));
	printf("Server IP addr: %s\n", inet_ntoa(serverAddr.sin_addr));
	printf("Server Port is: %d\n", ntohs(serverAddr.sin_port));
	// Send these data to server
	if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
	    printf("Client send() failed");
	    return 1;
	}

	/*---- Read the message from the server into the buffer ----*/
	if (recv(clientSocket, buffer, 1024, 0) < 0) {
	    printf("recv() function failed");
	    break;
	}

	/*---- Print the received message ----*/
	printf("Data received: %s\n", buffer);
	i++;
    }

    strcpy(buffer, "x");

    if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
	printf("Client send() failed");
	return 1;
    }

    close(clientSocket);
    return 0;
}
