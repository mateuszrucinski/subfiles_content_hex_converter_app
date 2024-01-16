#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define MAX 10

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>

static struct sembuf buf;
static struct sembuf intBuf;

pid_t k, l, m;
int shmid, semid;
int intShmid, intSemid;
char* charBuffer;
int* intBuffer;
int isStopped;

void podnies(int semid, int semnum, struct sembuf sembuf){
    sembuf.sem_num = semnum;
    sembuf.sem_op = 1;
    sembuf.sem_flg = 0;

    int result;
    do {
        result = semop(semid, &sembuf, 1);
    } while (result == -1 && errno == EINTR);

    if (result == -1){
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void opusc(int semid, int semnum, struct sembuf sembuf){
    sembuf.sem_num = semnum;
    sembuf.sem_op = -1;
    sembuf.sem_flg = 0;

    int result;
    do {
        result = semop(semid, &sembuf, 1);
    } while (result == -1 && errno == EINTR);

    if (result == -1){
        perror("Opuszczenie semafora");
        exit(1);
    }
}


void wyczysc() {
    //czyszczenie pamieci i zasobow
    wait(NULL);
    shmdt(charBuffer);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 1, IPC_RMID);
    shmdt(intBuffer);
    shmctl(intShmid, IPC_RMID, NULL);
    semctl(intSemid, 1, IPC_RMID);
}

// Signal handler function
void handle_signal(int signum) {
    if (signum == SIGQUIT) {
        printf("Koniec programu elo, odebrano sygnal: %d\n", signum);
        kill(k, 1);
        kill(m, SIGINT);
        sleep(1);
        wyczysc();
        exit(0);
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_stop_signal(int signum) {
    if (signum == SIGILL) {
        printf("Received SIGUSR1. Stopping the application.\n");
        isStopped = 1;
        kill(k, 11);
    }
}

// Signal handler for SIGUSR2 (Resume app)
void handle_resume_signal(int signum) {
    if (signum == SIGTRAP) {
        printf("Received SIGUSR2. Resuming the application.\n");
        isStopped = 0;
        kill(k, 13);
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_get_isStopped_from_other_process(int signum) {
    if (signum == SIGUSR1) {
        isStopped = 1;
    }
}

//// Signal handler for SIGUSR2 (Resume app)
void handle_get_isRunning_from_other_process(int signum) {
    if (signum == SIGUSR2) {
        isStopped = 0;
    }
}

int main() {
    // Seed the random number generator with the current time
    srand(time(NULL));

    isStopped = 0;

    signal(SIGQUIT, handle_signal);
    signal(SIGILL, handle_stop_signal);
    signal(SIGTRAP, handle_resume_signal);
    signal(SIGUSR1, handle_get_isStopped_from_other_process);
    signal(SIGUSR2, handle_get_isRunning_from_other_process);

    shmid = shmget(45281, sizeof(char), 0);
    if (shmid == -1)
    {
        perror("Uzyskanie dostepu do segmentu pamieci wspoldzielonej");
        exit(1);
    }

    intShmid = shmget(45282, sizeof(int), 0);
    if (intShmid == -1)
    {
        perror("Uzyskanie dostepu do segmentu pamieci wspoldzielonej");
        exit(1);
    }

    //wskaznik na pamiec char
    charBuffer = (char*)&buf;
    charBuffer= (char *) shmat(shmid, NULL, 0);
    if (charBuffer == NULL) {
        perror("Przylaczenie segmentu pamieci wspoldzielonej");
        exit(1);
    }

    //wskaznik na pamiec int
    intBuffer = (int*)&intBuf;
    intBuffer = (int*)shmat(intShmid, NULL, 0);
    if (intBuffer == NULL) {
        perror("Przylaczenie segmentu pamieci wspoldzielonej (int)");
        exit(1);
    }

    semid = semget(45281, 3, 0);
    if (semid == -1)
    {
        perror("Otwarcie tablicy semaforow char");
        exit(1);
    }

    intSemid = semget(45282, 3, 0);
    if (intSemid == -1)
    {
        perror("Otwarcie tablicy semaforow int");
        exit(1);
    }

    //pobranie pidow
    opusc(intSemid, 2, intBuf);
    m = intBuffer[0];
    k = intBuffer[1];
    l = intBuffer[2];
    printf("W K1K2 dotarly pidy: M: %d, getPPid: %d, P1: %d, getPid: %d, K1K2: %d\n", m, getppid(), k, getpid(), l);

    while (1) {
        opusc(semid, 2, buf);
        printf("K1 przyjal wartosc: %c\n", charBuffer[0]);

        // Sleep to simulate processing time
        sleep(5);

        while(isStopped) {
            pause();
        }

        podnies(semid, 0, buf);
    }

}
