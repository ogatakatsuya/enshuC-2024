#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
#define NUMPROCS 4
char filename[] = "counter";

int count1() {
    FILE *ct;
    int count;
    if ((ct = fopen(filename, "r")) == NULL) exit(1);
    fscanf(ct, "%d\n", &count);
    count++;
    fclose(ct);
    if ((ct = fopen(filename, "w")) == NULL) exit(1);
    fprintf(ct, "%d\n", count);
    fclose(ct);
    return count;
}

void sem_op(int sid, int op) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = op;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("semop error");
        exit(1);
    }
}

int main() {
    int i, count, pid, status, sid;
    key_t key;
    FILE *ct;
    setbuf(stdout, NULL);
    count = 0;

    if ((ct = fopen(filename, "w")) == NULL) exit(1);
    fprintf(ct, "%d\n", count);
    fclose(ct);

    if ((key = ftok(".", 1)) == -1) {
        fprintf(stderr, "ftok path does not exist.\n");
        exit(1);
    }

    if ((sid = semget(key, 1, 0666 | IPC_CREAT)) == -1) {
        perror("semget error.");
        exit(1);
    }
    if (semctl(sid, 0, SETVAL, 1) == -1) {  // Initial value set to 1
        perror("semctl error.");
        exit(1);
    }

    for (i = 0; i < NUMPROCS; i++) {
        if ((pid = fork()) == -1) {
            perror("fork failed.");
            exit(1);
        }
        if (pid == 0) {
            sem_op(sid, -1);  // Wait (P)
            count = count1();
            printf("process%d: count = %d\n", i, count);
            sem_op(sid, 1);   // Signal (V)
            exit(0);
        }
    }

    for (i = 0; i < NUMPROCS; i++) {
        wait(&status);
    }

    if (semctl(sid, 0, IPC_RMID) == -1) {  // Clean up semaphore
        perror("semctl remove error.");
        exit(1);
    }

    exit(0);
}
