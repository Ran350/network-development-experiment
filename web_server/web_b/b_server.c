#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080

int main() {
    /* ソケットの生成 */
    int srcSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (srcSocket < 0) {
        perror("socket");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }

    /* 自分のソケットの設定 */
    struct sockaddr_in srcAddr;
    bzero((char *)&srcAddr, sizeof(srcAddr));  // srcAddrの初期化
    srcAddr.sin_port = htons(PORT);
    srcAddr.sin_family = AF_INET;
    srcAddr.sin_addr.s_addr = INADDR_ANY;

    /* 通信相手のソケット */
    struct sockaddr_in dstAddr;
    int dstAddrSize = sizeof(dstAddr);

    /* ソケットのバインド */
    if (bind(srcSocket, (struct sockaddr *)&srcAddr, sizeof(srcAddr)) < 0) {
        perror("bind");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }

    /* TCPクライアントからの接続要求を待てる状態にする */
    if (listen(srcSocket, 1) < 0) {
        perror("listen");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }

    /* TCPクライアントからの接続要求を受け付ける */
    printf("接続を待っています。\n");
    int dstSocket = accept(srcSocket, (struct sockaddr *)&dstAddr, &dstAddrSize);
    if (dstSocket < 0) {
        perror("accept");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }
    close(srcSocket);

    /* パケットの受信 */
    char buf[1024];
    memset(buf, 0, sizeof(buf));  // bufの初期化

    int bytes_read = read(dstSocket, buf, sizeof(buf));
    if (bytes_read < 0) {
        perror("read");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("HTTP request: %s \n", buf);

    // 16進数出力
    int i = 0;
    while (buf[i]) printf("%x ", buf[i++]);

    close(srcSocket);

    return 0;
}
