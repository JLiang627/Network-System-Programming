#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h> // 這裡改用 pthread 的鎖
#include <time.h>
#include <string.h>
#include <fcntl.h>

// 資料結構
struct DataItem {
    pid_t PID;
    int Value;
};

struct SharedStack {
    int top;
    int stack_size;     // 你的命名
    
    // 這裡改用 Mutex + Condition Variables
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_full;  // 用來等待 "不滿" (有空位)
    pthread_cond_t cond_not_empty; // 用來等待 "不空" (有資料)
    
    struct DataItem stack[]; // 彈性陣列
};

// 全域指標
struct SharedStack *stack_ptr;

void push(int givennum) {
    pid_t my_pid = getpid();

    pthread_mutex_lock(&stack_ptr->mutex);
    
    // 檢查是否已滿
    while (stack_ptr->top >= stack_ptr->stack_size) {
        // 如果滿了，就睡眠等待 "cond_not_full" 信號
        pthread_cond_wait(&stack_ptr->cond_not_full, &stack_ptr->mutex);
    }

    // --- Critical Section ---
    int idx = stack_ptr->top;
    stack_ptr->stack[idx].PID = my_pid;
    stack_ptr->stack[idx].Value = givennum;
    stack_ptr->top++;
    // ------------------------
    
    // 寫入完畢，通知 Consumer 現在 "不空" 了
    pthread_cond_signal(&stack_ptr->cond_not_empty);

    pthread_mutex_unlock(&stack_ptr->mutex);
}

struct DataItem pop() {
    struct DataItem item;
    
    // 1. 取得互斥鎖
    pthread_mutex_lock(&stack_ptr->mutex);
    
    // 2. 檢查是否為空
    while (stack_ptr->top <= 0) {
        // 如果空了，就睡眠等待 "cond_not_empty" 信號
        pthread_cond_wait(&stack_ptr->cond_not_empty, &stack_ptr->mutex);
    }
    
    // --- Critical Section ---
    stack_ptr->top--;
    item = stack_ptr->stack[stack_ptr->top];
    // ------------------------
    
    // 3. 取出完畢，通知 Producer 現在 "不滿" 了 (有空位)
    pthread_cond_signal(&stack_ptr->cond_not_full);
    
    // 4. 釋放互斥鎖
    pthread_mutex_unlock(&stack_ptr->mutex);
    
    return item;
}

void producer(int cnt, int num) {
    int pid = getpid();
    srand(time(NULL) ^ getpid()); 

    printf("Producer P%d (PID %d) starts writing...\n", cnt + 1, pid);
    
    int i = 1;
    while (i <= num) {
        int random_num = (rand() % 10) + 1;
        
        push(random_num);
        
        printf("P%d writes (%d, %d). Stack Depth: %d.\n", cnt + 1, pid, random_num, stack_ptr->top);
        
        i++;
    }
    exit(0);
}

void consumer(int limit) {
    int i = 0;
    int sum = 0;
    int pid = getpid();
    
    printf("Consumer (PID %d) starts reading...\n", pid);

    while (i < limit) {
        struct DataItem item = pop();
        
        sum += item.Value;
        i++;
        
        printf("Consumer reads (%d, %d). Sum = %d.\n", item.PID, item.Value, sum);
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

    size_t shm_size = sizeof(struct SharedStack) + sizeof(struct DataItem) * num_prod;
    
    stack_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (stack_ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    stack_ptr->top = 0;
    stack_ptr->stack_size = num_prod;

    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    
    // 初始化屬性物件
    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);
    
    // 設定屬性為 PTHREAD_PROCESS_SHARED (允許跨 Process 使用)
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    
    // 使用設定好的屬性來初始化 Shared Memory 中的鎖
    pthread_mutex_init(&stack_ptr->mutex, &mattr);
    pthread_cond_init(&stack_ptr->cond_not_full, &cattr);
    pthread_cond_init(&stack_ptr->cond_not_empty, &cattr);
    
    // 銷毀屬性物件
    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);
    // ==========================================

    // 建立 Producer Processes
    for (int i = 0; i < num_prod; i++) {
        if (fork() == 0) {
            producer(i, num_write);
        }
    }

    // 建立 Consumer Process
    if (fork() == 0) {
        consumer(total_items);
    }

    // 等待所有子行程
    for (int i = 0; i < num_prod + 1; i++) {
        wait(NULL);
    }

    // 清理
    pthread_mutex_destroy(&stack_ptr->mutex);
    pthread_cond_destroy(&stack_ptr->cond_not_full);
    pthread_cond_destroy(&stack_ptr->cond_not_empty);
    munmap(stack_ptr, shm_size);

    return 0;
}