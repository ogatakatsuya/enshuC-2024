#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int sock;
    int csock1 = -1;
    int csock2 = -1;
    struct sockaddr_in svr;
    struct sockaddr_in clt1;
    struct sockaddr_in clt2;
    struct hostent *cp1, *cp2;
    socklen_t clen1, clen2;
    int nbytes, reuse;
    char rbuf[1024], buffer[1024];
    fd_set rfds;
    struct timeval tv;

    // ソケットの生成
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    // ソケットアドレス再利用の指定
    reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    // client 受付用ソケットの情報設定
    bzero(&svr, sizeof(svr));
    svr.sin_family = AF_INET;
    svr.sin_addr.s_addr = htonl(INADDR_ANY); // 受付側の IP アドレスは任意
    svr.sin_port = htons(10120); // ポート番号 10120 を介して受け付ける

    // ソケットにソケットアドレスを割り当てる
    if (bind(sock, (struct sockaddr *)&svr, sizeof(svr)) < 0) {
        perror("bind");
        exit(1);
    }

    // 待ち受けクライアント数の設定
    if (listen(sock, 5) < 0) { // 待ち受け数に 5 を指定
        perror("listen");
        exit(1);
    }

    do {
        // クライアントの受付
        clen1 = sizeof(clt1);
        clen2 = sizeof(clt2);
        if ((csock1 = accept(sock, (struct sockaddr *)&clt1, &clen1)) < 0) {
            perror("accept");
            exit(2);
        }
        if ((csock2 = accept(sock, (struct sockaddr *)&clt2, &clen2)) < 0) {
            perror("accept");
            exit(2);
        }

        // クライアントのホスト情報の取得
        cp1 = gethostbyaddr((char *)&clt1.sin_addr, sizeof(struct in_addr), AF_INET);
        cp2 = gethostbyaddr((char *)&clt2.sin_addr, sizeof(struct in_addr), AF_INET);

        if (csock1 > 0 && csock2 > 0) {
            printf("----------Connected----------\n");

            do {
                FD_ZERO(&rfds);
                FD_SET(csock1, &rfds);
                FD_SET(csock2, &rfds);

                int max_fd = (csock1 > csock2) ? csock1 : csock2;

                tv.tv_sec = 1;
                tv.tv_usec = 0;

                if (select(max_fd + 1, &rfds, NULL, NULL, &tv) > 0) {
                    if (FD_ISSET(csock1, &rfds)) {
                        memset(rbuf, 0, sizeof(rbuf));
                        if ((nbytes = read(csock1, rbuf, sizeof(rbuf))) < 0) {
                            perror("read");
                        } else if (nbytes > 0) {
                            printf("Client 1: %s\n", rbuf);
                            if (write(csock2, rbuf, nbytes) < 0) {
                                perror("write to client 2");
                            }
                        }
                    }
                    if (FD_ISSET(csock2, &rfds)) {
                        memset(buffer, 0, sizeof(buffer));
                        if ((nbytes = read(csock2, buffer, sizeof(buffer))) < 0) {
                            perror("read");
                        } else if (nbytes > 0) {
                            printf("Client 2: %s\n", buffer);
                            if (write(csock1, buffer, nbytes) < 0) {
                                perror("write to client 1");
                            }
                        }
                    }
                }
            } while (nbytes != 0);
        }

        close(csock1);
        close(csock2);
        nbytes = 1;
        printf("-------Session Closed--------\n");
    } while (1);

    return 0;
}
