// 沒寫完也還沒更正，不能正常執行。

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <stdlib.h> // 為了 exit()
#include <time.h>
#include <errno.h>
#include <string.h>

sem_t resource_pool;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int stack_size;

struct SharedStack{
    pid_t PID;
    int Value;
    int top;
    int stack[256];
};

struct SharedStack stack;

void push(int givennum) {
    pthread_mutex_lock(&mutex);
    while (stack.top >= stack_size){
        sleep(1);
    }
    stack.stack[stack.top] = givennum;
    stack.top++;
    
    pthread_mutex_unlock(&mutex);
}


char pop() {
    while (stack.top == 0) {
        
    }
    pthread_mutex_lock(&mutex);

    int reval = stack.stack[stack.top];
    stack.top--;    
    pthread_mutex_unlock(&mutex);
    return reval;
}

void *producer(int cnt, int num){
    int i = 1;
    stack.top = 0;
    int pid = getpid();
    printf("Producer P%d (PID %d) starts writing...\n", cnt+1, pid);
    srand(time(NULL));
    while (i <= num){
        sem_wait(&resource_pool);
        int random_num = (rand() % 10) + 1;
        i++;
        int sem_stored;
        printf("P%d writes (%d, %d), increments Sem %d.\n", cnt, pid, random_num, sem_getvalue(&resource_pool, sem_stored));
        push(random_num);
        sem_post(&resource_pool); 
    }


    pthread_exit(NULL);
}

void *consumer(int limit){
    int i = 0;
    int sum = 0;
    int pid = getpid();
    int sem_stored;
    printf("Consumer (PID %d) waits on Semaphore %d\n", pid, sem_getvalue(&resource_pool, sem_stored));

    while (i < limit){
        while (sem_trywait(&resource_pool) != 0) {
            // 2. 檢查錯誤
            if (errno == EAGAIN) {
                // EAGAIN 代表信號量為 0 (資源不可用)
                sleep(1);
            } else {
                // 發生其他錯誤
                perror("sem_trywait error");
                break;
            }
        }
        i++;
        sum+=pop();
    }
    printf("Total Data Points Processed: %d\n", limit);
    printf("Final Cumulative Sum: %d\n", sum);

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 3 || (strcmp(argv[1], "help") == 0)){
        fprintf(stderr, "Usage: %s M(producers) N(writes each)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_prod = atoi(argv[1]);
    int num_write = atoi(argv[2]);
    pthread_t prod[num_prod];
    pthread_t cons;
    stack_size = num_prod*num_write;
    
    // 初始化sema
    if (sem_init(&resource_pool, 0, 2) != 0) {
        perror("Semaphore 初始化失敗");
        exit(EXIT_FAILURE);
    }

    // create producer
    for (int i = 0; i < num_prod; i++) {
        pthread_create(&prod[i], NULL, producer(i, num_write), NULL);
    }

    // create consumer
    if (pthread_create(&cons, NULL, consumer(num_prod*num_write), NULL) != 0) {
        perror("Consumer thread 建立失敗");
        exit(EXIT_FAILURE);
    }

    // join pthread
    for (int i = 0; i < num_prod; i++) {
        pthread_join(prod[i], NULL);
    }
    pthread_join(cons, NULL);

    return 0;
}