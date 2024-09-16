#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>


#include "protocol.h"

#define DEBUG
#define MAX_RETRIES 3
#define TIMEOUT 2

void set_socket_timeout(int sockfd, int timeout) {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
}

void calcProtocol_ntoh(struct calcProtocol *cp) {
    cp->type = ntohs(cp->type);
    cp->major_version = ntohs(cp->major_version);
    cp->minor_version = ntohs(cp->minor_version);
    cp->id = ntohl(cp->id);
    cp->arith = ntohl(cp->arith);
    cp->inValue1 = ntohl(cp->inValue1);
    cp->inValue2 = ntohl(cp->inValue2);
    cp->inResult = ntohl(cp->inResult);
}

void print_calcProtocol(struct calcProtocol *cp) {
    printf("Type: %d\n", cp->type);
    printf("Major version: %d\n", cp->major_version);
    printf("Minor version: %d\n", cp->minor_version);
    printf("ID: %d\n", cp->id);
    printf("Arith: %d\n", cp->arith);
    printf("In value 1: %d\n", cp->inValue1);
    printf("In value 2: %d\n", cp->inValue2);
    printf("In result: %d\n", cp->inResult);
    printf("Fl Value 1: %f", cp->flValue1);
    printf("Fl Value 2: %f", cp->flValue2);
    printf("Fl Result: %f", cp->flResult);
}

int main(int argc, char *argv[]){
    if (argc != 2) {
        printf("Usage: %s <DNS|IPv4|IPv6>:<PORT>\n", argv[0]);
        return 0;
    }
    // Find the last colon in the input to split address and port
    char *input = argv[1];
    char *last_colon = strrchr(input, ':');

    if (last_colon == NULL) {
        printf("Invalid input. Use <DNS|IPv4|IPv6>:<PORT>\n");
        return 0;
    }

    // Split the string into address and port
    *last_colon = '\0';
    char *dest_host = input;
    char *dest_port = last_colon + 1;

    #ifdef DEBUG
    printf("Host %s, and port %d.\n",dest_host, atoi(dest_port));
    #endif

    // Set up for getaddrinfo
    // https://stackoverflow.com/questions/755308/whats-the-hints-mean-for-the-addrinfo-name-in-socket-programming
    struct addrinfo hints, *servinfo, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // Allow for both IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;    // UDP datagram sockets

    // The getaddrinfo can automatically resolve the format of the address and do DNS lookup
    int status;
    if ((status = getaddrinfo(dest_host, dest_port, &hints, &servinfo)) != 0) {
        printf("ERROR\n");
        fprintf(stderr, "RESOLVE ISSUE\n");
        return 0;
    }

    // Kudos to Patrik
    // https://github.com/patrikarlos/networkprogramming/blob/master/tcp_basic_client.c
    int s;
    for(res = servinfo; res != NULL; res = res->ai_next) {
        if ((s = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        break;
    }
    if (res == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    // populate the message to be sent
    struct calcMessage message;
    message.type = htons(22);
    message.message = htonl(0);
    message.protocol = htons(17);
    message.major_version = htons(1);
    message.minor_version = htons(0);

    // send the message using sendto
    // use max retries to avoid infinite loop


    // receive the message using recvfrom
    struct calcProtocol protocol_response;
    struct calcMessage message_response;

    char buf[1024];
    int bytes_received;
    set_socket_timeout(s, TIMEOUT);
    int retries = 0;
    while (retries <= MAX_RETRIES) {
        if (retries == MAX_RETRIES) {
            printf("No response after %d retries. Exiting...\n", MAX_RETRIES);
            exit(1);
        }
        if (sendto(s, &message, sizeof(message), 0, res->ai_addr, res->ai_addrlen) == -1) {
            printf("Error sending message\n");
            perror("sendto");
            exit(1);
        }
        printf("Sent message\n");
        bytes_received = recvfrom(s, &buf, sizeof(buf), 0, res->ai_addr, &res->ai_addrlen);
        if (bytes_received == -1) {
            retries++;
        } else {
            printf("Received message\n");
            break;
        }
    }
    if (bytes_received == -1) {
        printf("No response after %d retries. Exiting...\n", MAX_RETRIES);
        exit(1);
    }

    printf("bytes received: %d\n", bytes_received);

    if (bytes_received == sizeof(struct calcProtocol)) {
        #ifdef DEBUG
        printf("Received protocol response\n");
        #endif
        memcpy(&protocol_response, buf, sizeof(struct calcProtocol));
        calcProtocol_ntoh(&protocol_response);
        print_calcProtocol(&protocol_response);
    } else if (bytes_received == sizeof(struct calcMessage)) {
        #ifdef DEBUG
        printf("Received message response\n");
        #endif
        memcpy(&message_response, buf, sizeof(struct calcMessage));
        printf("Message response: %d\n", message_response.message);
    } else {
        printf("Invalid response\n");
        exit(1);
    }
    // calcProtocol_ntoh(&protocol_response);
    // print_calcProtocol(&protocol_response);
    // print the response
    return 0;

}
