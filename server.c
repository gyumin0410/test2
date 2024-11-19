#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 20

void errorHandling(char *msg);

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr, clnt_addr;
    struct timeval timeout;
    int serv_sock, clnt_sock;
    fd_set fds, cpy_fds;
    
    int clnt_socks[MAX_CLNT];  // 클라이언트 소켓 목록
    int clnt_cnt = 0;           // 클라이언트 수 카운트

    char buf[BUF_SIZE];
    int fd_max, str_len, fd_num;
    socklen_t addr_size;

    // 잘못된 인자 입력 시 종료
    if(argc != 2) {
        printf("Wrong input. Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        errorHandling("socket creation failed");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    // 바인딩
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        errorHandling("binding error occurred");
    }

    // 리스닝
    if (listen(serv_sock, 5) == -1) {
        errorHandling("listening error occurred");
    }

    // IO 멀티플렉싱 준비
    FD_ZERO(&fds);
    FD_SET(serv_sock, &fds);
    fd_max = serv_sock;

    while(1) {
        cpy_fds = fds;
        timeout.tv_sec = 5;
        timeout.tv_usec = 5000;

        // select 호출 (클라이언트 연결을 기다림)
        if((fd_num = select(fd_max + 1, &cpy_fds, 0, 0, &timeout)) == -1){
            break;
        }

        // 타임아웃 시 계속 실행
        if(fd_num == 0) {
            continue;
        }

        // 각 파일 디스크립터 체크
        for(int i = 0; i < fd_max + 1; i++) {
            if (!FD_ISSET(i, &cpy_fds)) {
                continue;
            }

            // 클라이언트의 연결 요청
            if(serv_sock == i) {
                addr_size = sizeof(clnt_addr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &addr_size);
                if (clnt_sock == -1) {
                    errorHandling("client connection failed");
                }

                // 클라이언트 소켓 배열에 추가
                clnt_socks[clnt_cnt++] = clnt_sock;
                FD_SET(clnt_sock, &fds);
                if(fd_max < clnt_sock)
                    fd_max = clnt_sock;
                printf("connected client: %d\n", clnt_sock);
            }
            // 클라이언트 메시지 읽기
            else {
                str_len = read(i, buf, BUF_SIZE);
                if(str_len == 0) {
                    FD_CLR(i, &fds);
                    close(i);
                    printf("closed client: %d\n", i);
                }
                else {
                    // 받은 메시지를 모든 클라이언트에게 전송
                    for(int j = 0; j < clnt_cnt; j++) {
                        write(clnt_socks[j], buf, str_len);
                    }
                }
            }
        }
    }
    close(serv_sock);
    return 0;
}

// 에러 처리 함수
void errorHandling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
