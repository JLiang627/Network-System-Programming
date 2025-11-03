#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 5
#define INCREMENTS 1000000

int global_counter = 0;
pthread_mutex_t counter_lock;

/* 執行緒要執行的函式 */
void *increment_counter(void *arg) {
    for (int i = 0; i < INCREMENTS; i++) {
        
        /* 3. 鎖定 */
        pthread_mutex_lock(&counter_lock);
        
        /* === 臨界區間 === */
        global_counter++;
        /* === 臨界區間結束 === */
        
        /* 4. 解鎖 */
        pthread_mutex_unlock(&counter_lock);
    }
    return NULL;
}

int main() {
    pthread_t tid[NUM_THREADS];

    /* 2. 初始化互斥鎖 */
    if (pthread_mutex_init(&counter_lock, NULL) != 0) {  //也可以在一開始宣告INITIALIZER
        printf("Mutex 初始化失敗\n");                     //但是要刪掉init跟destroy
        return 1;
    }

    /* 建立執行緒 */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&tid[i], NULL, increment_counter, NULL);
    }

    /* 5. 等待所有執行緒完成 */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    /* 6. 銷毀互斥鎖 */
    pthread_mutex_destroy(&counter_lock);

    /*
     * 如果沒有用 Mutex，這裡的結果會小於 5,000,000，
     * 因為 global_counter++ (讀取、加一、寫回) 
     * 不是原子操作，會導致更新遺失。
     */
    printf("最終計數器結果: %d\n", global_counter);

    return 0;
}