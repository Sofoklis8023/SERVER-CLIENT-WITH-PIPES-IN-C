#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_QUEUE_SIZE 1000
// Δομή που ορίζει το πριεχόμενο των queue_elements
typedef struct {
    char* id;
    char** job;
    int job_size;
    int job_args;
    int queue_position;
    int process_pid;
} Value;
// Δομή που ορίζει τα στοιχεία της ουράς
typedef struct {
    Value queue_elements[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
    int size_of_jobs_running;
} Queue;

// Συνάρτηση για τη δημιουργία νέας ουράς
Queue* queue_create() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->front = -1;
    queue->rear = -1;
    queue->size = 0;
    queue->size_of_jobs_running = 0;
    return queue;
}
// Συνάρτηση για εισαγωγή στοιχείου στην ουρά
void queue_insert(Queue* queue,char** command,int command_size,int pid) {
    if (queue->rear == MAX_QUEUE_SIZE - 1) {
        printf("Η ουρά είναι γεμάτη\n");
        return;
    }
    queue->size++;
    queue->rear++;
    int id;
    if (queue->front == -1){
        queue->front = 0;
        id = 1;
    }
    else{
        id = queue->rear - queue->front +1;
    }
    
    char *str = "job_"; 
    char * job = malloc(sizeof(str)+1);
    sprintf(job,"%s%d",str,id);
    queue->queue_elements[queue->rear].job_size = command_size;
    queue->queue_elements[queue->rear].id = job;
    queue->queue_elements[queue->rear].job = malloc((command_size - 1) * sizeof(char*));
    for(int j = 0; j < command_size -  2;j++){
        queue->queue_elements[queue->rear].job[j] = malloc(strlen(command[j]));
        strcpy(queue->queue_elements[queue->rear].job[j], command[j]);
    }
    
    queue->queue_elements[queue->rear].job[command_size -1] = NULL;
    queue->queue_elements[queue->rear].job_args = command_size - 2;
    queue->queue_elements[queue->rear].process_pid = pid;
    queue->queue_elements[queue->rear].queue_position = queue->rear;
}
// Συνάρτηση για εισαγωγή pid στο στοιχείο της ουράς που βρίσκεται στην θέση queue_pos
int queue_set_pid(Queue *queue,int queue_pos,int pid) {
    if (queue->front == -1 || queue->front > queue->rear) {
        printf("empty queue\n");
        return -1;
    }
    queue->size_of_jobs_running++;
    queue->queue_elements[queue_pos].process_pid = pid;
    return pid;

}
// Συνάρτηση για αφαίρεση στοιχείου από την ουρά με βάση το job_id του
Value queue_pop(Queue* queue,char* job_id) {
    if (queue->front == -1 || queue->front > queue->rear) {
        printf("empty queue\n");
        Value return_null_value;
        return_null_value.id = "null";
        return return_null_value;
    }
    for(int i = queue->front; i <= queue->rear; i++){
        if(strcmp(queue->queue_elements[i].id,job_id) == 0){
            if( queue->queue_elements[i].process_pid != -1){
                queue->size_of_jobs_running--;
            }
            Value return_job = queue->queue_elements[i];
            for(int j = i; j < queue->rear; j ++){
                queue->queue_elements[j+1].queue_position =  queue->queue_elements[j+1].queue_position - 1;
                queue->queue_elements[j] = queue->queue_elements[j+1];
            }
            queue->rear--;
            if(queue->rear == -1){
                queue->front = queue->rear;
            }
            queue->size--;
            
            return return_job;
        }
    }
    Value return_null_value;
    return_null_value.id = "null";
    return return_null_value;
}
// Συνάρτηση για αφαίρεση στοιχείου από την ουρά με βάση το pid του
int queue_pop_pid(Queue* queue,int pid) {
    if (queue->front == -1 || queue->front > queue->rear) {
        return 0;
    }
    for(int i = queue->front; i <= queue->rear; i++){
        if(queue->queue_elements[i].process_pid == pid){
            for(int j = i; j < queue->rear; j ++){
                queue->queue_elements[j+1].queue_position =  queue->queue_elements[j+1].queue_position - 1;
                queue->queue_elements[j] = queue->queue_elements[j+1];
            }
            queue->rear--;
            if(queue->rear == -1){
                queue->front = queue->rear;
            }
            queue->size--;
            queue->size_of_jobs_running--;
            return pid;
        }
    }
    return 0;
}
// Συνάρτηση που επιστρέφει το πρώτο στoιχείο της ουράς
Value queue_front(Queue* queue) {
    return queue->queue_elements[queue->front];
}
Value queue_front_without_pid(Queue *queue){
    Value just_to_return;
    just_to_return.id = "null";
    if (queue->front == -1 || queue->front > queue->rear) {
        printf("queue front: empty queue\n");
        
        return just_to_return;
    }
    for(int i = queue->front; i <= queue->rear; i++){
        if(queue->queue_elements[i].process_pid == -1){
            return queue->queue_elements[i];
        }
    }
    return just_to_return;
}
// Συνάρτηση που επιστρέφει τον δείκτη front
int front(Queue* queue) {
    return queue->front;
}
// Συνάρτηση που επιστρέφει το τελευταίο στιχείο της ουράς
Value queue_rear(Queue* queue) {
    return queue->queue_elements[queue->rear];
}
// Συνάρτηση που επιστρέφει τον δείκτη rear
int rear(Queue* queue) {
    return queue->rear;
}
// Συνάρτηση που επιστρέφει το στοιχείο της ουράς που βρίσκεται στην θέση i
Value job_in_position_i(Queue* queue,int i) {
    if (queue->front == -1 || queue->front > queue->rear) {
        printf("job_in_potision_i: empty queue\n");
    }
    if(i >queue->rear || i < queue->front){
        printf("invalid i");
    }
    return queue->queue_elements[i];
}
