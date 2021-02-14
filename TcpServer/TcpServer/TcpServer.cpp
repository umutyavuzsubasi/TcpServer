#include <WS2tcpip.h>
#include <windows.h>
#include <signal.h>
#include <stdio.h>
#include <thread>

#pragma comment (lib, "ws2_32.lib")


void signal_callback_handler(int);
int serverThread(void*);

volatile int signalNumber = 0;
SOCKET serverSock;

struct ThreadData
{
	SOCKET clientSock;
	sockaddr_in client;
};


int main(int argc, char* argv[]) {
	int port = 0;
	int iResult;
	int len;

	if (argc == 2) {
		port = atoi(argv[1]);
	}
	else{
		printf("Usage %s Ip port\n", argv[0]);
		printf("Terminating");
		return 1;
	}

	signal(SIGINT, signal_callback_handler);

	//create wsa object
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		printf("Cant start winsock, ERROR: %d\n", wsResult);
		return 1;
	}

	//create socket
	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSock == INVALID_SOCKET) {
		printf("Cant create socket, ERROR: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	//fill hint structre
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	//bind
	iResult = bind(serverSock, (sockaddr*)&hint, sizeof(hint));
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(serverSock);
		WSACleanup();
		return 1;
	}

	//listen
	iResult = listen(serverSock, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(serverSock);
		WSACleanup();
		return 1;
	}


	while (true)
	{
		ThreadData *newClient = new ThreadData;
		len = sizeof(sockaddr_in);
		newClient->clientSock = accept(serverSock, (sockaddr*) &(newClient->client), (socklen_t*)&len);
		if (newClient->clientSock == INVALID_SOCKET){
			printf("Accept Error\n");
			closesocket(serverSock);
			WSACleanup();
			break;
		}
		std::thread clientThread = std::thread(&serverThread, newClient);
		clientThread.detach();
	}

	if (signalNumber != 0) {
		printf("Signal handled signal number : %d\n", signalNumber);
	}

	printf("Terminated\n");
	system("pause");
	return 1;
}


void signal_callback_handler(int signum) {
	closesocket(serverSock);
	WSACleanup();
	signalNumber = signum;
}

int serverThread(void* param){
	ThreadData *clientData = ((ThreadData*)param);

	char clientInput[1024] = "";
	char* ipAddr = NULL;
	char host[NI_MAXHOST];		// Client's remote name
	char service[NI_MAXSERV];	// Service (i.e. port) the client is connect on

	ZeroMemory(host, NI_MAXHOST); 
	ZeroMemory(service, NI_MAXSERV);
	if (getnameinfo((sockaddr*)&clientData->client, sizeof(clientData->client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0){
		//cout << host << " connected on port " << service << endl;
		printf("Client host name: %s port : %s connected \n", host, service);
	}
	while (true)
	{
		int bytes_recv = recv(clientData->clientSock, clientInput, 1024, 0);
		if (bytes_recv == -1){
			printf("Recv Error\n");
			break;
		}

		else if (bytes_recv > 0){
			printf("Data Received from client %s : %s\n ", host, clientInput);
 			int bytes_sent = send(clientData->clientSock, clientInput, strlen(clientInput) + 1, 0);
			if (bytes_sent == -1){
				printf("Send Error\n");
				break;
			}
		}

		else{
			break;
		}
	}

	printf("Client host name : %s port : %s disconnected.\n", host, service);
	closesocket(clientData->clientSock);
	delete clientData;
	return 1;
}


