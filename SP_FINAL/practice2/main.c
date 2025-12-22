#include "stack.h"

void push(long pid, char ch, struct shmbuf *shmp) {
    // 1. P(empty): 等待有空位。如果滿了 (sem_empty=0)，會卡在這裡睡眠
    if (sem_wait(&shmp->sem_empty) == -1) errorHandler("sem_wait empty");

    // 2. P(mutex): 取得互斥鎖，準備修改記憶體
    if (sem_wait(&shmp->mutex) == -1) errorHandler("sem_wait mutex");

    // --- Critical Section Start ---
    // 這裡不需要再檢查 if(top == -1)，因為 sem_empty 保證了一定有位子
    int top = shmp->top;
    shmp->stack[top] = ch;
    shmp->top--; 
    printf("[Process %ld] Pushed '%c' (Remaining slots: val of empty)\n", pid, ch);
    // --- Critical Section End ---

    // 3. V(mutex): 釋放互斥鎖
    if (sem_post(&shmp->mutex) == -1) errorHandler("sem_post mutex");

    // 4. V(full): 增加「現有資料」計數，若有消費者在等資料，會喚醒它
    if (sem_post(&shmp->sem_full) == -1) errorHandler("sem_post full");
}

char pop(long pid, struct shmbuf *shmp) {
    // 1. P(full): 等待有資料。如果空了 (sem_full=0)，會卡在這裡睡眠
    if (sem_wait(&shmp->sem_full) == -1) errorHandler("sem_wait full");

    // 2. P(mutex): 取得互斥鎖
    if (sem_wait(&shmp->mutex) == -1) errorHandler("sem_wait mutex");

    // --- Critical Section Start ---
    // 這裡不需要再檢查 if(top == size-1)，因為 sem_full 保證了一定有資料
    char ch;
    int top = shmp->top;
    ch = shmp->stack[top + 1];
    shmp->top++;
    printf("[Process %ld] Popped '%c'\n", pid, ch);
    // --- Critical Section End ---

    // 3. V(mutex): 釋放互斥鎖
    if (sem_post(&shmp->mutex) == -1) errorHandler("sem_post mutex");

    // 4. V(empty): 增加「剩餘空位」計數，若有生產者在等空位，會喚醒它
    if (sem_post(&shmp->sem_empty) == -1) errorHandler("sem_post empty");

    return ch;
}

void print_stack(struct shmbuf *shmp) {
    // 為了 Debug 方便，不加鎖直接讀取 (真實環境建議加鎖)
    printf("--- Stack Content ---\n");
    if (shmp->top == STACK_SIZE - 1) {
        printf("(Empty)\n");
    } else {
        for (int i = shmp->top + 1; i < STACK_SIZE; i++) {
            printf("[%d]: %c\n", i, shmp->stack[i]);
        }
    }
    printf("---------------------\n");
}

int main(int argc, char *argv[]) {
    int fd;
    struct shmbuf *shmp;

    // 清除舊的共享記憶體
    shm_unlink(SHM_PATH);

    fd = shm_open(SHM_PATH, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) errorHandler("shm_open");

    if (ftruncate(fd, sizeof(struct shmbuf)) == -1)
        errorHandler("ftruncate");

    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED)
        errorHandler("mmap");

    // --- 初始化變數 ---
    shmp->top = STACK_SIZE - 1; // 堆疊初始為空
    
    if (sem_init(&shmp->mutex, 1, 1) == -1) errorHandler("sem_init mutex");

    // 2. sem_empty: 空位數量，初始為 STACK_SIZE (3)
    if (sem_init(&shmp->sem_empty, 1, STACK_SIZE) == -1) errorHandler("sem_init empty");

    // 3. sem_full: 資料數量，初始為 0
    if (sem_init(&shmp->sem_full, 1, 0) == -1) errorHandler("sem_init full");


    printf("Parent: Spawning processes. Stack Size = %d\n", STACK_SIZE);

    pid_t pid = fork();
    if (pid == -1) errorHandler("fork 1");

    if (pid == 0) { // --- Child 1 (瘋狂生產者) ---
        long pid = (long)getpid();
        // 嘗試推入 4 個元素 (Stack 只有 3 格)
        // 第 4 個 'D' 應該會導致阻塞，直到 Child 2 取走元素
        push(pid, 'A', shmp); print_stack(shmp);
        push(pid, 'B', shmp); print_stack(shmp);
        push(pid, 'C', shmp); print_stack(shmp);
        
        printf("[Process %ld] Trying to push 'D' (Should BLOCK here if full)...\n", pid);
        push(pid, 'D', shmp); // <--- 會卡在這裡
        printf("[Process %ld] Successfully pushed 'D' (I was unblocked!)\n", pid);
        print_stack(shmp);

        exit(EXIT_SUCCESS);
    }

    pid = fork();
    if (pid == -1) errorHandler("fork 2");

    if (pid == 0) { // --- Child 2 (慢速消費者) ---
        long pid = (long)getpid();
        
        printf("[Process %ld] Sleeping 2s before popping...\n", pid);
        sleep(2); // 讓 Child 1 先把堆疊塞滿並卡住

        pop(pid, shmp); // 這會讓 Child 1 的 'D' 能夠寫入
        print_stack(shmp);

        sleep(1);
        pop(pid, shmp);
        print_stack(shmp);

        exit(EXIT_SUCCESS);
    }

    // 父行程等待大家結束
    wait(NULL);
    wait(NULL);

    // 清理資源
    sem_destroy(&shmp->mutex);
    sem_destroy(&shmp->sem_empty);
    sem_destroy(&shmp->sem_full);
    munmap(shmp, sizeof(*shmp));
    shm_unlink(SHM_PATH);
    
    printf("All processes finished.\n");
    exit(EXIT_SUCCESS);
}