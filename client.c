#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

void *sendMsg(void *arg);
void *recvMsg(void *arg);
void errorHandling(char *msg);

int main(int argc, char *argv[]) {
    pthread_t snd_t, rcv_t;
    struct sockaddr_in serv_addr;
    void *return_t;
    int sock;

    // 입력 오류 처리
    if (argc != 4) {
        printf("잘못된 입력. 사용법 : %s <IP> <PORT> <NAME>\n", argv[0]);
        exit(1);
    }

    // 사용자 이름 설정
    sprintf(name, "[%s]", argv[3]);

    // 소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        errorHandling("소켓 생성 실패");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        errorHandling("서버 연결 실패");
    }

    // 쓰레드 생성
    pthread_create(&snd_t, NULL, sendMsg, (void *)&sock);
    pthread_create(&rcv_t, NULL, recvMsg, (void *)&sock);
    pthread_join(snd_t, &return_t);
    pthread_join(rcv_t, &return_t);

    close(sock);
    return 0;
}

// 메시지 전송 쓰레드
void *sendMsg(void *arg) {
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];

    while (1) {
        // 사용자로부터 메시지 입력 받기
        if (fgets(msg, BUF_SIZE, stdin) == NULL) {
            continue;  // 입력 오류시 다시 시도
        }

        // 종료 명령어 입력 시 소켓 닫고 종료
        if (strcmp(msg, "q\n") == 0 || strcmp(msg, "Q\n") == 0) {
            close(sock);
            exit(0);
        }

        // 사용자 이름과 메시지를 합쳐서 전송할 메시지 준비
        snprintf(name_msg, sizeof(name_msg), "%s %s", name, msg);

        // 서버로 메시지 전송
        if (send(sock, name_msg, strlen(name_msg), 0) == -1) {
            perror("메시지 전송 실패");
            close(sock);
            exit(1);
        }
    }
    return NULL;
}

// 메시지 수신 쓰레드
void *recvMsg(void *arg) {
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    int str_len;

    while (1) {
        // 서버로부터 메시지 수신
        str_len = recv(sock, name_msg, sizeof(name_msg) - 1, 0);
        if (str_len == -1) {
            perror("메시지 수신 실패");
            close(sock);
            exit(1);
        }

        // 서버에서 수신한 메시지 출력
        if (str_len == 0) {
            break;  // 연결 종료된 경우
        }

        name_msg[str_len] = '\0';  // null-terminate
        fputs(name_msg, stdout);
    }
    return NULL;
}

// 오류 처리 함수
void errorHandling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
