#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#define MAX_LENGTH 1024
#define FTP_PORT 21
#define READY_TRANSFER 150
#define NEW_CLIENT 220
#define TRANSFER_DONE 226
#define CLOSE_CONNECTION 221
#define PASV_CODE 227
#define USER_OK 331
#define LOGIN_SUCCESS 230
#define CONNECTION_ALREADY_OPEN 125
#define BLOCKING_MODE 0

struct URL {
    unsigned char user[MAX_LENGTH];
    unsigned char password[MAX_LENGTH];
    unsigned char host[MAX_LENGTH];
    unsigned char path[MAX_LENGTH];
    unsigned char ip[MAX_LENGTH];
};

int parse_url(unsigned char* url, struct URL* parsedUrl);

int create_socket(char* serverIp, int serverPort);

int read_server_response(const int socket, char* responseBuffer);

int authenticate_user(const int socket, const char* user, const char* password);

int enter_passive_mode(const int socket, char* ip, int* port);

int get_resource(const int cmdSocket, const int dataSocket, char* resource);

int request_resource(const int socket, char* resource);
