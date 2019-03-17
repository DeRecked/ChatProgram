#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_LENGTH 512
#define IP_ADDRESS "192.168.10.2"
#define DEFAULT_PORT "49152"

struct ClientSocket
{
	char buffer_in[BUFFER_LENGTH];
	SOCKET socket;
};

char checksum(const char *buff, size_t len)
{
	char chk = 0;
	// Sum elements of buff
	while (len-- != 0)
		chk += *buff++;

	// Return one's compliment of chk
	return ~chk; 
}

DWORD WINAPI receive(struct ClientSocket *newClient)
{	
	int iResult = 0;

	while (1)
	{
		// Clear buffer_in
		memset(newClient->buffer_in, 0, BUFFER_LENGTH);
		
		// Receive first byte of incoming data
		iResult = recv(newClient->socket, newClient->buffer_in, BUFFER_LENGTH, 0);
		
		// Check for recv error
		if (iResult == SOCKET_ERROR)
		{
			printf("recv() error\r\n");
			break;
		}
		else printf("%s", newClient->buffer_in);
	}
	// Breaking out of the while loop will exit the function, and thus close the thread
}

int main()
{
	char buffer_out[BUFFER_LENGTH];
	char check;
	char hello[] = "A new client has connected";
	int iResult = 0;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	struct ClientSocket client = { "", INVALID_SOCKET };
	//SOCKET client = INVALID_SOCKET;
	WSADATA wsa_data;

	printf("Initializing Winsock\r\n");
	
	// Initialize Winsock; calls the ws2_32.dll
	iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	
	// Fills block of memory for hints and sizeof(hints) with all zeros
	ZeroMemory(&hints, sizeof(hints));

	// Sets the address family, socket type, and protocol in the addrinfo struct
	hints.ai_family = AF_INET;				// AF_INET (IPv4), AF_INET6 (IPv6)
	hints.ai_socktype = SOCK_STREAM;		// SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, SOCK_RDM, SOCK_SEQPACKET
	hints.ai_protocol = IPPROTO_TCP;		// IPPROTO_TCP, IPPROTO_UDP

	printf("Resolving IP Address and port\r\n");
	
	// Resolve server address and port
	iResult = getaddrinfo(IP_ADDRESS, DEFAULT_PORT, &hints, &result);
	// If getaddrinfo returns an error
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error: \r\n", iResult);
		WSACleanup();
		return 1;
	}

	printf("Connecting...\r\n");
	
	// Move through "result" linked list looking for an address to connect to; ai_next points to next struct in the list
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		client.socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (client.socket == INVALID_SOCKET)
		{
			printf("socket() failed with error: \r\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(client.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(client.socket);
			client.socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	// Frees address info that was added to addrinfo struct by getaddrinfo function
	freeaddrinfo(result);
	if (client.socket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\r\n");
		WSACleanup();
		return 1;
	}

	printf("Connected to server\r\n");
	
	// Send initial message to tell server that we're ready to chat
	send(client.socket, hello, strlen(hello), 0);

	// Create thread to handle receiving messages in parallel to sending messages
	HANDLE recv_thread = CreateThread(NULL, 0, receive, &client, 0, NULL);

	while (1)
	{
		// Clear buffer_out by writing zero to memory
		memset(buffer_out, 0, BUFFER_LENGTH);

		// Get keyboard input
		fgets(buffer_out, sizeof(buffer_out), stdin);
				
		// Calculate checksum of message
		//check = checksum(buffer_out, strlen(buffer_out));
		//printf("Outgoing checksum: 0x%X\r\n", check);
		
		// Add checksum to end of buffer_out
		//buffer_out[strlen(buffer_out) - 1] = check;
		
		// Add "eom" to end of buffer_out
		//buffer_out[strlen(buffer_out)] = 126;
		//printf("buff + check + eom: %s\r\n", buffer_out);
		
		// Send message
		iResult = send(client.socket, buffer_out, strlen(buffer_out), 0);
		if (iResult <= 0)
			printf("send() failed: %d\r\n", WSAGetLastError());
	}

	printf("Shutting down socket...\r\n");

	// Shutdown socket
	iResult = shutdown(client.socket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown() failed with error: \r\n", WSAGetLastError());
		closesocket(client.socket);
		WSACleanup();
		return 1;
	}
	closesocket(client.socket);
	WSACleanup();
	return 0;
}