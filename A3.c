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
static void enqueue(pthread_t patient);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
sem_t s;

static pthread_t *queue;

int main(void){

    int patientNum;
    char userInput[81];
    char *inputTok;
    printf("Enter your desired number of patients:\n");
    fflush(stdout);

    fgets(userInput, 81, stdin);
    inputTok = strtok(userInput, " ");
    patientNum = atoi(inputTok);

    pthread_t docThread, patThread[patientNum];
    sem_t s = sem_init(&s, 0, 1);

    pthread_create(&docThread, NULL, doctor, NULL);

    for(int i=0; i<patientNum; i++){
        pthread_create(&patThread[i], NULL, patient, NULL);
        enqueue(patThread[i]);
        printf("Patient %d is going home\n", i);
        pthread_join(patThread[i], NULL);
    }

    pthread_join(docThread, NULL);
    sem_destroy(&s);

    return 0;
}

void *doctor(void *param){ 
    while(1){
        printf("\tDoctor is sleeping\n");
        sem_wait(&s);
        //Critical section for doctor thread
        printf("The Doctor is examining patient\n");
        sem_post(&s);
    }
}

void *patient(void *param){
    while(1){
    }
}

static void enqueue(pthread_t patient){
    int i;
    
    for(i=0; queue[i]!=NULL; i++);

    queue[i] = patient;
}