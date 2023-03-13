#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;
typedef struct quiz_question {
    char question[256];
    char answers[3][256];
    int correct_answer;
    struct quiz_question* next;
} quiz_question;
int main(int argc, char* argv[])
{
    int sd;			// descriptorul de socket
    struct sockaddr_in server;	// structura folosita pentru conectare 
    // mesajul trimis
    int nr = 0;
    char buf[10];

    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    /* stabilim portul */
    port = atoi(argv[2]);

    /* cream socketul */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr(argv[1]);
    /* portul de conectare */
    server.sin_port = htons(port);

    /* ne conectam la server */
    if (connect(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect().\n");
        return errno;
    }
    fd_set read_fds;
    struct timeval tv;
    int retval;
    int change = 0, number = 0, ok = 0;
    while (1 && number < 11) {
        number++;
        if (change == 0) {
            char msg[100];
            bzero(msg, 100);
            // citirea mesajului primit de la server
            if (read(sd, &msg, sizeof(msg)) < 0)
            {
                perror("[client]Eroare la read() de la server.\n");
                return errno;
            }

            /* afisam mesajul primit */
            printf("[client]Mesajul primit este: %s\n", msg);
            printf("[client]Introduceti numele:\n ");
            fflush(stdout);
            // ștergere setul de descriitori de citire
            FD_ZERO(&read_fds);
            // adăugarea descriitorului standard de intrare (tastatură) în set
            FD_SET(0, &read_fds);

            // setarea intervalului de timp pentru select
            tv.tv_sec = 120; //120 sec
            tv.tv_usec = 0;

            // apelul funcției select
            retval = select(1, &read_fds, NULL, NULL, &tv);

            if (retval == -1) {
                perror("[client]Eroare la select().\n");
                return errno;
            }
            else if (retval) {
                // introducerea mesajului de trimis

                bzero(msg, 100);

                read(0, msg, sizeof(msg));


                // trimiterea mesajului la server
                if (write(sd, &msg, sizeof(msg)) <= 0)
                {
                    perror("[client]Eroare la write() spre server.\n");
                    return errno;
                }
            }
            else {
                // intervalul de timp a fost depășit fără ca să se primească ceva pe descriitorul de citire
                printf("Ai depășit limita de timp pentru transmiterea răspunsului.\n");
                close(sd);
            }
            change = 1;
        }
        else {
            quiz_question q;
            int answer;
            // citirea mesajului primit de la server
            if (read(sd, &q, sizeof(quiz_question)) < 0)
            {
                perror("[client]Eroare la read() de la server.\n");
                return errno;
            }
            /* afisam mesajul primit */
            printf("Intrebare: %s\n", q.question);
            for (int i = 0; i < 3; i++) {
                printf("Raspuns %d: %s", i + 1, q.answers[i]);
            }
            FD_ZERO(&read_fds);
            FD_SET(0, &read_fds);

            // setarea intervalului de timp pentru select
            tv.tv_sec = 60; // 60 de secunde
            tv.tv_usec = 0;

            // apelul funcției select
            retval = select(1, &read_fds, NULL, NULL, &tv);

            if (retval == -1) {
                perror("[client]Eroare la select().\n");
                return errno;
            }
            else if (retval) {
                // s-a primit ceva pe descriitorul de citire (tastatură) în intervalul stabilit
                // citirea răspunsului de la tastatură
                scanf("%d", &answer);
                // trimiterea răspunsului la server
                if (write(sd, &answer, sizeof(answer)) <= 0) {
                    perror("[client]Eroare la write() spre server.\n");
                    return errno;
                }
            }
            else {
                // intervalul de timp a fost depășit fără ca să se primească ceva pe descriitorul de citire
                printf("Ai depășit limita de timp pentru transmiterea răspunsului.\n");
                continue;
            }
        }
    }
    char msg[100];
    bzero(msg, 100);
    // citirea mesajului primit de la server
    if (read(sd, &msg, sizeof(msg)) < 0)
    {
        perror("[client]Eroare la read() de la server.\n");
        return errno;
    }
    /* afisam mesajul primit */
    printf("[client]Castigatorul este: %s\n", msg);

    /* inchidem conexiunea, am terminat */
    close(sd);
}
