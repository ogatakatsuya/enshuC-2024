#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 10140
#define MAXCLIENTS 5

void echoUserList(char *, char [MAXCLIENTS][50]);
int usernameExists(const char *, char [MAXCLIENTS][50]);
int getUserSocket(const char *, char [MAXCLIENTS][50], int [MAXCLIENTS]);
void broadcastMessage(const char *, int [MAXCLIENTS], int);

int main(int argc, char **argv) {
    int sock;
    int client_socks[MAXCLIENTS];
    struct sockaddr_in svr, clt[MAXCLIENTS];
    struct hostent *cp[MAXCLIENTS];
    socklen_t clen[MAXCLIENTS];
    int nbytes, reuse, i, max_fd, k = 0;
    char rbuf[1024], buffer[1024], usernames[MAXCLIENTS][50];
    fd_set rfds;
    struct timeval tv;

    // Initialize client sockets array
    for (i = 0; i < MAXCLIENTS; i++) {
        client_socks[i] = -1;
        memset(usernames[i], 0, sizeof(usernames[i]));
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    bzero(&svr, sizeof(svr));
    svr.sin_family = AF_INET;
    svr.sin_addr.s_addr = htonl(INADDR_ANY);
    svr.sin_port = htons(PORT);

    // State s1: Assign socket address to socket
    if (bind(sock, (struct sockaddr *)&svr, sizeof(svr)) < 0) {
        perror("bind");
        exit(1);
    }

    // Set the number of waiting clients
    if (listen(sock, 5) < 0) {
        perror("listen");
        exit(1);
    }

    while (1) {
        // State s2: Monitor sockets for input
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        max_fd = sock;

        for (i = 0; i < MAXCLIENTS; i++) {
            if (client_socks[i] > 0) {
                FD_SET(client_socks[i], &rfds);
            }
            if (client_socks[i] > max_fd) {
                max_fd = client_socks[i];
            }
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if (select(max_fd + 1, &rfds, NULL, NULL, &tv) < 0) {
            perror("select");
            exit(1);
        }

        // State s3: Handle input processing
        // New connection request
        if (FD_ISSET(sock, &rfds)) {
            for (i = 0; i < MAXCLIENTS; i++) {
                if (client_socks[i] < 0) {
                    // State s4: Accept connection request
                    clen[i] = sizeof(clt[i]);
                    if ((client_socks[i] = accept(sock, (struct sockaddr *)&clt[i], &clen[i])) < 0) {
                        perror("accept");
                        exit(2);
                    }
                    cp[i] = gethostbyaddr((char *)&clt[i].sin_addr, sizeof(struct in_addr), AF_INET);
                    printf("Connected: Client %d\n", i+1);
                    write(client_socks[i], "REQUEST ACCEPTED\n", 17);

                    // State s5: Register username
                    memset(rbuf, 0, sizeof(rbuf));
                    if (read(client_socks[i], rbuf, sizeof(rbuf)) > 0) {
                        rbuf[strcspn(rbuf, "\n")] = '\0';  // Remove newline character
                        if (usernameExists(rbuf, usernames)) {
                            write(client_socks[i], "USERNAME ALREADY EXISTS\n", 25);
                            close(client_socks[i]);
                            client_socks[i] = -1;
                            printf("Duplicate username: %s. Connection rejected.\n", rbuf);
                        } else {
                            strncpy(usernames[i], rbuf, sizeof(usernames[i])-1);
                            printf("Username registered: %s\n", usernames[i]);
                            write(client_socks[i], "USERNAME REGISTERED\n", 20);
                            k++;

                            // Notify all users about the new connection
                            snprintf(buffer, sizeof(buffer), "%s joined. Total users: %d\n", usernames[i], k);
                            broadcastMessage(buffer, client_socks, MAXCLIENTS);
                        }
                    }
                    break;
                }
            }
            if (i == MAXCLIENTS) {
                int temp_sock;
                struct sockaddr_in temp_clt;
                socklen_t temp_clen = sizeof(temp_clt);
                temp_sock = accept(sock, (struct sockaddr *)&temp_clt, &temp_clen);
                write(temp_sock, "REQUEST REJECTED\n", 17);
                close(temp_sock);
            }
        }

        // Handle connection request from clients
        for (i = 0; i < MAXCLIENTS; i++) {
            if (client_socks[i] > 0 && FD_ISSET(client_socks[i], &rfds)) {
                memset(rbuf, 0, sizeof(rbuf));
                if ((nbytes = read(client_socks[i], rbuf, sizeof(rbuf))) < 0) {
                    perror("read");
                } else if (nbytes == 0) {
                    // State s7: Handle client disconnection
                    printf("Client %d disconnected\n", i+1);
                    close(client_socks[i]);
                    client_socks[i] = -1;
                    snprintf(buffer, sizeof(buffer), "%s left. Total users: %d\n", usernames[i], k-1);
                    broadcastMessage(buffer, client_socks, MAXCLIENTS);
                    memset(usernames[i], 0, sizeof(usernames[i]));  // Clear the username
                    k--;
                } else {
                    // State s6: Broadcast message with username or handle DM
                    rbuf[strcspn(rbuf, "\n")] = '\0';  // Remove newline character
                    if (strncmp(rbuf, "send ", 5) == 0) {
                        char *target_username = strtok(rbuf + 5, " ");
                        char *message = strtok(NULL, "\0");
                        int target_sock = getUserSocket(target_username, usernames, client_socks);
                        if (target_sock != -1) {
                            snprintf(buffer, sizeof(buffer), "(DM from %s): %s", usernames[i], message);
                            write(target_sock, buffer, strlen(buffer));
                            printf("DM from %s to %s: %s\n", usernames[i], target_username, message);
                        } else {
                            write(client_socks[i], "USER NOT FOUND\n", 15);
                        }
                    } else if (strcmp(rbuf, "/list") == 0) {
                        echoUserList(buffer, usernames);
                        write(client_socks[i], buffer, strlen(buffer));
                    } else {
                        snprintf(buffer, sizeof(buffer), "%s: %s", usernames[i], rbuf);
                        printf("%s\n", buffer);
                        for (int j = 0; j < MAXCLIENTS; j++) {
                            if (j != i && client_socks[j] > 0) {
                                write(client_socks[j], buffer, strlen(buffer));
                            }
                        }
                    }
                }
            }
        }
    }

    close(sock);
    return 0;
}

void echoUserList(char *buffer, char name_list[MAXCLIENTS][50]) {
    buffer[0] = '\0';  // Clear the buffer
    strcat(buffer, "Users connected:\n");
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (name_list[i][0] != '\0') {
            strcat(buffer, name_list[i]);
            strcat(buffer, "\n");
        }
    }
}

int usernameExists(const char *username, char name_list[MAXCLIENTS][50]) {
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (strcmp(name_list[i], username) == 0) {
            return 1;
        }
    }
    return 0;
}

int getUserSocket(const char *username, char name_list[MAXCLIENTS][50], int sockets[MAXCLIENTS]) {
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (strcmp(name_list[i], username) == 0) {
            return sockets[i];
        }
    }
    return -1;  // User not found
}

void broadcastMessage(const char *message, int sockets[MAXCLIENTS], int max_clients) {
    for (int i = 0; i < max_clients; i++) {
        if (sockets[i] > 0) {
            write(sockets[i], message, strlen(message));
        }
    }
}
