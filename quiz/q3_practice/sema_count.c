#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <stdlib.h> // 為了 exit()

#define MAX_RESOURCES 3
#define NUM_THREADS 10 // 總共有 10 個執行緒想使用

sem_t resource_pool; // 計數信號量

void *use_resource(void *arg) {
    int id = *(int *)arg;
    
    printf("Thread %d: 正在等待資源...\n", id);
    
    // 1. 請求資源 (sem_wait)
    // 前 3 個執行緒會立刻通過，第 4 個開始會在這裡等待
    sem_wait(&resource_pool); 

    /* === 臨界區間 (已取得資源) === */
    printf("Thread %d: *** 取得資源，開始工作 ***\n", id);
    sleep(2); // 模擬使用資源
    printf("Thread %d: --- 工作完成，釋放資源 ---\n", id);
    /* === 臨界區間結束 === */

    // 2. 釋放資源 (sem_post)
    sem_post(&resource_pool); 
    
    return NULL;
}

int main() {
    pthread_t tid[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // 3. 初始化信號量，初始值為 3 (代表有 3 個可用資源)
    if (sem_init(&resource_pool, 0, MAX_RESOURCES) != 0) {
        perror("Semaphore 初始化失敗");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&tid[i], NULL, use_resource, &thread_ids[i]) != 0) {
            perror("Thread 建立失敗");
            exit(EXIT_FAILURE);
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    sem_destroy(&resource_pool);
    return 0;
}