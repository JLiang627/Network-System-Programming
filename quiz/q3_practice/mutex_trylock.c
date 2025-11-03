#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define STACK_SIZE 10

pthread_mutex_t stack_lock = PTHREAD_MUTEX_INITIALIZER;
int stack[STACK_SIZE];
int count = 0;

/*
 * 嘗試 push，如果失敗 (鎖正忙) 則返回 0，
 * 如果成功則返回 1。
 */
int try_push_data(int n) {
    if (pthread_mutex_trylock(&stack_lock) == 0) {
        if (count < STACK_SIZE) {
            stack[count] = n;
            count++;
            printf("Thread %ld: 成功 Push 資料 %d, Stack size: %d\n", pthread_self(), n, count);
        } else {
            printf("Thread %ld: Stack 滿了\n", pthread_self());
        }
        pthread_mutex_unlock(&stack_lock);
        return 1;
    } else {
        printf("Thread %ld: 堆疊正忙，稍後再試...\n", pthread_self());
        return 0;
    }
}

/* 只讀 Stack 顯示內容 (含鎖保護) */
void print_stack() {
    pthread_mutex_lock(&stack_lock);
    printf("Stack 狀態：");
    for (int i = 0; i < count; i++) {
        printf("%d ", stack[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&stack_lock);
}

/* 執行緒函式：持續嘗試 push */
void* push_worker(void* arg) {
    int val = *(int*)arg;
    for (int i = 0; i < 6; i++) {
        try_push_data(val + i);
        usleep(100000); // 0.1 秒，模擬其他操作
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[2];
    int start_vals[2] = {1, 100};

    // 建立 2 個執行緒
    for (int i = 0; i < 2; i++) {
        pthread_create(&threads[i], NULL, push_worker, &start_vals[i]);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    print_stack();

    // 清理資源
    pthread_mutex_destroy(&stack_lock);

    return 0;
}
