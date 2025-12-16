#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>


static pid_t *pid_record;
static int max_fds;

FILE *mypopen(const char *command, const char *type) {
	// 參數宣告區
	int fd[2];
	pid_t child_pid;

    // 初始化 record
    if (pid_record == NULL) {
        max_fds = sysconf(_SC_OPEN_MAX); // 取得系統 fd 上限
        pid_record = calloc(max_fds, sizeof(pid_t));
        
        if (pid_record == NULL) {
            errno = ENOMEM; // 記憶體不足
            return NULL; 
        }
    }

    // --- 步驟 2：檢查 type 參數 ---
    if (strcmp(type, "w") != 0 && strcmp(type, "r") != 0) {
	    errno = EINVAL;
	    return NULL;
	}

    // --- 步驟 3：建立水管 ---
    if (pipe(fd) == -1){
    	perror("pipe failed"); 
    	return NULL;
    }

    int pread_flag=1;  //預設parent會是read
    if (strcmp(type, "r") != 0) pread_flag = 0;  //如果不是的話再設false

    // 步驟 4：fork()
    child_pid = fork();
    switch (child_pid){
    case -1:
    	perror("fork failed"); 
    	close(fd[0]);
    	close(fd[1]);
    	return NULL;
    	break;
    case 0: // --- 子行程 ---
    	if (pread_flag){
    		close(fd[0]);
			if (dup2(fd[1], STDOUT_FILENO) == -1) {
                perror("child dup2 failed");
                _exit(127);
            }	    	
            close(fd[1]);
    	}else{
			close(fd[1]); 
            if (dup2(fd[0], STDIN_FILENO) == -1) {
                perror("child dup2 failed");
                _exit(127);
            }
            close(fd[0]);
    	}
    	
        // 關閉所有繼承的 fd
    	for (int i = 0; i < max_fds; i++) {
			if (pid_record[i] > 0) {
		     	close(i);
			}
		}

		if ((execl("/bin/sh", "sh", "-c", command, (char *) NULL)) == -1){
			perror("execl failed");
			_exit(127);
		}
    	break;
    default: // --- 父行程 ---
        if (pread_flag) {
            close(fd[1]); 
            pid_record[fd[0]] = child_pid; 
            
            FILE *stream = fdopen(fd[0], "r");
            if (stream == NULL) {
                close(fd[0]);
                waitpid(child_pid, NULL, 0);
            }
            return stream;

        } else {
            close(fd[0]); 
            pid_record[fd[1]] = child_pid;

            FILE *stream = fdopen(fd[1], "w");
            if (stream == NULL) {
                close(fd[1]);
                waitpid(child_pid, NULL, 0);
           }
            return stream;
        }
    }
    return NULL; 
}

int mypclose(FILE *stream) {
    int fd;
    pid_t child_pid;
    int status;
    pid_t wait_ret;

    // 步驟 1：檢查 fd 並取得 pid
    fd = fileno(stream);
    if (fd < 0 || fd >= max_fds || pid_record[fd] <= 0) {
        errno = EBADF; 
        return -1;     
    }
    child_pid = pid_record[fd];

    // 步驟 2：關閉串流 (這會 flush 並 close fd)
    if (fclose(stream) == EOF) {
    }

    // 步驟 3：等待子行程 (處理 EINTR)
    do {
        wait_ret = waitpid(child_pid, &status, 0); 
    } while (wait_ret == -1 && errno == EINTR);

    // 步驟 4：檢查 waitpid 錯誤
    if (wait_ret == -1) {
        errno = ECHILD;
        return -1;
    }

    // 步驟 5：清理 record
    pid_record[fd] = 0;

    // 步驟 6：回傳子行程狀態
    return status;
}


// --- 主要測試函式 ---

int main() {
    FILE *pipe_stream;
    char buffer[1024];
    int status;

    // === Test 1: "r" mode (Reading from command) ===
    printf("--- 測試 mypopen(\"ls -l\", \"r\") ---\n");
    pipe_stream = mypopen("ls -l", "r");
    if (pipe_stream == NULL) {
        perror("mypopen (read) failed");
        return 1;
    }

    // 從 pipe 讀取所有輸出
    while (fgets(buffer, sizeof(buffer), pipe_stream) != NULL) {
        printf("  [ls output]: %s", buffer); // 印出從指令讀到的內容
    }

    // 關閉 pipe 並取得狀態
    status = mypclose(pipe_stream);
    printf("--- mypopen(\"ls -l\") 結束 ---\n");
    if (WIFEXITED(status)) {
        printf("子行程 (ls -l) 正常結束, 狀態碼: %d\n", WEXITSTATUS(status));
    } else {
        printf("子行程 (ls -l) 異常結束\n");
    }
    printf("\n");


    // === Test 2: "w" mode (Writing to command) ===
    printf("--- 測試 mypopen(\"grep test\", \"w\") ---\n");
    printf("  (子行程 grep 會把結果直接印在下面)\n");
    pipe_stream = mypopen("grep test", "w"); // "grep test" 會過濾包含 "test" 的行
    if (pipe_stream == NULL) {
        perror("mypopen (write) failed");
        return 1;
    }

    // 寫入幾行資料到 "grep" 的 STDIN
    fprintf(pipe_stream, "This line has the word test.\n");
    fprintf(pipe_stream, "This line does not.\n");
    fprintf(pipe_stream, "Here is another test line.\n");
    fprintf(pipe_stream, "Final line, no keyword.\n");
    printf("  [main]: 寫入了 4 行資料到 'grep test'...\n");
    
    // 關閉 pipe。這會 flush 緩衝區, 並送 EOF 給 "grep".
    // "grep" 收到 EOF 後就會結束執行，並把它過濾到的結果印出。
    status = mypclose(pipe_stream);
    printf("--- mypopen(\"grep test\") 結束 ---\n");
    
    // 檢查 grep 的結束狀態
    if (WIFEXITED(status)) {
        printf("子行程 (grep test) 正常結束, 狀態碼: %d\n", WEXITSTATUS(status));
        printf("  (狀態 0 = 找到匹配, 1 = 沒找到, >1 = 錯誤)\n");
    } else {
        printf("子行程 (grep test) 異常結束\n");
    }

    return 0;
}