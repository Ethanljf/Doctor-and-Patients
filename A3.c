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

sem_t lock;
sem_t emptyChairs;
sem_t waiting;
sem_t inExam;
sem_t walkIn;
sem_t doneExam;
sem_t walkOut;

int queue[4];
int goneHome;
int patientCount;
int inRoom = 0;

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

    //add error checks (-1 if failed)
    sem_init(&lock, 1, 1);
    sem_init(&emptyChairs, 1, 4);
    sem_init(&waiting, 1, 0);
    sem_init(&inExam, 1, 0);
    sem_init(&walkIn, 1, 1);
    sem_init(&doneExam, 1, 0);
    sem_init(&walkOut, 1, 1);

    for(int i=0; i<4; i++){
        queue[i] = 0;
    }

    pthread_create(&docThread, NULL, doctor, NULL);

    patientData **data = malloc(sizeof(patientData**));

    for(int i=0; i<patientNum; i++){
        data[i] = malloc(sizeof(patientData));
        data[i]->id = i+1;
        sem_wait(&walkIn);
        pthread_create(&patThread[i], NULL, patient, data[i]);
    }

    for(int i=0; i<patientNum; i++){
        pthread_join(patThread[i], NULL);
        free(data[i]);
    }

    pthread_join(docThread, NULL);
    sem_destroy(&lock);
    sem_destroy(&emptyChairs);
    sem_destroy(&walkIn);
    sem_destroy(&walkOut);
    sem_destroy(&inExam);
    sem_destroy(&doneExam);
    sem_destroy(&waiting);

    printf("All patients have been cured.\n");

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
        sem_wait(&lock);
        sem_wait(&walkOut);
        //Critical section for doctor thread
        int x = randTime();
        printf("The Doctor is examining Patient %d for %d seconds. Chairs occupied = %d\n", queue[0], x, queueSize()-1);
        dequeue();
        sem_post(&inExam);
        sem_post(&emptyChairs);
        sem_post(&lock);
        sleep(x);
        sem_post(&doneExam);
    }
}

void *patient(void *param){
    patientData *data = param;
    int first = 0;
    int lab = 0;
    while(1){
        sem_wait(&emptyChairs);
        sem_wait(&lock);
        if(queueSize() == 4){
            int x = randTime();
            printf("\tWaiting room full. Patient %d drinking coffee for %d seconds\n", data->id, x);
            sleep(x);
        }
        if(queueSize() == 0 && inRoom == 0){
            printf("Waiting room is empty. Patient %d wakes up the doctor.\n", data->id);
        }
        if((queueSize() > 0 && queueSize() < 4) || (queueSize() == 0 && inRoom != 0)){
            printf("\tPatient %d is waiting. Chairs occupied = %d\n", data->id, queueSize()+1);
        }
        enqueue(data->id);  
        sem_post(&walkIn);
        sem_post(&waiting); 
        sem_post(&lock);

        sem_wait(&inExam);
        if(first == 0){
            printf("Patient %d getting first examination\n", data->id);
            inRoom = data->id;
            first = 1;
            sem_wait(&doneExam);
            inRoom = 0;
            int x = randTime();
            printf("Patient %d is visiting the lab for %d seconds\n", data->id, x);
            sem_post(&walkOut);
            sleep(x);
            lab = 1;
        } else if (lab == 1) {
            printf("Patient %d getting second examination\n", data->id);
            goneHome++;
            inRoom = data->id;
            sem_wait(&doneExam);
            inRoom = 0;
            printf("Patient %d is going home\n", data->id);
            sem_post(&walkOut);
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