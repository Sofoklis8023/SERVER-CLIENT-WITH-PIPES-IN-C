#include  <sys/types.h>
#include  <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "queue.h" 
#include <signal.h>
#define MAXBUFF 1024
#define FIFO1   "/tmp/fifo.1"
#define FIFO2   "/tmp/fifo.2"
#define PERMS   0666

extern int errno;
// Η συνάρτηση που θα κληθεί όταν λάβει το σήμα SIGURS1
void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        printf("Λάβαμε το SIGUSR1 σήμα!\n");
    } 
}
//Ορίζουμε το jobs_queue εδώ γιατί χρησιμοποιειται στο sigchld_handler
Queue *jobs_queue;
// Η συνάρτηση που θα κληθεί όταν λάβει το σήμα SIGCHLD
void sigchld_handler(int signal) {
    if (signal == SIGCHLD) {
        printf("Λάβαμε το SIGCHLD σήμα!\n");
    }
    pid_t pid;
    int status;
    
    // Αναμονή για την ολοκλήρωση της παιδικής διεργασίας
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child %d terminated with exit status %d\n", pid, WEXITSTATUS(status));
        } else {
            printf("Child %d terminated abnormally\n", pid);
        }
        queue_pop_pid(jobs_queue,pid);
    }
    
}







int main(){
    //Άνοιγμα του txt αρχείου που δείχνει ότι ο jobExecutorServer βρίσκεται σε εκτέλεση 
    int filedes;
    if (( filedes = open ("jobExecutorServer.txt", O_CREAT | O_RDWR,0644 ) ) == -1) {
        perror(" creating ");
        exit(1); 
        
    }
    else {
        printf (" Managed to get to the file successfully\n" ); 
        // Γράψιμο του PID στο αρχείο
        pid_t pid = getpid();
        write(filedes,&pid,sizeof(pid));
    }
    //Δημιουργία ουράς για αποθήκευση των  διεργασιών  
    jobs_queue = queue_create();
    //Προκαθορισμένη τιμή Concurrency
    int Concurrency = 1;
    // Καθορισμός του SIGURS1 
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);


    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    // Καθορισμός του SIGCHLD 
    struct sigaction sa_2;
    sa_2.sa_handler = sigchld_handler;
    sa_2.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGCHLD, &sa_2, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    


    int readfd,writefd;
    ssize_t bytes_read;
    int bytes;
    //Άπειρο loop μέχρις ώτου να δωθεί σήμα τερματισμού για τον server
    while (1) {
        //Ελέγχουμε αν χρειάζεται να θέσουμε κάποια διεργασία από την ουρά προς εκτέλεση
        while(jobs_queue->size_of_jobs_running < Concurrency && jobs_queue->size - jobs_queue->size_of_jobs_running > 0){
            //Παίρνουμε το πρώτο στοιχείο της ουράς που είναι σε αναμονή
            Value job_to_run = queue_front_without_pid(jobs_queue);
            if(strcmp(job_to_run.id,"null")){
                int pid = fork();
                if(pid == -1){
                    perror("fork error");
                }
                else if(pid == 0){
                    //child proccess
                    if(execvp(job_to_run.job[0],job_to_run.job) < 0) {
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                }
                else{
                    //parent proccess
                    //Θέτουμε το pid στο στοιχείο που εκτελείται έτσι ώστε όταν τερματισεί να μπορούμε να το κάνουμε pop
                    queue_set_pid(jobs_queue,job_to_run.queue_position,pid);                  
                }
            }
        }
        

        
       

        //Περιμένουμε κάποιο σήμα για να συνεχίσουμε την εκτέλεση
        pause();
        //Ανοίγουμε το fifo1 για να διαβάσουμε από τον commander
        if ( (readfd = open(FIFO1, O_RDONLY)) < 0) {
        }
        //Διαβάζουμε το μέγεθος του argv[1]     
        if((read(readfd, &bytes,sizeof(int))) == -1) {
            close(readfd);
        }
        //Δεσμεύουμε χώρο για το argv[1]
        char *command = malloc(bytes+1);
        //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
        memset(command, 0, bytes+1);
       
        if((read(readfd, command,bytes)) == -1) {
            close(readfd);
        }
        //Εάν η εντολή που έστειλε ο commander είναι issueJob
        if(!(strcmp(command,"issueJob"))){
            //Διαβάζουμε το argc
            int argc;
            if((read(readfd, &argc,sizeof(int))) == -1) {
                perror("server: αδυναμία ανάγνωσης του argc από το  pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Στο παρακάτω for διαβάζουμε τα υπόλοιπα argv του commander
            int size_of_argv;
            char *arguments_for_exec[argc -1];
            int size_of_argv_array[argc-2];
            for(int i = 2;i < argc; i++){
                //Διαβάζουμε το size of argv
                if((bytes_read = read(readfd, &size_of_argv,sizeof(int))) > 0) {
                    size_of_argv_array[i-2] = size_of_argv;
                    //Δέσμευση μνήμης για το argv
                    arguments_for_exec[i-2] = malloc(size_of_argv+1);
                    //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
                    memset(arguments_for_exec[i-2], 0, size_of_argv+1);
                }
                //Διαβάζουμε το argv
                if((bytes_read = read(readfd,arguments_for_exec[i-2],size_of_argv)) == -1) {
                    perror("server: αδυναμία ανάγνωσης του arguments_for_exec[i-2] από το  pipe");
                    close(readfd);
                    exit(EXIT_FAILURE);
                    
                }
                
            }
            close(readfd);
            arguments_for_exec[argc - 2] = NULL;
            //Προσθήκη της εργασίας στην ουρά
            queue_insert(jobs_queue,arguments_for_exec,argc,-1);
            //Παίρνουμε το στοιχείο που μόλις βάλαμε στην ουρά για το στείλουμε στον commander
            Value last_job_in_queue = queue_rear(jobs_queue);
            //Ανοίγουμε το fifo2 για γράψιμο στον commander
            if ((writefd = open(FIFO2, O_WRONLY)) < 0) {
                perror("server: can't open write fifo");
            }
            //Γράφουμε το size of job_id στο pipe
            int size_of_job_id = strlen(last_job_in_queue.id);
            if(write(writefd,&size_of_job_id,sizeof(int))== -1){
                perror("server: αδυναμία εγγραφής του size of job_id στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            //Γράφουμε το  job_id στο pipe
            if(write(writefd,last_job_in_queue.id,size_of_job_id) == -1){
                perror("server: αδυναμία εγγραφής του job_id στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            //Γράφουμε το job στο pipe 
            for(int i = 2;i < argc; i++){
                if((write(writefd,last_job_in_queue.job[i-2],size_of_argv_array[i-2])) == -1) {
                    perror("server: αδυναμία εγγραφής του last_job_in_queue.job[i-2] στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
            }
            //Γράφουμε το queue_position στο pipe
            if(write(writefd,&last_job_in_queue.queue_position,sizeof(int)) == -1){
                perror("server: αδυναμία εγγραφής του last_job_in_queue.queue_position στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            close(writefd);


            //Αποδέσμευση μνήμης
            for(int i = 0; i < argc-1 ; i++){   
                free(arguments_for_exec[i]);
            }
            free(command);
        
        }
        else if(!(strcmp(command,"setConcurrency"))){
            //Διαβάζουμε το Concurrency από το Pipe
            if((read(readfd, &Concurrency,sizeof(int))) == -1) {
                perror("server: αδυναμία ανάγνωσης του concurrency από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            close(readfd);
            free(command);
        }
        else if(!(strcmp(command,"stop"))){
            //Διάβασμα του size of job_id από το pipe
            int size_of_job_id;
            if((read(readfd,&size_of_job_id,sizeof(int))) == -1) {
                perror("server: αδυναμία ανάγνωσης του size_of_job_id από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Δέσμευση μνήμης βάση του size of job_id που διαβάσαμε προηγουμένως
            char *job_id = malloc(size_of_job_id+1);
            //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
            memset(job_id,0,size_of_job_id+1);
            //Διαβάζουμε το job_id από το Pipe
            if((read(readfd, job_id,size_of_job_id)) == -1) {
                perror("server: αδυναμία ανάγνωσης του job_id από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            close(readfd);
            //Αφαιρούμε την Job_id από την ουρά και επιστρέφουμε τα στοιχεία της στον commander
            Value removed_job = queue_pop(jobs_queue,job_id);
            //Ελέγχουμε αν η διεργασία που μας ζήτησε ο commander να τερματίσουμε υπάρχει στην ουρά
            
            if((strcmp(removed_job.id,"null"))){
                //Εάν το pid είναι -1 σημαίνει ότι η εργασία είναι σε αναμονή και θα στείλουμε στον commander removed
                if(removed_job.process_pid == -1){
                    //Άνοιγμα του fifo2  για γράψιμο στον commander
                    if ((writefd = open(FIFO2, O_WRONLY)) < 0) {
                        perror("server: can't open write fifo");
                    }
                    char message_to_commander[100];
                    sprintf(message_to_commander, "%s removed", removed_job.id);
                    int size_of_message = strlen(message_to_commander);
                    //Γράψιμο του size of message στον commander
                    if((write(writefd,&size_of_message,sizeof(int))) == -1){
                        perror("server: αδυναμία εγγραφής του size_of_message στο pipe");
                        close(writefd);
                        exit(EXIT_FAILURE);
                    }
                    //Γράψιμο του  message στον commander
                    if((write(writefd,message_to_commander,size_of_message)) == -1){
                        perror("server: αδυναμία εγγραφής του message στο pipe");
                        close(writefd);
                        exit(EXIT_FAILURE);
                    }
                    close(writefd);
                }
                //Εάν το pid είναι διάφορο του -1 σημαίνει ότι η εργασία εκτελείται και θα στείλουμε στον commander terminated
                else{
                    //Άνοιγμα του fifo2  για γράψιμο στον commander
                    if ((writefd = open(FIFO2, O_WRONLY)) < 0) {
                        perror("server: can't open write fifo");
                    }
                    char message_to_commander[100];
                    sprintf(message_to_commander, "%s  terminated", removed_job.id);
                    int size_of_message = strlen(message_to_commander);
                    //Γράψιμο του size of message στον commander
                    if((write(writefd,&size_of_message,sizeof(int))) == -1){
                        perror("server: αδυναμία εγγραφής του size_of_message στο pipe");
                        close(writefd);
                        exit(EXIT_FAILURE);
                    }
                    //Γράψιμο του  message στον commander
                    if((write(writefd,message_to_commander,size_of_message)) == -1){
                        perror("server: αδυναμία εγγραφής του message στο pipe");
                        close(writefd);
                        exit(EXIT_FAILURE);
                    }
                    close(writefd);
                    //Τερματισμός της διεργασίας με process id = removed_job.process_pid 
                    kill(removed_job.process_pid, SIGTERM);
                }
            }
            //Περίπτωση που η διεργασία δεν υπάρχει στην ουρά στέλνουμε στον commander το μήνυμα this job dont exist
            else{
                //Άνοιγμα του fifo2 για γράψιμο
                if ((writefd = open(FIFO2, O_WRONLY)) < 0) {
                    perror("server: can't open write fifo");
                }
                char *source_message = "this job don't exist";
                char message_to_commander[100];
                strcpy(message_to_commander, source_message);
                int size_of_message = strlen(message_to_commander);
                //Γράψιμο του size of message στο Pipe
                if((write(writefd,&size_of_message,sizeof(int))) == -1){
                    perror("server: αδυναμία εγγραφής του size_of_message στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                //Γράψιμο του message στο Pipe
                if((write(writefd,message_to_commander,size_of_message)) == -1){
                    perror("server: αδυναμία εγγραφής του message στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                close(writefd);
            }
            free(job_id);
            free(command);
        }
        else if(!(strcmp(command,"poll"))){
            int size_of_argv_2;
            //Διαβάζουμε το size of argv[2] από το pipe
            if(read(readfd,&size_of_argv_2,sizeof(int)) == -1){
                perror("server: αδυναμία ανάγνωσης του size_ofargv[2] από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Δεσμέυουμε χώρο ανάλογα με το size of argv[2]
            char * running_or_queued = malloc(size_of_argv_2+1);
            //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
            memset(running_or_queued, 0, sizeof(size_of_argv_2+1));
            //Διαβάζουμε αν ο commander επιθυμεί να του στείλουμε τις εργασιές που είναι σε αναμονή ή τις διεργασίες που εκτελούνται
            if(read(readfd,running_or_queued,size_of_argv_2) == -1){
                perror("server: αδυναμία ανάγνωσης του running_or_queued από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Περίπτωση εργασιών που εκτελούνται
            if(!(strcmp(running_or_queued,"running"))){
                //Ανοίγουμε το fifo2 για γράψιμο στον commander
                if ((writefd = open(FIFO2, O_WRONLY)) < 0) {
                    perror("server: can't open write fifo");
                }
                int jobs_queue_front = front(jobs_queue);
                //Γράφουμε το front στο Pipe
                if(write(writefd,&jobs_queue_front,sizeof(int)) == -1){
                    perror("server: αδυναμία εγγραφής του front στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                int jobs_queue_rear = rear(jobs_queue);
                //Γράφουμε το rear στο Pipe
                if(write(writefd,&jobs_queue_rear,sizeof(int)) == -1){
                    perror("server: αδυναμία εγγραφής του rear στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                int jobs_running =jobs_queue->size_of_jobs_running;
                //Γράφουμε το πόσες εργασίες εκτελούνται αυτή τη στιγμή στο pipe
                if(write(writefd,&jobs_running,sizeof(int)) == -1){
                    perror("server: αδυναμία εγγραφής του jobs_running στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                //Ελέγχουμε αν υπάρχουν διεργασίες υπό εκτέλεση για να στείλουμε στον commander
                if(jobs_queue_front != -1 && jobs_queue_rear != -1 && jobs_running !=0){
                    //Στο παρακάτω for γράφουμε για κάθε διεργασία που εκτελείται το job_id,job,queue_potision στο pipe
                    for(int i = jobs_queue_front; i <= jobs_queue_rear;i++){
                        Value job_i = job_in_position_i(jobs_queue,i);
                        if(job_i.process_pid != -1){
                            int size_of_job_id = strlen(job_i.id);
                            if(write(writefd,&size_of_job_id,sizeof(int)) == - 1){
                                perror("server: αδυναμία εγγραφής του size_of_job_id στο pipe");
                                close(writefd);
                                exit(EXIT_FAILURE);
                            }
                            if(write(writefd,job_i.id,size_of_job_id) == - 1){
                                perror("server: αδυναμία εγγραφής του job_id στο pipe");
                                close(writefd);
                                exit(EXIT_FAILURE);
                            }
                            if(write(writefd,&job_i.job_args,sizeof(int)) == - 1){
                                perror("server: αδυναμία εγγραφής του job_args στο pipe");
                                close(writefd);
                                exit(EXIT_FAILURE);
                            }
                            for(int j = 0; j < job_i.job_args; j++){
                                int size_of_job_elements = strlen(job_i.job[j]);
                                if(write(writefd,&size_of_job_elements,sizeof(int)) == - 1){
                                    perror("server: αδυναμία εγγραφής του size_of_job_elements στο pipe");
                                    close(writefd);
                                    exit(EXIT_FAILURE);
                                }
                                if(write(writefd,job_i.job[j],size_of_job_elements) == - 1){
                                    perror("server: αδυναμία εγγραφής του job_i.job[j] στο pipe");
                                    close(writefd);
                                    exit(EXIT_FAILURE);
                                }

                            }
                            if(write(writefd,&job_i.queue_position,sizeof(int)) == - 1){
                                perror("server: αδυναμία εγγραφής του job_i.queue_potision στο pipe");
                                close(writefd);
                                exit(EXIT_FAILURE);
                            }
                            
                        }
                    }
                }
                else{
                    printf("empty queue\n");
                }
                close(readfd);
                free(running_or_queued);
            
            }
            //Περίπτωση εργασιών που είναι σε αναμονή 
            if(!(strcmp(running_or_queued,"queued"))){
                //Ανοίγουμε το fifo2 για γράψιμο στον commander
                if ((writefd = open(FIFO2, O_WRONLY)) < 0) {
                    perror("server: can't open write fifo");
                }
                int jobs_queue_front = front(jobs_queue);
                //Γράφουμε το front στον commander
                if(write(writefd,&jobs_queue_front,sizeof(int)) == -1){
                    perror("server: αδυναμία εγγραφής του front στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                int jobs_queue_rear = rear(jobs_queue);
                //Γράφουμε το rear στον commander
                if(write(writefd,&jobs_queue_rear,sizeof(int)) == -1){
                    perror("server: αδυναμία εγγραφής του rear στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                int jobs_queued = jobs_queue->size - jobs_queue->size_of_jobs_running;
                //Γράφουμε των αριθμό των διεργασιών που είναι σε αναμονή στον commander
                if(write(writefd,&jobs_queued,sizeof(int)) == -1){
                    perror("server: αδυναμία εγγραφής του jobs_queued στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                //Ελέγχουμε αν υπάρχουν εργασίες σε αναμονή
                if(jobs_queue_front != -1 && jobs_queue_rear != -1 && jobs_queued != 0){
                    //Στο παρακάτω for γράφουμε για κάθε διεργασία που βρίσκεται σε αναμονή το job_id,job,queue_potision στο pipe
                    for(int i = jobs_queue_front; i <= jobs_queue_rear;i++){
                        Value job_i = job_in_position_i(jobs_queue,i);
                        if(job_i.process_pid == -1){
                            int size_of_job_id = strlen(job_i.id);
                            if(write(writefd,&size_of_job_id,sizeof(int)) == - 1){
                                perror("server: αδυναμία εγγραφής του size_of_job_id στο pipe");
                                close(writefd);
                                exit(EXIT_FAILURE);
                            }
                            if(write(writefd,job_i.id,size_of_job_id) == - 1){
                                perror("server: αδυναμία εγγραφής του size_of_job_id στο pipe");
                                close(writefd);
                                exit(EXIT_FAILURE);
                            }
                            if(write(writefd,&job_i.job_args,sizeof(int)) == - 1){
                                perror("server: αδυναμία εγγραφής του job_args στο pipe");
                                close(writefd);
                                exit(EXIT_FAILURE);
                            }
                            for(int j = 0; j < job_i.job_args; j++){
                                int size_of_job_elements = strlen(job_i.job[j]);
                                if(write(writefd,&size_of_job_elements,sizeof(int)) == - 1){
                                    perror("server: αδυναμία εγγραφής του size_of_job_elements στο pipe");
                                    close(writefd);
                                    exit(EXIT_FAILURE);
                                }
                                if(write(writefd,job_i.job[j],size_of_job_elements) == - 1){
                                    perror("server: αδυναμία εγγραφής του job_i.job[j] στο pipe");
                                    close(writefd);
                                    exit(EXIT_FAILURE);
                                }

                            }
                            if(write(writefd,&job_i.queue_position,sizeof(int)) == - 1){
                                perror("server: αδυναμία εγγραφής του job_i.queue_potision στο pipe");
                                close(writefd);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
                else{
                    printf("empty queue\n");
                }
                
                close(readfd);
                free(running_or_queued);
            }
            


            free(command);
        }
        else if(!(strcmp(command,"exit"))){
            //Αν υπάρχουν διεργασίες υπό εκτέλεση τις τερματίζουμε πρωτού κλείσουμε το αρχείο 
            int jobs_queue_front = front(jobs_queue); 
            int jobs_queue_rear = rear(jobs_queue);
            if(jobs_queue_front != -1 && jobs_queue_rear != -1 && jobs_queue->size_of_jobs_running != 0){
                for( int i = jobs_queue_front; i <= jobs_queue_rear; i++){
                    Value job = job_in_position_i(jobs_queue,i);
                    if(job.process_pid != -1){
                        kill(job.process_pid, SIGTERM);
                    }
                }
            }
            //Διαγράφουμε το jobExecutorServer.txt
            char * filename = "jobExecutorServer.txt";
            if (remove(filename) == 0) {
                printf("%s διαγράφτηκε επιτυχώς.\n", filename);
                if ((writefd = open(FIFO2, O_WRONLY)) < 0) {
                    perror("server: can't open write fifo");
                }
                //Στέλνουμε στον commander το μήνυμα jobExecutorServer terminated
                char * exit_message = "jobExecutorServer terminated";
                int size_of_exit_message = strlen(exit_message);
                //Γράφουμε στο Pipe το size του exit message
                if((write(writefd,&size_of_exit_message,sizeof(int))) == -1){
                    perror("server: αδυναμία εγγραφής του size_of_exit_message στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                //Γράφουμε στο pipe το exit message
                if((write(writefd,exit_message,size_of_exit_message)) == -1){
                    perror("server: αδυναμία εγγραφής του exit_message στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                close(writefd);
                exit(0);
            } else {
                printf("Αδυναμία διαγραφής του %s.\n", filename);
            }


            free(command);
        }
        
    }

}
