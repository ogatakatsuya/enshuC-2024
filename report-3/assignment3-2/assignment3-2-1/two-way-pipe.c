#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFSIZE 256
int main(int argc, char *argv[])
{
    char child_buf[BUFSIZE],parent_buf[BUFSIZE];
    int fd_1[2];
    int fd_2[2];
    int pid, msglen, status;

    if (argc != 3) {
        printf("bad argument.\n");
        exit(1);
    }
    if (pipe(fd_1) == -1) {
        perror("pipe1 failed.");
        exit(1);
    }
    if (pipe(fd_2) == -1) {
        perror("pipe2 failed.");
        exit(1);
    }
    if ((pid=fork())== -1) {
        perror("fork failed.");
        exit(1);
    }
    if (pid == 0) { /* Child process */
        close(fd_1[0]);
        close(fd_2[1]);
        msglen = strlen(argv[1]) + 1;
        if (write(fd_1[1], argv[1], msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        if (read(fd_2[0], child_buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        printf("Message from parent process: ");
        printf("%s\n",child_buf);
        exit(0);
    } else { /* Parent process */
        close(fd_1[1]);
        close(fd_2[0]);
        if (read(fd_1[0], parent_buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        if (write(fd_2[1], argv[2], msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        printf("Message from child process: ");
        printf("%s\n",parent_buf);
        wait(&status);
    }
}