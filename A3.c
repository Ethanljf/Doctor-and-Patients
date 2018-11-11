//Program created by Ethan Foss and Alex Manuele

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

//Struct used to pass patient data to pthreads
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

    //string parse user input and create num processes specified
    if( !fgets(userInput, 81, stdin)){
	printf("Error: fgets fail\n");
	exit(EXIT_FAILURE);
    }
    inputTok = strtok(userInput, " ");
    patientNum = atoi(inputTok);
    if(patientNum < 1){
      printf("Error: User input must be a positive integer.\n");
      exit(EXIT_FAILURE);
    }
    patientCount = patientNum;

    //Create the doctor thread along with the desired number of patient threads
    pthread_t docThread, patThread[patientNum];

    //initialize semaphores: failure if return value is not 0
    if(sem_init(&waiting, 1, 0) != 0){
      printf("Error: sem_init\n");
      exit(EXIT_FAILURE);
    }
    if(sem_init(&doneExam, 1, 0) != 0){
      printf("Error: sem_init\n");
      exit(EXIT_FAILURE);
    }
    if(sem_init(&inExam, 1, 0) != 0){
      printf("Error: sem_init\n");
      exit(EXIT_FAILURE);
    }
    //create doctor thread: Return != 0  is error
    if(pthread_create(&docThread, NULL, doctor, NULL) != 0){
      printf("Error: Doctor thread create\n");
      exit(EXIT_FAILURE);
    }
    
    //allocate memory for patient threads 
    patientData *data; 
    data = malloc(patientNum*sizeof(patientData));
    //check return value of malloc
    if(!data){
      printf("Error: malloc fail\n");
      exit(EXIT_FAILURE);
    }

    //use allocated data to created the user specified num of patient threads
    //check each pthread_create system call
    for(int i=0; i<patientNum; i++){
      data[i].id = i+1;
	if(pthread_create(&patThread[i], NULL, patient, &data[i]) != 0){
	  printf("Error: patient thread create\n");
	  exit(EXIT_FAILURE);
	}
    }
    //join the threads and check system call
    for(int i=0; i<patientNum; i++){
      if(pthread_join(patThread[i], NULL) != 0){
	printf("Error: pthread_join\n");
	exit(EXIT_FAILURE);
      }
    }
    //free malloc memory
    free(data); //free doesn't have a return value so can't check success
    
    //join doctor thread and check success
    if(pthread_join(docThread, NULL) != 0){
      printf("Error: pthread_join doctor thread\n");
      exit(EXIT_FAILURE);
    }
    
    //destroy semaphores and check return value; 0 if success
    if(sem_destroy(&waiting) != 0){
      printf("Error: sem_destroy waiting\n");
      exit(EXIT_FAILURE);
    }
    if(sem_destroy(&doneExam) != 0){
      printf("Error: sem_destroy doneExam\n");
      exit(EXIT_FAILURE);
    }
    if(sem_destroy(&inExam) != 0){
      printf("Error: sem_destroy inExam\n");
      exit(EXIT_FAILURE);
    }

    printf("All patients have been examined.\n");

    return 0;
}

//Thread method for Doctor thread
/* doctor() checkc if there are patients in the queue. If there are none, the doctor sleeps and prints a message.
 * If there are patients, the doctor thread requests access to the critical section through the &waiting semaphore.
 * In the critical section, the doctor thread calls randtime to examine the top patient of the queue for a random
 * interval. It does so by dequeing the patient, posting to the &inExam semaphore, sleeping a random interval, and
 * then posting to the &doneExam semaphore.
 */
//sem_wait and sem_post return 0 on success
void *doctor(void *param){ 
    while(1){
        if(queueSize() == 0){
            if(goneHome == patientCount){
                pthread_exit(NULL);
            }
            printf("The Doctor is sleeping\n");
        }
        if(sem_wait(&waiting) != 0){
	  printf("Error: sem_Wait doctor\n");
	  exit(EXIT_FAILURE);
	}
        //Critical section for doctor thread
        int x = randTime();
        printf("The Doctor is examining Patient %d for %d seconds. Chairs occupied = %d\n", queue[0], x, queueSize()-1);
        dequeue();
        if(sem_post(&inExam) != 0){
	  printf("Error: sem_post inExam\n");
	  exit(EXIT_FAILURE);
	}
        sleep(x);
        if(sem_post(&doneExam) != 0){
	  printf("Error: sem_post doneExam\n");
	  exit(EXIT_FAILURE);
	}
    }
}
/* Patient thread method
 * The patient method uses flags to keep track of whether the patient is seeing the doctor initially or waiting for lab results.
 * When a patient is created, it goes to reception and checks the sie of the queue. If the queue has four patients, the wait room 
 * is full. It calls mutex_unlock(&reception), sleeps a random interval, and then checks to queue again.
 * If the queue size is empty and the doctor is not seeing a patient, the patient wakes the doctor- it enqueus itself and posts
 * to the &waiting semaphore. If the queue size is between 0 and 4 and the doctor is busy, the patient enqueues and unlocks the 
 * recption mutex.
 * If the patient has access to &inExam, then its flag is checked to see whether it is in its initial doctor exam or waiting for lab.
 * The patient prints a message and posts/waits on the appropriate semaphores depending on these flags. Upon completing the second
 * visit with the doctor, the pthread exits and increments a counter of the number of patients who have left.
 */
void *patient(void *param){
    patientData *data = param;
    int first = 0;
    int lab = 0;
    while(1){
        reception:
      if(pthread_mutex_lock(&reception) != 0){
	printf("Error: mutex_lock\n");
	exit(EXIT_FAILURE);
      }
        if(queueSize() == 4){
            int x = randTime();
            printf("\tWaiting room full. Patient %d drinking coffee for %d seconds\n", data->id, x);
            if(pthread_mutex_unlock(&reception) != 0){
	      printf("Error: mutex_unlock\n");
	      exit(EXIT_FAILURE);
	    }
            sleep(x);
            goto reception;
        } else if(queueSize() == 0 && inRoom == 0){
            printf("Waiting room is empty. Patient %d wakes up the doctor.\n", data->id);
            enqueue(data->id);
            if(sem_post(&waiting) != 0){
	      printf("Error: sem_post\n");
	      exit(EXIT_FAILURE);
	    }
        } else if((queueSize() > 0 && queueSize() < 4) || (queueSize() == 0 && inRoom != 0)){
            printf("\tPatient %d is waiting. Chairs occupied = %d\n", data->id, queueSize()+1);
            enqueue(data->id);
            if(pthread_mutex_unlock(&reception) != 0){
	      printf("Error: mutex_unlock\n");
	      exit(EXIT_FAILURE);
	    }
		     
        }

        if(sem_wait(&inExam) != 0){
	  printf("Error: sem_wait\n");
	  exit(EXIT_FAILURE);
	}
        if(pthread_mutex_lock(&exam) != 0){
	  printf("Error: mutex_lock\n");
	  exit(EXIT_FAILURE);
	}
        if(first == 0){
            printf("Patient %d getting first examination\n", data->id);
            inRoom = data->id;
            first++;
            if(pthread_mutex_unlock(&reception) != 0){
	      printf("Error\n");
	      exit(EXIT_FAILURE);
	    }
            if(sem_wait(&doneExam) != 0){
	      printf("Error\n");
	      exit(EXIT_FAILURE);
	    }
            int x = randTime();
            printf("Patient %d is visiting the lab for %d seconds\n", data->id, x);
            inRoom = 0;
	    if(patientCount != 1){
	      if(sem_post(&waiting) != 0){
		printf("Error\n");
		exit(EXIT_FAILURE);
	      }
	    }
            if(pthread_mutex_unlock(&exam) != 0){
	      printf("Error\n");
	      exit(EXIT_FAILURE);
	    }
            sleep(x);
            lab++;
        } else if (lab == 1) {
            printf("Patient %d getting second examination\n", data->id);
            inRoom = data->id;
            goneHome++;
            if(sem_wait(&doneExam) != 0){
	      printf("Error\n");
	      exit(EXIT_FAILURE);
	    }
            printf("Patient %d is going home\n", data->id);
            inRoom = 0;
	    if(patientCount != 1){
	      if(sem_post(&waiting) != 0){
		printf("Error\n");
		exit(EXIT_FAILURE);
	      }
	    }
            if(pthread_mutex_unlock(&exam) != 0){
	      printf("Error\n");
	      exit(EXIT_FAILURE);
	    }
            pthread_exit(NULL);
        } 
    }
}

//a method for adding patient id's to the queue
static int enqueue(int pID){
    for(int i=0; i<4; i++){
        if(queue[i] == 0){
            queue[i] = pID;
            return 1;
        }
    }
    return 0;
}
// a pop method to dequeue the top patient
static void dequeue(void){
    for(int i=0; i<3; i++){
        queue[i] = queue[i+1];
    }
    queue[3] = 0;
}
//a method to query the queue size
static int queueSize(void){
    int i; 
    for(i=0; i<4; i++){
        if(queue[i] == 0){
            return i;
        }
    }
    return i;
}
//random number generator, limited to 5 s to reduce time of program run
static int randTime(void){
    srand(time(NULL));
    return (rand() % 5) + 1;
}
