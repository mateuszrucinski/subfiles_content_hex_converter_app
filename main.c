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
#include <pthread.h>
#include <errno.h>

static struct sembuf buf;
static struct sembuf intBuf;

int isStopped;
pid_t k, l, n;
int shmid, semid;
int intShmid, intSemid;
char *charBuffer;
int *intBuffer;
int isRunning;

void podnies(int semid, int semnum, struct sembuf sembuf) {
    sembuf.sem_num = semnum;
    sembuf.sem_op = 1;
    sembuf.sem_flg = 0;

    int result;
    do {
        result = semop(semid, &sembuf, 1);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void opusc(int semid, int semnum, struct sembuf sembuf) {
    sembuf.sem_num = semnum;
    sembuf.sem_op = -1;
    sembuf.sem_flg = 0;

    int result;
    do {
        result = semop(semid, &sembuf, 1);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("Opuszczenie semafora");
        exit(1);
    }
}

void wyczysc() {
    //czyszczenie pamieci i zasobow
    kill(k, SIGINT);
    kill(l, SIGINT);
    kill(n, SIGINT);
    wait(NULL);
    shmdt(charBuffer);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 1, IPC_RMID);
    shmdt(intBuffer);
    shmctl(intShmid, IPC_RMID, NULL);
    semctl(intSemid, 1, IPC_RMID);
}

// Signal handler function
void handle_signal_k1(int signum) {
    if (signum == SIGQUIT) {
        printf("Koniec programu, odebrano sygnal: %d\n", signum);
        kill(k, 1);
        kill(l, 1);
        kill(n, 1);
        sleep(1);
        isRunning = 0;
        exit(0);
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_stop_signal_k1(int signum) {
    if (signum == SIGUSR1) {
        printf("Received SIGUSR1. Stopping the application.\n");
        isStopped = 1;
    }
}

// Signal handler for SIGUSR2 (Resume app)
void handle_resume_signal(int signum) {
    if (signum == SIGUSR2) {
        printf("Received SIGUSR2. Resuming the application.\n");
        isStopped = 0;
    }
}

void handle_for_M_hex_coding_signal() {
    if(SIGPWR) {
        kill(l, 30);
    }
}

int main() {
    // Seed the random number generator with the current time
    srand(time(NULL));
    isStopped = 0;
    isRunning = 1;

    signal(SIGQUIT, handle_signal_k1);
    signal(SIGILL, handle_stop_signal_k1);
    signal(SIGTRAP, handle_resume_signal);
    signal(SIGPWR, handle_for_M_hex_coding_signal);

    //deklaracja tablicy 3 semaforow o wart poczatkowych 1, 0, 0
    semid = semget(45281, 3, IPC_CREAT | 0600);
    if (semid == -1) {
        semid = semget(45281, 3, 0600);
        if (semid == -1) {
            perror("Blad utworzenia tablicy semaforow");
            exit(1);
        }
    } else {
        if (semctl(semid, 0, SETVAL, (int) 1) == -1) {
            perror("Nadanie wartosci semaforowi 0");
            exit(1);
        }
        if (semctl(semid, 1, SETVAL, (int) 0) == -1) {
            perror("Nadanie wartosci semaforowi 1");
            exit(1);
        }
        if (semctl(semid, 2, SETVAL, (int) 0) == -1) {
            perror("Nadanie wartosci semaforowi 2");
            exit(1);
        }
    }

    //deklaracja tablicy 3 semaforow o wart poczatkowych 1, 0, 0
    intSemid = semget(45282, 3, IPC_CREAT | 0600);
    if (intSemid == -1) {
        intSemid = semget(45282, 3, 0600);
        if (intSemid == -1) {
            perror("Blad utworzenia tablicy semaforow");
            exit(1);
        }
    } else {
        if (semctl(intSemid, 0, SETVAL, (int) 1) == -1) {
            perror("Nadanie wartosci semaforowi 3");
            exit(1);
        }
        if (semctl(intSemid, 1, SETVAL, (int) 0) == -1) {
            perror("Nadanie wartosci semaforowi 4");
            exit(1);
        }
        if (semctl(intSemid, 2, SETVAL, (int) 0) == -1) {
            perror("Nadanie wartosci semaforowi 5");
            exit(1);
        }
    }


    //pamiec wspoldzielona typu char
    shmid = shmget(45281, (MAX + 2) * sizeof(char), IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("Utworzenie segmentu pamieci wspoldzielonej");
        exit(1);
    }

    //wskaznik na pamiec char
    charBuffer = (char *) &buf;
    charBuffer = (char *) shmat(shmid, NULL, 0);
    if (charBuffer == NULL) {
        perror("Przylaczenie segmentu pamieci wspoldzielonej");
        exit(1);
    }

    //pamiec wspoldzielona typu int
    intShmid = shmget(45282, sizeof(int), IPC_CREAT | 0600);
    if (intShmid == -1) {
        perror("Utworzenie segmentu pamieci wspoldzielonej (int)");
        exit(1);
    }

    //wskaznik na pamiec int
    intBuffer = (int *) &intBuf;
    intBuffer = (int *) shmat(intShmid, NULL, 0);
    if (intBuffer == NULL) {
        perror("Przylaczenie segmentu pamieci wspoldzielonej (int)");
        exit(1);
    }


    //proces1 producent P1
    if ((k = fork()) == 0) {
        sleep(1);
        //uruchomienie w procesie aplikacje P1
        execl("./p1", "p1", (char *) NULL);
        wyczysc();
        exit(1);
    }
        //proces2 konsument K1
    else if ((l = fork()) == 0) {
        sleep(1);
        //uruchomienie w procesie aplikacje K1&K2
        execl("./k1k2", "k1k2", (char *) NULL);
        wyczysc();
        exit(1);
    }
        //proces3 do zatrzymanania apki i wznowienia
    else if (fork() == 0) {
        if (isStopped) {
            kill(getpid(), 19);
            kill(k, 19);
            kill(l, 19);
        }

    }
        //proces matki
    else {
        //przekazanie pidy procesow przez semafory
        opusc(intSemid, 0, intBuf);
        int tab[4];
        tab[0] = getpid();
        tab[1] = k;
        tab[2] = l;
        tab[3] = 0;

        memcpy(intBuffer, tab, sizeof(tab));

        printf("Wyslano do procesow pidy procesow: M: %d, P1: %d, K1: %d, K2: %d\n", intBuffer[0], intBuffer[1],
               intBuffer[2], intBuffer[3]);

        podnies(intSemid, 1, intBuf);

        sleep(5);

        printf("Wyslano do procesow pidy procesow: M: %d, P1: %d, K1: %d, K2: %d\n", intBuffer[0], intBuffer[1],
               intBuffer[2], intBuffer[3]);

        n = intBuffer[3];

        sleep(1);

        printf("M PID: %d \n", getpid());


        opusc(semid, 0, buf);

        printf("Rozpoczecie wysylania danych\n");

        printf("Zaczynamy zabawe o M: %d\n", getpid());

        sleep(1);

        while (isStopped) {
            pause();
        }

        podnies(semid, 1, buf);

        while(isRunning) {

        }

        //czyszczenie danych
        wyczysc();

        return 0;
    }
    return 0;
}