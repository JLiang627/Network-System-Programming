#include <stdio.h>
#include <time.h>

int main() {
    time_t now;
    struct tm *tm_now;
    char date_str[64];

    // 取得目前時間
    time(&now);
    tm_now = localtime(&now);

    strftime(date_str, sizeof(date_str), "%b %e(%a), %Y %I:%M %p", tm_now);
    printf("%s\n", date_str);
    return 0;
}
