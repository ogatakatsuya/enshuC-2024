#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PORT 10120

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in host;
    struct hostent *hp;
    char buffer[1024], rbuf[1024];
    int buf_len, nbytes;
    fd_set rfds;
    struct timeval tv;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }

    /* ソケットの生成 */
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    /* host(ソケットの接続先) の情報設定 */
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
    printf("----------Connected----------\n");

    /* クライアントからのメッセージ受信 */
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);   // 標準入力を監視
        FD_SET(sock, &rfds); // ソケットを監視

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if (select(sock + 1, &rfds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(0, &rfds)) { // 標準入力からの入力
                memset(buffer, 0, sizeof(buffer));
                if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                    perror("fgets");
                    exit(1);
                }
                size_t len = strlen(buffer);
                if (len > 0 && buffer[len - 1] == '\n') {
                    buffer[len - 1] = '\0';
                }
                write(sock, buffer, strlen(buffer));
            }
            if (FD_ISSET(sock, &rfds)) { // ソケットからの受信
                memset(rbuf, 0, sizeof(rbuf));
                if ((nbytes = read(sock, rbuf, sizeof(rbuf))) < 0) {
                    perror("read");
                } else if (nbytes > 0) {
                    printf("Host:%s\n", rbuf);
                }
            }
        }
    }

    close(sock);
    exit(0);
}
