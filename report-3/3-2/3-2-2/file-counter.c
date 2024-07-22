#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
#define NUMPROCS 4
char filename[]="counter";
int count1()
{
    FILE *ct;
    int count;
    if ((ct=fopen(filename, "r"))==NULL) exit(1);
    fscanf(ct, "%d\n", &count);
    count++;
    fclose(ct);
    if ((ct=fopen(filename, "w"))==NULL) exit(1);
    fprintf(ct, "%d\n", count);
    fclose(ct);
    return count;
}
int main()
{
    int i, count, pid, status, sid;
    struct sembuf sb;
    key_t key;
    FILE *ct;
    setbuf(stdout, NULL); /* set stdout to be unbuffered */
    count = 0;
    if ((ct=fopen(filename, "w"))==NULL) exit(1);
    fprintf(ct, "%d\n", count);
    fclose(ct);

    if ((key = ftok(".", 1)) == -1){
        fprintf(stderr,"ftok path does not exist.\n");
        exit(1);
    }

    if ((sid=semget(key, 1, 0666 | IPC_CREAT)) == -1) {
        perror("semget error.");
        exit(1);
    }
    
    // Initialize semaphore value to 3
    if (semctl(sid, 0, SETVAL, 0) == -1) {
        perror("semctl error.");
        exit(1);
    }

    if ((pid=fork())== -1) {
        perror("fork failed.");
        exit(1);
    }
    if (pid == 0) { /* Child process */
        // wait
        count = count1();
        printf("process0:");
        printf("count = %d\n", count);
        sb.sem_num = 0;
        sb.sem_op = 1;
        sb.sem_flg = 0;
        if (semop(sid, &sb, 1) == -1) {
            perror("sem_signal semop error.");
            exit(1);
        }
        exit(0);
    }
    for (i=1; i<NUMPROCS; i++) {
        if ((pid=fork())== -1) {
            perror("fork failed.");
            exit(1);
        }
        if (pid == 0) { /* Child process */
            // wait
            sb.sem_num = 0;
            sb.sem_op = -1;
            sb.sem_flg = 0;
            if (semop(sid, &sb, 1) == -1) {
                perror("sem_wait semop error.");
                exit(1);
            }
            count = count1();
            printf("process%d:",i);
            printf("count = %d\n", count);
            
            // Release the semaphore
            sb.sem_num = 0;
            sb.sem_op = 1;
            sb.sem_flg = 0;
            if (semop(sid, &sb, 1) == -1) {
                perror("sem_signal semop error.");
                exit(1);
            }
            exit(0);
        }
    }
    for (i=0; i<NUMPROCS; i++) {
        wait(&status);
    }
    exit(0);
}