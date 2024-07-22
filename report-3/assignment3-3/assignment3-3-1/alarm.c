#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFSIZE 256
#define TIMEOUT 10

pid_t child_pid = -1;

void myalarm(int sec) {
    if(child_pid != -1){
        kill(child_pid,SIGTERM);
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
    printf("This program is timeout.\n");
    exit(0);
}
int main() {
    char buf[BUFSIZE];
    if(signal(SIGALRM,timeout) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
    }
    myalarm(TIMEOUT);
    while (fgets(buf, BUFSIZE, stdin) != NULL) {
        printf("echo: %s",buf);
        myalarm(TIMEOUT);
    }
}