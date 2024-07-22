#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <sys/wait.h>

#define PORT 10120
#define TIMEOUT 10

pid_t child_pid = -1;
int sock;
volatile sig_atomic_t timeout_flag = 0;

void myalarm(int sec) {
    if (child_pid != -1) {
        kill(child_pid, SIGTERM);
    }
    if ((child_pid = fork()) < 0) {
        perror("fork failed");
        exit(1);
    } else if (child_pid == 0) {
        sleep(sec);
        kill(getppid(), SIGALRM);
        exit(0);
    } else {
        if (waitpid(child_pid, NULL, WNOHANG) < 0) {
            perror("waitpid failed");
            exit(1);
        }
    }
}

void timeout() {
    timeout_flag = 1;
    if (close(sock) == -1) {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
    exit(0);
}

int main(int argc, char **argv) {
    struct sockaddr_in host;
    struct hostent *hp;
    char buffer[1024], rbuf[1024];
    int nbytes;
    fd_set rfds;
    struct timeval tv;

    if (signal(SIGALRM, timeout) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
    }

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
    myalarm(TIMEOUT);

    while (1) {
        if (timeout_flag) {
            fprintf(stderr, "This program is timeout.\n");
            break;
        }

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
                myalarm(TIMEOUT);
            }
            if (FD_ISSET(sock, &rfds)) { // ソケットからの受信
                memset(rbuf, 0, sizeof(rbuf));
                if ((nbytes = read(sock, rbuf, sizeof(rbuf))) < 0) {
                    perror("read");
                } else if (nbytes > 0) {
                    printf("HOST:%s\n", rbuf);
                }
                myalarm(TIMEOUT);
            }
        }
    }

    close(sock);
    return 0;
}
