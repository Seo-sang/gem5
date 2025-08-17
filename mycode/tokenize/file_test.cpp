#include <iostream>
#include <stdlib.h>

int main() {
    int pq_time = 20;
    int hash_time = 23;
    FILE *fp = fopen("/gem5/mycode/tokenize/file_test.txt", "a+");
    if (fp == NULL) {
        perror("파일 오픈 실패");
        exit(EXIT_FAILURE);
    }

    // 파일의 끝에 데이터를 추가합니다.
    fprintf(fp, "%d %d\n", pq_time, hash_time);

    // 파일을 닫습니다.
    fclose(fp);
}
