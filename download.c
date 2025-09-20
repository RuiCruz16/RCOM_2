#include "download.h"

int parse_url(unsigned char* url, struct URL* parsedUrl) {
    unsigned char* start = url;
    unsigned char* end;
    unsigned char* p;

    end = strchr((char*)start, ':');
    if (!end || strncmp((char*)start, "ftp://", 6) != 0) {
        fprintf(stderr, "Invalid URL format: %s\n", url);
        return 1;
    }
    start = end + 3;

    end = strchr((char*)start, '@');
    if (end) {
        p = strchr((char*)start, ':');
        if (p && p < end) {
            strncpy((char*)parsedUrl->user, (char*)start, p - start);
            parsedUrl->user[p - start] = '\0';
            p++;
            strncpy((char*)parsedUrl->password, (char*)p, end - p);
            parsedUrl->password[end - p] = '\0';
        } else {
            strncpy((char*)parsedUrl->user, (char*)start, end - start);
            parsedUrl->user[end - start] = '\0';
            parsedUrl->password[0] = '\0';
        }
        start = end + 1;
    } else {
        parsedUrl->user[0] = '\0';
        parsedUrl->password[0] = '\0';
    }

    end = strchr((char*)start, '/');
    if (end) {
        strncpy((char*)parsedUrl->host, (char*)start, end - start);
        parsedUrl->host[end - start] = '\0';
        start = end;
    } else {
        strcpy((char*)parsedUrl->host, (char*)start);
        start += strlen((char*)start);
    }

    strcpy((char*)parsedUrl->path, (char*)start);

    struct hostent* h;
    if (strlen((char*)parsedUrl->host) == 0) return -1;

    if ((h = gethostbyname((char*)parsedUrl->host)) == NULL) {
        fprintf(stderr, "Invalid hostname: %s\n", parsedUrl->host);
        exit(-1);
    }
    strcpy((char*)parsedUrl->ip, inet_ntoa(*((struct in_addr*)h->h_addr)));

    if (strlen((char*)parsedUrl->user) == 0) {
        strcpy((char*)parsedUrl->user, "anonymous");
    }
    if (strlen((char*)parsedUrl->password) == 0) {
        strcpy((char*)parsedUrl->password, "anonymous");
    }

    return 0;
}

int create_socket(char* serverIp, int serverPort) {
    int sockfd;
    struct sockaddr_in serverAddr;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);
    serverAddr.sin_port = htons(serverPort);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creating socket");
        exit(-1);
    }

    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error connecting to server");
        close(sockfd);
        exit(-1);
    }

    return sockfd;
}

int read_server_response(const int socket, char* responseBuffer) {
    int totalBytesRead = 0;
	char tempBuffer[MAX_LENGTH];
    memset(responseBuffer, 0, MAX_LENGTH);
    memset(tempBuffer, 0, MAX_LENGTH);

    while (1) {
        int bytesRead = recv(socket, tempBuffer + totalBytesRead, MAX_LENGTH - totalBytesRead - 1, 0);

        if (bytesRead < 0) {
            perror("Error reading from socket");
            return -1;
        } else if (bytesRead == 0) {
            fprintf(stderr, "Server closed the connection\n");
            return -1;
        }

        totalBytesRead += bytesRead;
		tempBuffer[totalBytesRead] = '\0';
		
		char* line = tempBuffer;
		char* nextLine;
		int finalResponseCode = -1;
		int foundFinalLine = 0;
		
		while ((nextLine= strstr(line, "\n"))) {
			*nextLine = '\0';
			int responseCode;
			if (sscanf(line, "%d", &responseCode) == 1 &&
				responseCode >= 100 && responseCode < 600) {
				if (line[3] == ' ') {
					finalResponseCode = responseCode;
					foundFinalLine = 1;
				}
			}
			
			line = nextLine + 1;
			*nextLine = '\n';
		}
		
		if (foundFinalLine) {
			strcpy(responseBuffer, tempBuffer);
			return finalResponseCode;
		}
		
		if(totalBytesRead >= MAX_LENGTH -1) {
			fprintf(stderr, "Response too long\n");
			return -1;
		}
    }
	return -1;
}

int authenticate_user(const int socket, const char* user, const char* password) {
    char response[MAX_LENGTH];
    char command[MAX_LENGTH];

    snprintf(command, MAX_LENGTH, "USER %s\r\n", user);
    write(socket, command, strlen(command));
    if (read_server_response(socket, response) != USER_OK) {
        fprintf(stderr, "Invalid username\n");
        return -1;
    }

    snprintf(command, MAX_LENGTH, "PASS %s\r\n", password);
    write(socket, command, strlen(command));
    if (read_server_response(socket, response) != LOGIN_SUCCESS) {
        fprintf(stderr, "Invalid password\n");
        return -1;
    }

    return 0;
}

int enter_passive_mode(const int socket, char* ip, int* port) {
    char response[MAX_LENGTH];
    write(socket, "PASV\r\n", 6);

    if (read_server_response(socket, response) != PASV_CODE) {
        fprintf(stderr, "Error entering passive mode\n");
        return -1;
    }
	
    int h1, h2, h3, h4, p1, p2;
    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        fprintf(stderr, "Failed to parse PASV response\n");
        return -1;
    }

    snprintf(ip, MAX_LENGTH, "%d.%d.%d.%d", h1, h2, h3, h4);
    *port = p1 * 256 + p2;

    return 0;
}

int request_resource(const int socket, char* resource) {
    char cmd[MAX_LENGTH];
    snprintf(cmd, MAX_LENGTH, "RETR %s\r\n", resource);
    write(socket, cmd, strlen(cmd));
    char response[MAX_LENGTH];
    return read_server_response(socket, response);
}

int get_resource(const int cmdSocket, const int dataSocket, char* resource) {
    const char* lastSlash = strrchr(resource, '/');
    const char* filename = (lastSlash != NULL) ? (lastSlash + 1) : resource;

    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    char buf[MAX_LENGTH];
    int bytesRead;
    while ((bytesRead = recv(dataSocket, buf, MAX_LENGTH, 0)) > 0) {
        if (fwrite(buf, 1, bytesRead, file) < bytesRead) {
            perror("Error writing to file");
            fclose(file);
            return -1;
        }
    }

    if (bytesRead < 0) {
        perror("Error reading data socket");
        fclose(file);
        return -1;
    }

    fclose(file);
    close(dataSocket);

    char response[MAX_LENGTH];
    if (read_server_response(cmdSocket, response) != TRANSFER_DONE) {
        fprintf(stderr, "Error: Transfer not completed successfully\n");
        return -1;
    }

    return 0;
}


int close_connection(const int controlSocket, const int dataSocket) {
    write(controlSocket, "QUIT\r\n", 6);
    char response[MAX_LENGTH];
    if (read_server_response(controlSocket, response) != CLOSE_CONNECTION) {
        fprintf(stderr, "Error closing connection\n");
        return -1;
    }

    if (dataSocket > 0) {
        close(dataSocket);
    }
    close(controlSocket);
    return 0;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

    struct URL url = {0};
    if (parse_url((unsigned char*)argv[1], &url) != 0) {
        fprintf(stderr, "Failed to parse URL\n");
        exit(-1);
    }

    printf("Username: %s\n", url.user);
    printf("Host: %s\n", url.host);
    printf("Passowrd: %s\n", url.password);
    printf("Path: %s\n", url.path);
    printf("IP: %s\n", url.ip);

    int controlSocket = create_socket((char*)url.ip, FTP_PORT);
    char response[MAX_LENGTH];
    if (read_server_response(controlSocket, response) != NEW_CLIENT) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(-1);
    }

    if (authenticate_user(controlSocket, (char*)url.user, (char*)url.password) != 0) {
        fprintf(stderr, "Authentication failed\n");
        exit(-1);
    }

    char dataIp[MAX_LENGTH];
    int dataPort;
    if (enter_passive_mode(controlSocket, dataIp, &dataPort) != 0) {
        exit(-1);
    }

    int dataSocket = create_socket(dataIp, dataPort);
    int responseCode = request_resource(controlSocket, (char*)url.path);
    if (responseCode != READY_TRANSFER && responseCode != CONNECTION_ALREADY_OPEN) {
        fprintf(stderr, "Failed to retrieve resource\n");
        exit(-1);
    }

    if (get_resource(controlSocket, dataSocket, (char*)url.path) != 0) {
        fprintf(stderr, "Error: Transfer failed\n");
        close(dataSocket);
        close(controlSocket);
        exit(-1);
    }

    if (close_connection(controlSocket, dataSocket) != 0) {
        fprintf(stderr, "Failed to close connection\n");
        exit(-1);
    }

    printf("File transfer successful\n");
    return 0;
}
