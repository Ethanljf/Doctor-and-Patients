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

pthread_mutex_t exam = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
sem_t s;

static int queue[4];
static int goneHome;
static int patientCount;

typedef struct{
    int id; 
} patientData;

int main(void){

    int patientNum;
    char userInput[81];
    char *inputTok;
    printf("Enter your desired number of patients:\n");
    fflush(stdout);

    fgets(userInput, 81, stdin);
    inputTok = strtok(userInput, " ");
    patientNum = atoi(inputTok);
    patientCount = patientNum;

    pthread_t docThread, patThread[patientNum];
    sem_t s = sem_init(&s, 0, 1);

    pthread_create(&docThread, NULL, doctor, NULL);

    patientData **data;

    for(int i=0; i<patientNum; i++){
        data[i] = (patientData *) malloc(sizeof(patientData));
        data[i]->id = i+1;
        pthread_create(&patThread[i], NULL, patient, data[i]);
    }

    for(int i=0; i<patientNum; i++){
        pthread_join(patThread[i], NULL);
        free(data[i]);
    }

    pthread_join(docThread, NULL);
    sem_destroy(&s);

    return 0;
}

void *doctor(void *param){ 
    int sleeping = 0;
    while(1){
        while(queue[0] == NULL){
            if(goneHome == patientCount){
                pthread_exit(NULL);
            }
            if(sleeping == 0){
                printf("Doctor is sleeping\n");
                sleeping = 1;
                sleep(1);
            }
        }
        sleeping = 0;
        sem_wait(&s);
        //Critical section for doctor thread
        int x = randTime();
        printf("The Doctor is examining patient %d for %d seconds. Chairs occupied = %d\n", queue[0], x, queueSize());
        dequeue();
        sleep(x);
        sem_post(&s);
    }
}

void *patient(void *param){
    patientData *data = param;
    int first = 0;
    int second = 0;
    int lab = 0;
    while(1){

        sem_wait(&s);
            if(queueSize() == 0){
                printf("Waiting room is empty. Patient %d wakes up the doctor.\n", data->id);
                enqueue(data->id);
            }
        sem_post(&s);

        sem_wait(&s);
            if(queueSize() == 4){
                int x = randTime();
                printf("Waiting room full. Patient %d drinking coffee for %d seconds\n", data->id, x);
                sleep(x);
            }
        sem_post(&s);

        sem_wait(&s);
            if(first == 0 && queue[0] != data->id){
                printf("Patient %d is waiting. Chairs occupied = %d\n", data->id, queueSize());
                enqueue(data->id);
            }
        sem_post(&s);
        
        sem_wait(&s);
            if(first == 0 && queue[0] == data->id){
                printf("Patient %d getting first examination\n", data->id);
                first = 1;
            }
        sem_post(&s);

        sem_wait(&s);
            if(first == 1){
                int x = randTime();
                printf("Patient %d is visiting the lab for %d seconds\n", data->id, 1);
                sleep(x);
                lab = 1;
            }
        sem_post(&s);

        sem_wait(&s);
            if(lab == 1){
                printf("Patient %d getting second examination\n", data->id);
                second = 1;
            }
        sem_post(&s);

        sem_wait(&s);
            if(second == 1){
                printf("Patient %d is going home\n", data->id);
                goneHome++;
                pthread_exit(NULL);
            }
        sem_post(&s);
    }
}

static int enqueue(int pID){
    for(int i=0; i<4; i++){
        if(queue[i] == NULL){
            queue[i] = pID;
            return 1;
        }
    }
    return 0;
}

static void dequeue(void){    
    for(int i=3; i>0; i--){
        queue[i-1] = queue[i];
    }
    queue[3] = NULL;
}

static int queueSize(void){
    int i;
    
    for(i=0; i<4; i++){
        if(queue[i] == NULL){
            return i;
        }
    }
    return i;
}

static int randTime(void){
    srand(time(NULL));
    return (rand() % 5) + 1;
}
