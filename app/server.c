#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
// Response strings
const char *RESPONSE_200 = "HTTP/1.1 200 OK\r\n\r\n";
const char *RESPONSE_201 = "HTTP/1.1 201 Created\r\n\r\n";
const char *RESPONSE_400 = "HTTP/1.1 404 Bad Request\r\n\r\n";
const char *RESPONSE_404 = "HTTP/1.1 404 Not Found\r\n\r\n";
char response[BUFFER_SIZE]; // Response buffer
char *servedDirectory = NULL;

// Function prototypes
void *handleClient(void *arg);
int parseRequest(int client_fd, char* request);
void sendResponse(int client_fd, const char* response);

int main(int argc, char **argv) {
  if (argc >= 2 && (strncmp(argv[1], "--directory", 11) == 0)) {
    servedDirectory = argv[2];
  }
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);
	
	int server_fd, client_addr_len, connfd;
	struct sockaddr_in client_addr;
	pthread_t thread;
	
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
	
	while((connfd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t*)&client_addr_len))){
		printf("Client accepted\n");
		int *new_sock = malloc(1);
    	*new_sock = connfd;
		if( pthread_create( &thread , NULL ,  handleClient , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }

		printf("Handler assigned\n");
		pthread_detach(thread);
	}

	if (connfd < 0)
    {
        perror("accept failed");
        return 1;
    }

	close(server_fd);
	return 0;
}

void *handleClient(void *arg) {
	int client_fd = *(int *)arg;
	printf("client_fd: %d\n", client_fd);
	char request[BUFFER_SIZE];
	int bytesReceived = recv(client_fd, request, BUFFER_SIZE - 1, 0);
	if (bytesReceived == -1) {
		printf("Receive failed: %s\n", strerror(errno));
		return NULL;
	}
	request[bytesReceived] = '\0';
	printf("********************\n");
	printf("Received request:\n%s", request);
	printf("********************\n");
	parseRequest(client_fd, request);
	close(client_fd);
	return NULL;
}

int parseRequest(int client_fd, char* request) {
	// Extract from request
	char *request_method = strtok(request, " ");
	char *request_path = strtok(NULL, " ");
	char *request_version = strtok(NULL, "\r\n");

	printf("Request method: %s\n", request_method);
	printf("Request path: %s\n", request_path);
	printf("Request version: %s\n", request_version);
	//printf("Tester1: %s\n", strtok(NULL, "\r\n"));
	//printf("Tester2: %s\n", strtok(NULL, "\r\n"));
	//printf("Tester3: %s\n", strtok(NULL, "\r\n"));
	//printf("Tester4: %s\n", strtok(NULL, "\r\n"));
	//printf("Tester5: %s\n", strtok(NULL, "\r\n"));
	//printf("Tester6: %s\n", strtok(NULL, "\r\n"));

	// ********** ********** ********** ********** ********** ********** ********** ********** **********
	if (strcmp(request_method, "POST") == 0) {
		printf("POST request\n");
		for (int i = 0; i < 3; i++) {	// Magic numbers (I'm so sorry)
			strtok(NULL, "\r\n");
		}
		char *file_data = strtok(NULL, "\r\n");
		printf("File data: %s\n", file_data);
		if (strncmp(request_path, "/files/", 7) == 0) {
			char *file = strchr(request_path + 1, '/');
			printf("File path: %s\n", file);	
			char *filepath = strcat(servedDirectory, file);
			FILE *fp = fopen(filepath, "w");
			fprintf(fp, "%s", file_data);
			printf("File %s written with data %s\n", filepath, file_data);
			fclose(fp);	
			sendResponse(client_fd, RESPONSE_201);
			return 0;
		}
	}

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
	if (strncmp(request_path, "/user-agent", 11) == 0) {
		// User-Agent
		printf("User-Agent path\n");
		// Extract what comes after "User-Agent"
    	char *line = strtok(NULL, "\r\n"); // Start tokenizing lines
    	char *user_agent = NULL;

    	// Loop through the headers to find User-Agent
		while (line != NULL) {
			if (strncmp(line, "User-Agent:", 11) == 0) {
				user_agent = strchr(line, ':');
				if (user_agent != NULL) {
					user_agent += 2; // Skip ": "
					break;
				}
			}
			line = strtok(NULL, "\r\n");
		}
		int response_len = strlen(user_agent);
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", response_len, user_agent);
		printf("**********\n%s", response);
		sendResponse(client_fd, response);
	}
    if (strncmp(request_path, "/files/", 7) == 0) {
        char *file = strchr(request_path + 1, '/');
        printf("File path: %s\n", file);
		if (file != NULL) {
			char *filepath = strcat(servedDirectory, file);
			FILE *fp = fopen(filepath, "r");
			printf("File Pointer: %p\n", fp);
			if (fp != NULL) {
				char *buffer[BUFFER_SIZE] = {0};
				int bytes_read = fread(buffer, 1, BUFFER_SIZE, fp);

				if (bytes_read > 0) {
					int response_len = strlen(buffer);
					sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\n\r\n%s", response_len, buffer);
					printf("**********\n%s", response);
					sendResponse(client_fd, response);
				}
			}
			else {
				printf("File not found\n");
				sendResponse(client_fd, RESPONSE_404);
			}
		}
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
	pthread_exit(client_fd);
}