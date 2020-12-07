#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 4000

int main() {
    char destination[32] = "127.0.1.1";

    /* 接続先指定用構造体の準備 */
    struct sockaddr_in dstAddr;
    bzero((char *)&dstAddr, sizeof(dstAddr));
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(PORT);

    /* ホスト名からアドレスを取得 */
    struct hostent *hp;
    hp = gethostbyname(destination);
    bcopy(hp->h_addr, &dstAddr.sin_addr, hp->h_length);

    /* ソケットの作成 */
    int dstSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (dstSocket < 0) {
        perror("sock");
        printf("%d\n", errno);
        exit(-1);
    }

    /* サーバに接続 */
    if (connect(dstSocket, (struct sockaddr *)&dstAddr, sizeof(dstAddr)) < 0) {
        printf("%s に接続できませんでした。\n", destination);
        exit(EXIT_FAILURE);
    }
    printf("%s に接続しました。\n", destination);

    while (1) {
        char buf[1024];

        printf("自分 ");
        scanf("%s", buf);

        /* パケットの送信 */
        int bytes_wrote = write(dstSocket, buf, sizeof(buf));
        if (bytes_wrote < 1) {
            perror("write");
            printf("%d\n", errno);
            exit(EXIT_FAILURE);
        }
        if (strncmp(buf, "quit", sizeof(buf)) == 0) break;

        /* パケットの受信 */
        int bytes_read = read(dstSocket, buf, sizeof(buf));
        if (bytes_read < 0) {
            perror("read");
            printf("%d\n", errno);
            exit(EXIT_FAILURE);
        }
        printf("相手 %s\n", buf);
        if (strncmp(buf, "quit", sizeof(buf)) == 0) break;
    }

    close(dstSocket);

    return 0;
}
