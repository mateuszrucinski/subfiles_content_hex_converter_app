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
#include <dirent.h>

static struct sembuf buf;
static struct sembuf intBuf;

pid_t k, l, m, n;
int shmid, semid;
int intShmid, intSemid;
char *charBuffer;
int *intBuffer;
pid_t childPid, wpid;
int status;
int isStopped;

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
        kill(m, 3);
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_stop_signal_k1(int signum) {
    if (signum == SIGILL) {
        printf("Received SIGUSR1. Stopping the application.\n");
        isStopped = 1;
        kill(l, 10);
        kill(n, 16);
    }
}

// Signal handler for SIGUSR2 (Resume app)
void handle_resume_signal(int signum) {
    if (signum == SIGTRAP) {
        printf("Received SIGUSR2. Resuming the application.\n");
        isStopped = 0;
        kill(l, 12);
        kill(n, 17);
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_get_isStopped_from_other_process(int signum) {
    if (signum == SIGSEGV) {
        isStopped = 1;
    }
}

//// Signal handler for SIGUSR2 (Resume app)
void handle_get_isRunning_from_other_process(int signum) {
    if (signum == SIGPIPE) {
        isStopped = 0;
    }
}

void handle_for_P1_hex_coding_signal() {
    if(SIGPWR) {
        kill(l, 30);
    }
}


int main() {
    char character[2];
    isStopped = 0;

    // Seed the random number generator with the current time
    srand(time(NULL));

    signal(SIGQUIT, handle_signal_k1);
    signal(SIGILL, handle_stop_signal_k1);
    signal(SIGTRAP, handle_resume_signal);
    signal(SIGSEGV, handle_get_isStopped_from_other_process);
    signal(SIGPIPE, handle_get_isRunning_from_other_process);
    signal(SIGPWR, handle_for_P1_hex_coding_signal);

    shmid = shmget(45281, sizeof(char), 0);
    if (shmid == -1) {
        perror("Uzyskanie dostepu do segmentu pamieci wspoldzielonej");
        exit(1);
    }

    intShmid = shmget(45282, sizeof(int), 0);
    if (intShmid == -1) {
        perror("Uzyskanie dostepu do segmentu pamieci wspoldzielonej");
        exit(1);
    }

    //wskaznik na pamiec char
    charBuffer = (char *) &buf;
    charBuffer = (char *) shmat(shmid, NULL, 0);
    if (charBuffer == NULL) {
        perror("Przylaczenie segmentu pamieci wspoldzielonej");
        exit(1);
    }

    //wskaznik na pamiec int
    intBuffer = (int *) &intBuf;
    intBuffer = (int *) shmat(intShmid, NULL, 0);
    if (intBuffer == NULL) {
        perror("Przylaczenie segmentu pamieci wspoldzielonej (int)");
        exit(1);
    }

    semid = semget(45281, 3, 0);
    if (semid == -1) {
        perror("Otwarcie tablicy semaforow char");
        exit(1);
    }

    intSemid = semget(45282, 3, 0);
    if (intSemid == -1) {
        perror("Otwarcie tablicy semaforow int");
        exit(1);
    }

    //pobranie pidow
    opusc(intSemid, 2, intBuf);
    m = intBuffer[0];
    k = intBuffer[1];
    l = intBuffer[2];
    n = intBuffer[3];
    printf("W P1 dotarly pidy: M: %d, getPPid: %d, P1: %d, getPid: %d, K1: %d, K2: %d\n", m, getppid(), k, getpid(), l, n);

    sleep(3);
    printf("Zabawa plikiem w P1\n");
    struct dirent *de;  // Pointer for directory entry

    // opendir() returns a pointer of DIR type.
    //DIR* dr = opendir("/home/mati/CLionProjects/so-zad-semestralne/elo");

    char argument[49];

    char sciezka[50];

    printf("Podaj sciezke byku:\n");
    scanf(" %s", sciezka);

    strcpy(argument, sciezka);

    DIR *dr = opendir(argument);

    if (dr == NULL) {
        printf("Could not open current directory");
        return 0;
    }

    printf("%s\n", argument);


    // for readdir() funkcja ktora przechodzi przez kazdy zasob, ale rowniez przez rodzica i aktualny plik
    while ((de = readdir(dr)) != NULL) {
        //Czekanie na zapelnienie semafora o id 1
        opusc(semid, 1, buf);
        // ominiecie elementow "." i ".."
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            podnies(semid, 1, buf);
            continue;
        }

        //pelna sciezka
        size_t len1 = strlen(argument);
        size_t len2 = strlen(de->d_name);
        size_t totalLength = len1 + len2 + 1;

        char absolutePath[totalLength];

        // Copy the first string
        strcpy(absolutePath, argument);

        // Concatenate the second string
        strcat(absolutePath, de->d_name);

        printf("%s\n", absolutePath);

        //zapisanie do pamieci wspoldzielonej
        strcpy(charBuffer, absolutePath);

        printf("P1 o PID: %d wysyla sciezke do K1: %s\n", k, charBuffer);

        while (isStopped) {
            pause();
        }

        // Sleep to simulate processing time
        sleep(4);

        podnies(semid, 2, buf);
    }

    sleep(5);
    kill(l, 3);
    kill(n, 3);
    kill(m, 3);
}
