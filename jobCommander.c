#include  <sys/types.h>
#include  <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#define MAXBUFF 1024
#define FIFO1   "/tmp/fifo.1"
#define FIFO2   "/tmp/fifo.2"
#define PERMS   0666


int main(int argc, char *argv[]){
    int writefd,readfd;
    //Δημιουγούμε τα fifo ένα για γράψιμο στον server και ένα για διάβασμα από τον server
    if ( (mkfifo(FIFO1, PERMS) < 0) &&(errno != EEXIST) ) {
            perror("can't create fifo");
        }
    if ( (mkfifo(FIFO2, PERMS) < 0) &&(errno != EEXIST) ) {
        perror("can't create fifo");
    }
    if(argc > 1){
        //Ελέγχουμε αν ο jobExecutorServer είναι ενεργός,διαφορετικά τον δημιουργούμε 
        int filedes;
        if ((filedes = open("jobExecutorServer.txt", O_RDONLY)) == -1) {
            perror("opening");
            int pid = fork();
            if(pid == 0){
                //Child procces
                char *args_for_jobExecutor[] = {"./jobExecutorServer","&",NULL };
                execvp(args_for_jobExecutor[0],args_for_jobExecutor);
            }
            else{
                sleep(2);
                while((filedes = open("jobExecutorServer.txt", O_RDONLY)) == -1){
                }
                //parent process
                printf("continue with commander\n");
            }
        }
        pid_t pid;
        // Διάβασμα το PID από το αρχείο jobExecutorServer.txt
        if (read(filedes, &pid, sizeof(pid)) == -1) {
            perror("reading");
            exit(1);
        }
        

        // Στέλνουμε το SIGUSR1 σήμα στοv server
        if (kill(pid, SIGUSR1) == -1) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
        //Ανοίγουμε το fifo1 για να γράψουμε στον server
        if ( (writefd= open(FIFO1, O_WRONLY)) < 0){
            perror("client: can't open write fifo \n");
        }
        //Γράφουμε το size του argv[1] στο pipe 
        int size = strlen(argv[1]);
        if(write(writefd,&size,sizeof(int)) == -1){
            perror("αδυναμία εγγραφής του argv[1] στο pipe");
            close(writefd);
            exit(EXIT_FAILURE);
        }
        //Γράφουμε το argv[1] στο pipe 
        if(write(writefd,argv[1],size) == -1){
            perror("αδυναμία εγγραφής του argv[1] στο pipe");
            close(writefd);
            exit(EXIT_FAILURE);
        }
        //Στην περίπτωση που η εντολή είναι setConcurrency 
        if(strcmp(argv[1],"setConcurrency") == 0){
            //Γράφουμε το argv[2] αφού πρώτα το μετατρέψουμε σε Int στο pipe
            int new_concurrency = atoi(argv[2]);
            if(write(writefd,&new_concurrency,sizeof(int)) == -1){
                perror("commander: αδυναμία εγγραφής του argv[2] στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            close(writefd);
            //Κάνουμε unlink τα fifo
            if ( unlink(FIFO1) < 0) {
                perror("client: can't unlink \n");
            }
            if ( unlink(FIFO2) < 0) {
                perror("client: can't unlink \n");
            }
            exit(0);
        }
        //Στην περίπτωση που η εντολή είναι stop
        else if(strcmp(argv[1],"stop") == 0){
            //Γράφουμε το size_of_job_id στο Pipe
            int size_of_job_id = strlen(argv[2]);
            if(write(writefd,&size_of_job_id,sizeof(int)) == -1){
                perror("commander: αδυναμία εγγραφής του size_of_job_id στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            //Γράφουμε το argv[2](job_xx) στο Pipe
            if(write(writefd,argv[2],strlen(argv[2])) == -1){
                perror("commander: αδυναμία εγγραφής του argv[2] στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            close(writefd);
            //Ανοίγουμε το fifo2 για να διαβάσουμε από τον server
            if ((readfd= open(FIFO2, O_RDONLY)) < 0) {
                perror("commander: can't open read fifo");
            }
            //Διαβάζουμε πρώτα το μέγεθους του μηνύματος
            int size_of_message;
            if((read(readfd,&size_of_message,sizeof(int))) == -1){
                perror("commander: αδυναμία ανάγνωσης του size_of_message στο pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Ύστερα δεσμεύουμε χώρο ανάλογα με το μέγεθος του μηνύματος
            char* message = malloc(size_of_message+1);
            //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
            memset(message,0,size_of_message+1);
            //Διαβάζουμε το μήνυμα 
            if((read(readfd, message,size_of_message)) == -1){
                perror("commander: αδυναμία ανάγνωσης του message στο pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            close(readfd);
            printf("%s\n",message);
            free(message);
            //Κάνουμε unlink τα fifo
            if ( unlink(FIFO1) < 0) {
                perror("client: can't unlink \n");
            }
            if ( unlink(FIFO2) < 0) {
                perror("client: can't unlink \n");
            }
            exit(0);
        }
        //Στην περίπτωση που η εντολή είναι poll
        else if(strcmp(argv[1],"poll") == 0){
            //Γράφουμε το size του argv[2] στο pipe
            int size_of_argv_2 = strlen(argv[2]);
            if(write(writefd,&size_of_argv_2,sizeof(int)) == -1){
                perror("commander: αδυναμία εγγραφής του size_ofargv[2] στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            //Γράφουμε το argv[2] στο pipe
            if(write(writefd,argv[2],size_of_argv_2) == -1){
                perror("commander: αδυναμία εγγραφής του argv[2] στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            close(writefd);
            //Ανοίγουμε το Fifo2 για διάβασμα από τον server
            if ((readfd= open(FIFO2, O_RDONLY)) < 0) {
                perror("commander: can't open read fifo");
            }
            //Διαβάζουμε το front και το rear 
            int front,rear;
            if(read(readfd,&front,sizeof(int)) == -1){
                perror("commander: αδυναμία ανάγνωσης του front από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            if(read(readfd,&rear,sizeof(int)) == -1){
                perror("commander: αδυναμία ανάγνωσης του rear από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Διαβάζουμε τον αριθμό των jobs
            int number_of_jobs;
            if(read(readfd,&number_of_jobs,sizeof(int)) == -1){
                perror("commander: αδυναμία ανάγνωσης του number_of_jobs από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Εαν η ουρά είναι άδεια τυπώνουμε αντίστοιχο μήνυμα και κάνουμε unlink τα fifo
            if((rear == -1 && front == -1) || number_of_jobs == 0){
                printf("empty queue\n");
                close(readfd);
                if ( unlink(FIFO1) < 0) {
                    perror("client: can't unlink \n");
                }
                if ( unlink(FIFO2) < 0) {
                    perror("client: can't unlink \n");
                }
            
                exit(0);
            }
            //Εαν η ουρά έχει στοιχεία τυπώνουμε για κάθε στοιχείο την τριπλέτα job_id,job,queue_position
            char* job_id[number_of_jobs];
            for(int i = 0; i < number_of_jobs;i++){
                //Διαβάζουμε το size του job_id
                int size_of_job_id;
                if(read(readfd,&size_of_job_id,sizeof(int)) == -1){
                    perror("commander: αδυναμία ανάγνωσης του size_of_job_id από το pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                //Δεσμεύουμε χώρο ανάλογα με το size που διαβάσαμε
                job_id[i] = malloc(size_of_job_id+1);
                //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
                memset(job_id[i], 0, size_of_job_id+1);
                if(read(readfd,job_id[i],size_of_job_id) == -1){
                    perror("commander: αδυναμία ανάγνωσης του job_id από το pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                printf("job_id: %s\t",job_id[i]);
                //Διαβάζουμε από πόα ορίσματα αποτελείται το job
                int job_args;
                if(read(readfd,&job_args,sizeof(int)) == -1){
                    perror("commander: αδυναμία ανάγνωσης του job_args από το pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                //Για κάθε ένα από τα job_args αρχικά διαβάζουμε το μέγεθος του,δεσμεύουμε μνήμη ανάλογα αυτό,ύστερα διαβάζουμε το job_arg και το τυπώνουμε
                char * job[job_args];
                printf("job:");
                for(int j = 0; j < job_args; j++){
                    //Διάβασμα του size_of_job_elements
                    int size_of_job_elements;
                    if(read(readfd,&size_of_job_elements,sizeof(int)) == -1){
                        perror("commander: αδυναμία ανάγνωσης του size_of_job_elements από το pipe");
                        close(writefd);
                        exit(EXIT_FAILURE);
                    }
                    //Δέσμευση χώρου ανάλογα με το size_of_jobs_elements που διαβάσαμε 
                    job[j] = malloc(size_of_job_elements+1);
                     //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
                    memset(job[j], 0, size_of_job_elements+1);
                    if(read(readfd,job[j],size_of_job_elements) == -1){
                        perror("commander: αδυναμία ανάγνωσης του job[j] από το pipe");
                        close(writefd);
                        exit(EXIT_FAILURE);
                    }
                    printf(" %s",job[j]);
                        
                }
                printf("\t  ");
                //Διαβάζουμε το queue_position
                int queue_position;
                if(read(readfd,&queue_position,sizeof(int)) == - 1){
                    perror("commander: αδυναμία ανάγνωσης του queue_potision στο pipe");
                    close(readfd);
                    exit(EXIT_FAILURE);
                }
                printf("queue_position: %d\n",queue_position);


                
                for(int j = 0; j < job_args; j++){
                    free(job[j]);
                }
                
                
            }
            for(int i = 0; i < number_of_jobs;i++){
                free(job_id[i]);
            }
            close(readfd);
            //Κάνουμε unlink τα fifo
            if ( unlink(FIFO1) < 0) {
                perror("client: can't unlink \n");
            }
            if ( unlink(FIFO2) < 0) {
                perror("client: can't unlink \n");
            }
            
            exit(0);

        }
        //Στην περίπτωση που η εντολή είναι exit
        else if(strcmp(argv[1],"exit") == 0){
            //Ανοίγουμε το fifo2 για διάβασμα από τον server
            if ((readfd= open(FIFO2, O_RDONLY)) < 0) {
                perror("commander: can't open read fifo");
            }
            //Διαβάζουμε το μέγεθος του μηνύματος 
            int size_of_exit_message;
            if((read(readfd,&size_of_exit_message,sizeof(int))) == -1){
                perror("commander: αδυναμία ανάγνωσης του size_of_exit_message στο pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Δεσμέυουμε χώρο ανάλογα με το μέγεθος του μηνύματος 
            char *exit_message = malloc(size_of_exit_message+1);
            //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
            memset(exit_message,0,size_of_exit_message+1);
            //Διαβάζουμε το exit_message
            if((read(readfd,exit_message,size_of_exit_message)) == -1){
                perror("commander: αδυναμία ανάγνωσης του exit_message στο pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            printf("%s\n",exit_message);
            close(readfd);
            free(exit_message);
            //Κάνουμε unlink τα fifo
            if ( unlink(FIFO1) < 0) {
                perror("client: can't unlink \n");
            }
            if ( unlink(FIFO2) < 0) {
                perror("client: can't unlink \n");
            }
            
            exit(0);


        }
        //Στην περίπτωση που η εντολή είναι exit
        else if(strcmp(argv[1],"issueJob") == 0){
            //Γράφουμε το argc στον server
            if(write(writefd,&argc,sizeof(int)) == -1){
                perror("αδυναμία εγγραφής του argc στο pipe");
                close(writefd);
                exit(EXIT_FAILURE);
            }
            //Στο παρακάτω for γράφουμε τα ορίσματα του commander στον server(εκτός του issueJob που το έχουμε κάνει παραπάνω)
            int size_of_argv;
            for(int i = 2; i<argc;i++){
                //Γράψιμο του size of argv[i]
                size_of_argv = strlen(argv[i]);
                if(write(writefd,&size_of_argv,sizeof(int)) == -1){
                    perror("αδυναμία εγγραφής του size_of_argv  στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }
                //Γράψιμο του argv[i]
                if(write(writefd,argv[i],strlen(argv[i])) == -1){
                    perror("commander:αδυναμία εγγραφής του argv[i]  στο pipe");
                    close(writefd);
                    exit(EXIT_FAILURE);
                }

            }
            close(writefd);
            //Ανοίγουμε το fifo2 για διάβασμα από τον server
            if ((readfd= open(FIFO2, O_RDONLY)) < 0) {
                perror("commander: can't open write fifo");
            }
            //Διαβάζουμε το size του job_id
            int size_of_job_id;
            if(read(readfd,&size_of_job_id,sizeof(int)) == -1){
                perror("commander:αδυναμία ανάγνωσης του size_of_job_id  από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            //Δεσμεύουμε ανάλογα με το size του job_id
            char *id = malloc(sizeof(size_of_job_id+1));
            //Αρχικοποιούμε τον χώρο που δεσμεύσαμε ώστε να αποφύγουμε ανεπιθύμητες τιμές στην έξοδο
            memset(id,0,size_of_job_id+1);
            //Διαβάζουμε και τυπώνουμε το Job_id
            if(read(readfd,id,size_of_job_id) == -1){
                perror("commander:αδυναμία ανάγνωσης του job_id  από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
                
            }
            else{
                printf("job_id:%s\n",id);
            }
            //Διαβάζουμε και τυπώνουμε το job στο παρακάτω for
            char *job[argc - 2];
            printf("job: ");
            for(int i = 2;i < argc; i++){
                job[i-2] = malloc(strlen(argv[i])+1);
                memset(job[i-2], 0, strlen(argv[i])+1);
                if((read(readfd,job[i-2],strlen(argv[i]))) == -1) {
                    perror("commander:αδυναμία ανάγνωσης του job[i-2]  από το pipe");
                    close(readfd);
                    exit(EXIT_FAILURE);
                }
                else{
                    printf("%s ",job[i-2]);
                }
            }
            
            printf("\n");
            //Διαβάζουμε το queue_position
            int queue_potision;
            if(read(readfd,&queue_potision,sizeof(int)) == -1){
                perror("commander:αδυναμία ανάγνωσης του queue_potision  από το pipe");
                close(readfd);
                exit(EXIT_FAILURE);
            }
            else{
                printf("queue_potision:%d\n",queue_potision);
            }
            close(readfd);

            //Αποδεσμεύουμε τον χώρο 
            free(id);
            for(int i = 2;i < argc; i++){
                free(job[i-2]);
            }
            //Κάνουμε unlink τα fifo
            if ( unlink(FIFO1) < 0) {
                perror("client: can't unlink \n");
            }
            if ( unlink(FIFO2) < 0) {
                perror("client: can't unlink \n");
            }
            
            
            
            exit(0);
        }
    }
}
