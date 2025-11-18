#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <string.h> // for strcmp

struct DataItem {
    pid_t PID;  
    int Value;   
};

struct SharedStack {
    int top;
    int stack_size;     
    
    // 題目要求的三個 Semaphore
    sem_t mutex;        
    sem_t empty;        
    sem_t full;         
    
    struct DataItem stack[]; 
};

struct SharedStack *stack_ptr;

void push(int givennum) {
    pid_t my_pid = getpid();

    // 等待有空位
    sem_wait(&stack_ptr->empty);
    
    // 取得互斥鎖
    sem_wait(&stack_ptr->mutex);
    
    // --- Critical Section ---
    int idx = stack_ptr->top;
    stack_ptr->stack[idx].PID = my_pid;
    stack_ptr->stack[idx].Value = givennum;
    stack_ptr->top++;
    // ------------------------
    
    // 釋放互斥鎖
    sem_post(&stack_ptr->mutex);
    
    // 通知 Consumer 有資料了
    sem_post(&stack_ptr->full);
}

struct DataItem pop() {
    struct DataItem item;
    
    // 1. 等待有資料
    sem_wait(&stack_ptr->full);
    
    // 2. 取得互斥鎖
    sem_wait(&stack_ptr->mutex);
    
    // --- Critical Section ---
    stack_ptr->top--;
    item = stack_ptr->stack[stack_ptr->top];
    // ------------------------
    
    // 3. 釋放互斥鎖
    sem_post(&stack_ptr->mutex);
    
    // 4. 通知 Producer 有空位了
    sem_post(&stack_ptr->empty);
    
    return item;
}

void producer(int cnt, int num) {
    int pid = getpid();
    srand(time(NULL) ^ getpid());
    printf("Producer P%d (PID %d) starts writing...\n", cnt + 1, pid);
    
    int i = 1;
    while (i <= num) {
        int random_num = (rand() % 10) + 1;
        
        int sem_stored;
        sem_getvalue(&stack_ptr->empty, &sem_stored);

        push(random_num);
        
        printf("P%d writes (%d, %d), increments Sem %d.\n", cnt + 1, pid, random_num, sem_stored - 1);
        
        i++;
    }
    exit(0); 
}

void consumer(int limit) {
    int i = 0;
    int sum = 0;
    int pid = getpid();
    int sem_stored;
    
    // 取得目前的 semaphore 值
    sem_getvalue(&stack_ptr->full, &sem_stored);
    printf("Consumer (PID %d) waits on Semaphore %d\n", pid, sem_stored);

    while (i < limit) {
        // 等待並取出資料
        struct DataItem item = pop();
        
        sum += item.Value;
        i++;
        
        printf("# Consumer reads (%d, %d). Sum = %d.\n", item.PID, item.Value, sum);
    }
    
    printf("Total Data Points Processed: %d\n", limit);
    printf("Final Cumulative Sum: %d\n", sum);
    
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3 || (strcmp(argv[1], "help") == 0)){
        fprintf(stderr, "Usage: %s M(producers) N(writes each)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_prod = atoi(argv[1]);
    int num_write = atoi(argv[2]);
    int total_items = num_prod * num_write;

    // 計算所需的共享記憶體大小
    size_t shm_size = sizeof(struct SharedStack) + sizeof(struct DataItem) * num_prod;
    
    // mmap 配置共享記憶體
    stack_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (stack_ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // 初始化變數
    stack_ptr->top = 0;
    stack_ptr->stack_size = num_prod; 

    // 初始化 Semaphores
    // resource_pool (你的命名) -> 對應到 empty (空位)
    if (sem_init(&stack_ptr->mutex, 1, 1) != 0 ||
        sem_init(&stack_ptr->empty, 1, num_prod) != 0 ||
        sem_init(&stack_ptr->full, 1, 0) != 0) {
        perror("Semaphore 初始化失敗");
        exit(EXIT_FAILURE);
    }

    // 3. 建立 Producer Processes
    for (int i = 0; i < num_prod; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            producer(i, num_write); // 進入子行程邏輯
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }

    // 4. 建立 Consumer Process
    pid_t cons_pid = fork();
    if (cons_pid == 0) {
        consumer(total_items); // 進入消費者邏輯
    } else if (cons_pid < 0) {
        perror("fork consumer failed");
        exit(EXIT_FAILURE);
    }

    // 5. 等待所有子行程結束 (M 個 Producer + 1 個 Consumer)
    for (int i = 0; i < num_prod + 1; i++) {
        wait(NULL);
    }

    // 6. 清理資源
    sem_destroy(&stack_ptr->mutex);
    sem_destroy(&stack_ptr->empty);
    sem_destroy(&stack_ptr->full);
    munmap(stack_ptr, shm_size);

    return 0;
}