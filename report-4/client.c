#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PORT 10140

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in host;
    struct hostent *hp;
    char buffer[1024], rbuf[1024];
    int nbytes;
    fd_set rfds;
    struct timeval tv;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s hostname username\n", argv[0]);
        exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    bzero(&host, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(PORT);
    if ((hp = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "unknown host %s\n", argv[1]);
        exit(1);
    }
    bcopy(hp->h_addr, &host.sin_addr, hp->h_length);

    if (connect(sock, (struct sockaddr *)&host, sizeof(host)) < 0) {
        perror("ERROR! : connecting");
        exit(1);
    }

    if ((nbytes = read(sock, rbuf, 17)) < 0) {
        perror("read");
        close(sock);
        exit(1);
    }
    rbuf[nbytes] = '\0';
    if (strcmp(rbuf, "REQUEST ACCEPTED\n") == 0) {
        snprintf(buffer, sizeof(buffer), "%s\n", argv[2]);
        write(sock, buffer, strlen(buffer));

        if ((nbytes = read(sock, rbuf, 20)) < 0) {
            perror("read");
            close(sock);
            exit(1);
        }
        rbuf[nbytes] = '\0';
        if (strcmp(rbuf, "USERNAME REGISTERED\n") == 0) {
            while (1) {
                FD_ZERO(&rfds);
                FD_SET(0, &rfds); 
                FD_SET(sock, &rfds);

                tv.tv_sec = 1;
                tv.tv_usec = 0;

                if (select(sock + 1, &rfds, NULL, NULL, &tv) > 0) {
                    if (FD_ISSET(0, &rfds)) {
                        memset(buffer, 0, sizeof(buffer));
                        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                            perror("fgets");
                            close(sock);
                            exit(1);
                        }
                        size_t len = strlen(buffer);
                        if (len > 0 && buffer[len - 1] == '\n') {
                            buffer[len - 1] = '\0';
                        }
                        write(sock, buffer, strlen(buffer));
                    }
                    if (FD_ISSET(sock, &rfds)) {
                        memset(rbuf, 0, sizeof(rbuf));
                        if ((nbytes = read(sock, rbuf, sizeof(rbuf))) < 0) {
                            perror("read");
                            close(sock);
                            exit(1);
                        } else if (nbytes > 0) {
                            printf("%s\n", rbuf);
                        }
                    }
                }
                if (feof(stdin)) {
                    break;
                }
            }
            close(sock);
            exit(0);
        } else {
            fprintf(stderr, "ERROR: %s\n", rbuf);
            close(sock);
            exit(1);
        }
    } else {
        fprintf(stderr, "ERROR: %s\n", rbuf);
        close(sock);
        exit(1);
    }
}
