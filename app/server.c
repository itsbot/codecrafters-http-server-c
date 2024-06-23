#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
// Response strings
const char *RESPONSE_200 = "HTTP/1.1 200 OK\r\n\r\n";
const char *RESPONSE_404 = "HTTP/1.1 404 Not Found\r\n\r\n";
char response[BUFFER_SIZE]; // Response buffer

// Function prototypes
int handleClient(int client_fd);
int parseRequest(int client_fd, char* request);
void sendResponse(int client_fd, const char* response);

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);
	
	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	//  Since the tester restarts your program quite often, setting SO_REUSEADDR
	//  ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	
	int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");

	// Call handleClient to handle the client request
	// Return client_fd and request to parseRequest
	handleClient(client_fd);

	close(server_fd);
	return 0;
}

int handleClient(int client_fd) {
	char request[BUFFER_SIZE];
	int bytesReceived = recv(client_fd, request, BUFFER_SIZE - 1, 0);
	if (bytesReceived == -1) {
		printf("Receive failed: %s\n", strerror(errno));
		return -1;
	}
	request[bytesReceived] = '\0';
	return parseRequest(client_fd, request);
}

int parseRequest(int client_fd, char* request) {
	// Extract path from request
	char *request_path = strtok(request, " ");
	request_path = strtok(NULL, " ");
	printf("Request path: %s\n", request_path);
	//int path_len = strlen(request_path);

	// Check if path is valid
	if (strcmp(request_path, "/") == 0) {
		// valid
		printf("Valid path\n");
		sendResponse(client_fd, RESPONSE_200);
	}
	if (strncmp(request_path, "/echo/", 6) == 0) {
		// echo
		printf("Echo path\n");
		// Extract what comes after "/echo/"
		char *echo_response = request_path + 6;
		int response_len = strlen(echo_response);
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", response_len, echo_response);
		sendResponse(client_fd, response);
	}
	else {
		// invalid
		printf("Invalid path\n");
		sendResponse(client_fd, RESPONSE_404);
	}

	return 0;
}

void sendResponse(int client_fd, const char* response) {
	send(client_fd, response, strlen(response), 0);
}