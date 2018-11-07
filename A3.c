//Program created by Ethan Foss. 

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>

void *doctor(void *param);
void *patient(void *param);
static int enqueue(int pID);
static void dequeue(void);
static int queueSize(void);
static int randTime(void);

sem_t waiting;
sem_t doneExam;
sem_t inExam;
pthread_mutex_t reception = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t exam = PTHREAD_MUTEX_INITIALIZER;

int queue[4] = {0,0,0,0};
int head = 0;
int tail = 0;
int goneHome;
int patientCount;
int inRoom = 0;

typedef struct{
    int id;
} patientData;

int main(void){

    //Retrieve user input (desired number of patients)
    int patientNum;
    char userInput[81];
    char *inputTok;
    printf("Enter your desired number of patients:\n");
    fflush(stdout);

    fgets(userInput, 81, stdin);
    inputTok = strtok(userInput, " ");
    patientNum = atoi(inputTok);
    patientCount = patientNum;

    //Create the doctor thread along with the desired number of patient threads
    pthread_t docThread, patThread[patientNum];

    //add error checks (-1 if failed)
    sem_init(&waiting, 1, 0);
    sem_init(&doneExam, 1, 0);
    sem_init(&inExam, 1, 0);

    pthread_create(&docThread, NULL, doctor, NULL);

    patientData *data; 
    data = malloc(4*sizeof(patientData));

    for(int i=0; i<patientNum; i++){
        data->id = i+1;
        pthread_create(&patThread[i], NULL, patient, data);
        data++;
    }

    for(int i=0; i<patientNum; i++){
        pthread_join(patThread[i], NULL);
    }

    free(data);
    pthread_join(docThread, NULL);
    sem_destroy(&waiting);
    sem_destroy(&doneExam);
    sem_destroy(&inExam);

    printf("All patients have been examined.\n");

    return 0;
}

void *doctor(void *param){ 
    while(1){
        if(queueSize() == 0){
            if(goneHome == patientCount){
                pthread_exit(NULL);
            }
            printf("The Doctor is sleeping\n");
        }
        sem_wait(&waiting);
        //Critical section for doctor thread
        int x = randTime();
        printf("The Doctor is examining Patient %d for %d seconds. Chairs occupied = %d\n", queue[0], x, queueSize()-1);
        dequeue();
        sem_post(&inExam);
        sleep(x);
        sem_post(&doneExam);
    }
}

void *patient(void *param){
    patientData *data = param;
    int first = 0;
    int lab = 0;
    while(1){
        reception:
        pthread_mutex_lock(&reception);
        if(queueSize() == 4){
            int x = randTime();
            printf("\tWaiting room full. Patient %d drinking coffee for %d seconds\n", data->id, x);
            pthread_mutex_unlock(&reception);
            sleep(x);
            goto reception;
        } else if(queueSize() == 0 && inRoom == 0){
            printf("Waiting room is empty. Patient %d wakes up the doctor.\n", data->id);
            enqueue(data->id);
            sem_post(&waiting);
        } else if((queueSize() > 0 && queueSize() < 4) || (queueSize() == 0 && inRoom != 0)){
            printf("\tPatient %d is waiting. Chairs occupied = %d\n", data->id, queueSize()+1);
            enqueue(data->id);
            pthread_mutex_unlock(&reception);
        }

        sem_wait(&inExam);
        pthread_mutex_lock(&exam);
        if(first == 0){
            printf("Patient %d getting first examination\n", data->id);
            inRoom = data->id;
            first++;
            pthread_mutex_unlock(&reception);
            sem_wait(&doneExam);
            int x = randTime();
            printf("Patient %d is visiting the lab for %d seconds\n", data->id, x);
            inRoom = 0;
            sem_post(&waiting);
            pthread_mutex_unlock(&exam);
            sleep(x);
            lab++;
        } else if (lab == 1) {
            printf("Patient %d getting second examination\n", data->id);
            inRoom = data->id;
            goneHome++;
            sem_wait(&doneExam);
            printf("Patient %d is going home\n", data->id);
            inRoom = 0;
            sem_post(&waiting);
            pthread_mutex_unlock(&exam);
            pthread_exit(NULL);
        } 
    }
}

static int enqueue(int pID){
    for(int i=0; i<4; i++){
        if(queue[i] == 0){
            queue[i] = pID;
            return 1;
        }
    }
    return 0;
}

static void dequeue(void){
    for(int i=0; i<3; i++){
        queue[i] = queue[i+1];
    }
    queue[3] = 0;
}

static int queueSize(void){
    int i; 
    for(i=0; i<4; i++){
        if(queue[i] == 0){
            return i;
        }
    }
    return i;
}

static int randTime(void){
    srand(time(NULL));
    return (rand() % 5) + 1;
}