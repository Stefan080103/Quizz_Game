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
/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;
int server_should_stop = 0;
typedef struct thData {
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
	char user[50];
	int scor;
	float time;
}thData;
int sd;		//descriptorul de socket 
static void* treat(void*); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void timeout_handler(int signum) {
	server_should_stop = 1;
}
int break_sis = 0;
void set_timeout(int seconds) {
	struct itimerval old, new;
	new.it_interval.tv_usec = 0;
	new.it_interval.tv_sec = 0;
	new.it_value.tv_usec = 0;
	new.it_value.tv_sec = seconds;
	setitimer(ITIMER_REAL, &new, &old);
}
void set_handler() {
	struct sigaction act;
	act.sa_handler = timeout_handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGALRM, &act, NULL);
}
struct thread {
	pthread_t id;
	int cl_1;
	int idThread_1;
	char user_1[50];
	int scor_1;
	float time_1;
	struct thread* next;
};
struct thread* head = NULL;
typedef struct quiz_question {
	char question[256];
	char answers[3][256];
	int correct_answer;
	struct quiz_question* next;
} quiz_question;
quiz_question* Head = NULL;
quiz_question* tail = NULL;
int main()
{
	struct sockaddr_in server;	// structura folosita de server
	struct sockaddr_in from;
	int nr;		//mesajul primit de trimis la client 
	int pid;
	int i = 0;
	struct sigaction act;
	act.sa_handler = timeout_handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	// deschide fisierul de citire
	FILE* fp = fopen("questions.txt", "r");
	if (fp == NULL) {
		perror("Eroare la deschiderea fisierului de citire");
		return 1;
	}



	// citeste intrebarile, raspunsurile si raspunsul corect din fisier
	while (!feof(fp)) {
		// aloca memorie pentru intrebarea curenta
		quiz_question* q = malloc(sizeof(quiz_question));
		if (q == NULL) {
			perror("Eroare la alocarea memoriei pentru intrebarea curenta");
			return 1;
		}

		// citeste intrebarea, raspunsurile si raspunsul corect
		fgets(q->question, 256, fp);
		for (int i = 0; i < 3; i++) {
			fgets(q->answers[i], 256, fp);
		}
		fscanf(fp, "%d\n", &q->correct_answer);

		// adauga intrebarea la sfarsitul listei inlantuite
		q->next = NULL;
		if (tail == NULL) {
			Head = q;
			tail = q;
		}
		else {
			tail->next = q;
			tail = q;
		}
	}
	// parcurge lista inlantuita si afiseaza intrebarile
	quiz_question* p = Head;
	while (p != NULL) {
		printf("Intrebare: %s\n", p->question);
		for (int i = 0; i < 3; i++) {
			printf("Raspuns %d: %s", i + 1, p->answers[i]);
		}
		printf("Raspuns corect: %d\n", p->correct_answer);
		p = p->next;
	}
	// inchide fisierul de citire
	fclose(fp);
	/* crearea unui socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("[server]Eroare la socket().\n");
		return errno;
	}
	/* utilizarea optiunii SO_REUSEADDR */
	int on = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	/* pregatirea structurilor de date */
	bzero(&server, sizeof(server));
	bzero(&from, sizeof(from));

	/* umplem structura folosita de server */
	/* stabilirea familiei de socket-uri */
	server.sin_family = AF_INET;
	/* acceptam orice adresa */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	/* utilizam un port utilizator */
	server.sin_port = htons(PORT);

	/* atasam socketul */
	if (bind(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1)
	{
		perror("[server]Eroare la bind().\n");
		return errno;
	}

	/* punem serverul sa asculte daca vin clienti sa se conecteze */
	if (listen(sd, 2) == -1)
	{
		perror("[server]Eroare la listen().\n");
		fprintf(stderr, "[server]Eroare: %s\n", strerror(errno));
	}
	set_handler();
	set_timeout(300);
	int trimis = 0;
	/* servim in mod concurent clientii...folosind thread-uri */
	while (1)
	{
		int client;
		thData* td; //parametru functia executata de thread     
		int length = sizeof(from);
		if (server_should_stop == 1)
			if (break_sis == i)
				break;

		if (server_should_stop == 0) {
			printf("[server]Asteptam la portul %d...\n", PORT);
			fflush(stdout);
			// client= malloc(sizeof(int));
			/* acceptam un client (stare blocanta pina la realizarea conexiunii) */
			if ((client = accept(sd, (struct sockaddr*)&from, &length)) < 0)
			{
				if (errno == EINTR) {
					// Semnalul a intrerupt apelul la accept().
					// Afisati mesajul de eroare si continuati sa ascultati conexiuni noi.
					perror("[server]Eroare la accept().\n");
					continue;
				}
				else {
					// Alt tip de eroare. Iesiti din bucla.
					perror("[server]Eroare la accept().\n");
					break;
				}
			}
			/* s-a realizat conexiunea, se astepta mesajul */

			// int idThread; //id-ul threadului
			// int cl; //descriptorul intors de accept

			td = (struct thData*)malloc(sizeof(struct thData));
			td->idThread = i++;
			td->cl = client;
			// Creeaza un nou thread
			struct thread* new_thread = (struct thread*)malloc(sizeof(struct thread));
			pthread_create(&new_thread->id, NULL, &treat, td);

		}

	}
	int max_score = -1;
	char winner[50];
	struct thread* current_thread = head;
	while (current_thread != NULL) {
		if (current_thread->scor_1 > max_score) {
			max_score = current_thread->scor_1;
			strcpy(winner, current_thread->user_1);
		}
		current_thread = current_thread->next;
	}
	current_thread = head;
	int min_time = 10000;
	while (current_thread != NULL) {
		if (current_thread->scor_1 == max_score) {
			if (current_thread->time_1 < min_time) {
				min_time = current_thread->time_1;
				strcpy(winner, current_thread->user_1);
			}
		}
		current_thread = current_thread->next;
	}
	char message[100];
	bzero(message, 100);
	strcpy(message, winner);
	current_thread = head;
	while (current_thread != NULL) {
		write(current_thread->cl_1, message, strlen(message) + 1);
		current_thread = current_thread->next;
	}

	current_thread = head;
	while (current_thread != NULL) {
		pthread_join(current_thread->id, NULL);
		current_thread = current_thread->next;
	}

	close(sd);



}
static void* treat(void* arg)
{
	struct thData tdL;
	tdL = *((struct thData*)arg);
	int is_first_ans = 0, i;
	quiz_question* qse = Head;
	struct timespec timeout;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 200000000; // 200 ms

	// adaugă o variabilă pentru a ține minte momentul în care întrebarea curentă a început să fie răspunsă
	time_t current_question_start_time, current_question_end_time;

	while (!server_should_stop) {
		sleep(1);
	}
	tdL.scor = 0; tdL.time = 0;
	int nr = 0;
	while (1 && qse != NULL && nr < 11) {
		nr++;
		if (is_first_ans == 0) {
			// procesati mesajul primit si raspundeti prin apelarea functiei write()
			char response[100] = " te ai conectat la joc , introdu un nume:";
			// construiti raspunsul in variabila response
			int bytes_written = write(tdL.cl, response, sizeof(response));
			if (bytes_written <= 0) {
				printf("[Thread %d]Eroare la write() catre client.\n", tdL.idThread);
				close((intptr_t)arg);
				break;
			}
			is_first_ans = 1;
			printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
			fflush(stdout);
			char buffer[100];
			int bytes_read = read(tdL.cl, buffer, sizeof(buffer));
			if (bytes_read <= 0) {
				if (bytes_read == 0) {
					printf("[Thread %d]Conexiunea s-a inchis.\n", tdL.idThread);
				}
				else {
					printf("[Thread %d]Eroare la read(): %s.\n", tdL.idThread, strerror(errno));
				}
				close((intptr_t)arg);
				break;
			}
			strcpy(tdL.user, buffer);
			printf("[Thread %d]Mesajul a fost receptionat...%d\n", tdL.idThread, tdL.scor);
			fflush(stdout);
		}
		else {
			// înregistrează momentul în care întrebarea curentă a început să fie răspunsă
			time(&current_question_start_time);
			int bytes_written = write(tdL.cl, qse, sizeof(quiz_question));
			if (bytes_written <= 0) {
				printf("[Thread %d]Eroare la write() catre client.\n", tdL.idThread);
				close((intptr_t)arg);
				break;
			}
			printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
			fflush(stdout);
			while ((time(&current_question_end_time) - current_question_start_time) <= 60) {
				int aux;
				int bytes_read = read(tdL.cl, &aux, sizeof(int));
				if (bytes_read <= 0) {
					if (bytes_read == 0) {
						printf("[Thread %d]Conexiunea s-a inchis.\n", tdL.idThread);
					}
					else {
						printf("[Thread %d]Eroare la read(): %s.\n", tdL.idThread, strerror(errno));
					}
					close((intptr_t)arg);
					break;
				}
				printf("[Thread %d]Mesajul a fost receptionat...%d\n", tdL.idThread, aux);
				fflush(stdout);
				// procesează răspunsul primit de la client și trece la întrebarea următoare
				int right_ans[4], recv_ans[4];
				for (i = 1; i <= 3; i++)
					right_ans[i] = recv_ans[i] = 0;
				int right = qse->correct_answer;
				while (right) {
					right_ans[right % 10] = 1;
					right /= 10;
				}
				while (aux) {
					recv_ans[aux % 10] = 1;
					aux /= 10;
				}

				for (i = 1; i <= 3; i++)
				{
					if (right_ans[i] != recv_ans[i])
					{
						i = 999;
						break;
					}
				}
				if (i < 999) {
					tdL.scor++;
					tdL.time += (time(&current_question_end_time) - current_question_start_time);
				}
				break;
			}
			qse = qse->next;
		}
	}
	break_sis++;
	// adauga informatiile despre thread in lista
	struct thread* new_thread = malloc(sizeof(struct thread));
	new_thread->id = tdL.idThread;
	new_thread->cl_1 = tdL.cl;
	strcpy(new_thread->user_1, tdL.user);
	new_thread->scor_1 = tdL.scor;
	new_thread->time_1 = tdL.time;
	new_thread->next = NULL;

	if (head == NULL) {
		head = new_thread;
	}
	else {
		struct thread* current = head;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = new_thread;
	}
	return (NULL);
}
