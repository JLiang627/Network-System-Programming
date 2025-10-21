#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    // getcwd
#include <limits.h>    // PATH_MAX
#include <dirent.h>

int main(int argc, char *argv[]) {
    DIR *dir;
    char cwd[PATH_MAX];

    // 取得並印出目前工作目錄
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd error");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
