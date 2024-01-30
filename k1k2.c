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

pid_t k, l, m, n;
int shmid, semid;
int intShmid, intSemid;
char* charBuffer;
int* intBuffer;
int isStoppedK1;
int isStoppedK2;
int isHexCoded;

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
void handle_signal_k1(int signum) {
    if (signum == SIGQUIT) {
        kill(m, 3);
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_stop_signal_k1(int signum) {
    if (signum == SIGILL) {
        printf("Received SIGUSR1. Stopping the application.\n");
        isStoppedK1 = 1;
        kill(k, 11);
        kill(n, 16);
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_stop_signal_k2(int signum) {
    if (signum == SIGILL) {
        printf("Elo byku Stopping the application.\n");
        isStoppedK2 = 1;
        kill(k, 11);
        kill(l, 10);
    }
}

void handle_resume_signal_k1(int signum) {
    if (signum == SIGTRAP) {
        printf("Received SIGUSR2. Resuming the application.\n");
        isStoppedK1 = 0;
        kill(k, 13);
        kill(n, 17);
    }
}

void handle_resume_signal_k2(int signum) {
    if (signum == SIGTRAP) {
        printf("Jamnio. Resuming the application.\n");
        isStoppedK2 = 0;
        kill(k, 13);
        kill(l, 12);
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_for_K1_get_isStopped_from_other_process(int signum) {
    if (signum == SIGUSR1) {
        isStoppedK1 = 1;
    }
}

//// Signal handler for SIGUSR2 (Resume app)
void handle_for_K1_get_isRunning_from_other_process(int signum) {
    if (signum == SIGUSR2) {
        isStoppedK1 = 0;
    }
}

//// Signal handler for SIGUSR1 (Stop app)
void handle_for_K2_get_isStopped_from_other_process(int signum) {
    if (signum == SIGSTKFLT) {
        isStoppedK2 = 1;
    }
}

//// Signal handler for SIGUSR2 (Resume app)
void handle_for_K2_get_isRunning_from_other_process(int signum) {
    if (signum == SIGCHLD) {
        isStoppedK2 = 0;
    }
}

void handle_hex_coding_signal() {
    if(SIGPWR) {
        if(isHexCoded == 1) {
            printf("Kodowanie wylaczone\n");
            isHexCoded = 0;
        } else {
            printf("Kodowanie wlaczone\n");
            isHexCoded = 1;
        }
    }
}

void handle_for_K2_hex_coding_signal() {
    if(SIGPWR) {
        kill(l, 30);
    }
}



int main() {
    isHexCoded = 1;
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

    // dane do komunikacji K2
    // dane do pipe
    int pdes1[2];

    pipe(pdes1);
    if (pipe(pdes1) == -1) {
        perror("Błąd przy tworzeniu potoku 1");
        return 1;
    }

    // dane do komunikacji K2
    // dane do pipe
    int pdes2[2];

    pipe(pdes2);
    if (pipe(pdes2) == -1) {
        perror("Błąd przy tworzeniu potoku 1");
        return 1;
    }

    //process k2
    if ((n = fork()) == 0) {
        isStoppedK2 = 0;

        signal(SIGQUIT, handle_signal_k1);
        signal(SIGILL, handle_stop_signal_k2);
        signal(SIGTRAP, handle_resume_signal_k2);
        signal(SIGSTKFLT, handle_for_K2_get_isStopped_from_other_process);
        signal(SIGCHLD, handle_for_K2_get_isRunning_from_other_process);
        signal(SIGPWR, handle_for_K2_hex_coding_signal);

        // zamkniecie deskryptora zapisu
        close(pdes2[1]);
        int pidId[4];
        while(k == 0 || l == 0 || m == 0) {
            read(pdes2[0], pidId, sizeof(pidId));
            m = pidId[0];
            k = pidId[1];
            l = getppid();
        }
        close(pdes2[0]);

        while(1) {
            char hexAbsolutePath[256]; // Assuming a maximum size
            // Close the unused ends of the pipes
            close(pdes1[1]);
            ssize_t bytesRead = read(pdes1[0], hexAbsolutePath, sizeof(hexAbsolutePath));
            // zamkniecie deskryptora odczytu

            // Null-terminate the received string
            hexAbsolutePath[bytesRead] = '\0';
            // Print the received hexadecimal representation
            printf("K2 received: %s\n", hexAbsolutePath);

            // Sleep to simulate processing time
            sleep(4);

//            close(pdes1[0]);

            while(isStoppedK2) {
                pause();
            }
        }

    }
    //proces k1
    else {
        printf("Here i am pid: %d\n", getpid());
        isStoppedK1 = 0;
        signal(SIGQUIT, handle_signal_k1);
        signal(SIGILL, handle_stop_signal_k1);
        signal(SIGTRAP, handle_resume_signal_k1);
        signal(SIGUSR1, handle_for_K1_get_isStopped_from_other_process);
        signal(SIGUSR2, handle_for_K1_get_isRunning_from_other_process);
        signal(SIGPWR, handle_hex_coding_signal);

        //pobranie pidow
        opusc(intSemid, 1, intBuf);
        m = intBuffer[0];
        k = intBuffer[1];
        l = intBuffer[2];

        sleep(2);

        //saving n pid value to intBuffer
        intBuffer[3] = n;

        printf("W K1 dotarly pidy: M: %d, getPPid: %d, P1: %d, getPid: %d, K1: %d, K2: %d\n", m, getppid(), k, getpid(), l, n);

        // zamkniecie deskryptora odczytu
        close(pdes2[0]);
        write(pdes2[1], intBuffer, sizeof(intBuffer));
        // zamkniecie deskryptora zapisu
        close(pdes2[1]);

        podnies(intSemid, 2, intBuf);

        while (1) {
            opusc(semid, 2, buf);

            // Find the length of the string
            size_t bufferLength = strlen(charBuffer);

            // Allocate memory for the hexadecimal representation
            // (twice the length of the original string + 1 for the null terminator)
            size_t hexBufferLength = 2 * bufferLength + 1;
            char hexAbsolutePath[hexBufferLength];

            // Iterate through each character, convert to hexadecimal, and store in hexAbsolutePath
            for (size_t i = 0; i < bufferLength; i++) {
                //char w hex zajmuje 2 miejsca (ASCI) zatem przechodze co drugi adres w hexAbsolutePath aby zapelnic wartosci
                sprintf(hexAbsolutePath + 2 * i, "%02x", charBuffer[i]);
            }

            // Null-terminate the hexadecimal string
            hexAbsolutePath[2 * bufferLength] = '\0';

            while(isStoppedK1) {
                pause();
            }

            if(isHexCoded) {
                printf("%s-:-%s\n", charBuffer, hexAbsolutePath);
            } else {
                printf("%s-:-%s\n", charBuffer, charBuffer);
                strcpy(hexAbsolutePath, charBuffer);
            }

            sleep(5);

            while(isStoppedK1) {
                pause();
            }

            // zamkniecie deskryptora odczytu
            close(pdes1[0]);
            write(pdes1[1], hexAbsolutePath, hexBufferLength);

//            // zamkniecie deskryptora zapisu
//            close(pdes1[1]);

            podnies(semid, 1, buf);
        }
    }
}