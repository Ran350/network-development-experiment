#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_ROOT "/home/hoshi/ドキュメント/ritsumei/ネットワーク開発実験/network-development-experiment/web_server/htdocs"
// #define PORT_NUM 8080

void extract_file_path(char file_path[], char http_request[], int len_file_path);

void create_full_path(char full_path[], char file_path[], int len_file_path);

bool check_file_exist(char file_path[]);

void send_message(int socket, char message[]);

void send_response_status(int socket, bool file_exist, char file_path[]);

void send_file(int socket, char file_path[]);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: file %s", argv[0]);
        return 1;
    }

    // ソケットの生成
    int srcSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (srcSocket < 0) {
        perror("socket");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }

    int port_num = atoi(argv[1]);

    // 自分のソケットの設定
    struct sockaddr_in srcAddr;
    bzero((char *)&srcAddr, sizeof(srcAddr));  // srcAddrの初期化
    srcAddr.sin_port = htons(port_num);
    srcAddr.sin_family = AF_INET;
    srcAddr.sin_addr.s_addr = INADDR_ANY;

    // 通信相手のソケット
    struct sockaddr_in dstAddr;
    int dstAddrSize = sizeof(dstAddr);

    // ソケットのバインド
    if (bind(srcSocket, (struct sockaddr *)&srcAddr, sizeof(srcAddr)) < 0) {
        perror("bind");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }

    // TCPクライアントからの接続要求を待てる状態にする
    if (listen(srcSocket, 1) < 0) {
        perror("listen");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // TCPクライアントからの接続要求を受け付ける
        int dstSocket = accept(srcSocket, (struct sockaddr *)&dstAddr, &dstAddrSize);
        if (dstSocket < 0) {
            perror("accept");
            printf("%d\n", errno);
            exit(EXIT_FAILURE);
        }
        // close(srcSocket);

        puts("---------------------------------------------");
        printf("接続を待っています。\n\n");

        // HTTPリクエストの受信
        char http_request[1024];
        memset(http_request, 0, sizeof(http_request));  // http_requestの初期化
        int bytes_read = read(dstSocket, http_request, sizeof(http_request));
        if (bytes_read < 0) {
            perror("read");
            printf("%d\n", errno);
            exit(EXIT_FAILURE);
        }
        printf("HTTP request \n%s\n", http_request);

        // リクエスト文からファイルパス部を抽出
        char file_path[128];
        memset(file_path, 0, sizeof(file_path));
        extract_file_path(file_path, http_request, sizeof(file_path));

        // フルパスを生成
        char file_full_path[128];
        memset(file_full_path, 0, sizeof(file_full_path));
        create_full_path(file_full_path, file_path, sizeof(file_path));

        // ファイルが存在するか確認する
        bool file_exist = check_file_exist(file_full_path);

        // レスポンスステータスを送信
        send_response_status(dstSocket, file_exist, file_full_path);

        // ファイル送信
        if (file_exist == true) send_file(dstSocket, file_full_path);

        close(dstSocket);
    }

    return 0;
}

void extract_file_path(char file_path[], char http_request[], int len_file_path) {
    // file_path[]: 抽出したファイルパスを格納する初期化済配列
    //              出力時，最後の文字は"/"でない

    // HTTPリクエスト全文から，リクエストラインだけを抽出
    char request_line[len_file_path + 16];  // リクエストラインの長さ以上の要素数ならOK
    memset(request_line, 0, sizeof(request_line));
    sscanf(http_request, "%[^\n]", request_line);  // "\n"までを抽出
    printf("request_line: %s\n", request_line);

    // ファイルパスを抽出
    strtok(request_line, " ");       // メソッド（例：GET）
    char *path = strtok(NULL, " ");  // パス（例：/img/logo.gif）
    strncpy(file_path, path, len_file_path);

    // TODO: ファイルパスの最後の文字が"/"であれば削除

    printf("file_path: %s\n", file_path);
}

void create_full_path(char full_path[], char path[], int len_path) {
    // full_path[]: 生成したフルパスを格納する初期済の配列
    // path[]:      "/"から始まるファイルパス　（例："/img/logo.gif"）

    strncpy(full_path, SERVER_ROOT, sizeof(SERVER_ROOT));  // ルートパスをコピー
    strncat(full_path, path, len_path);                    // ファイルパスを付加
    printf("full_path(1): %s\n", full_path);               // 例："/home/~/htdocs/img/logo.gif"

    struct stat file_status;
    stat(full_path, &file_status);

    // ファイルパスがファイルのとき
    if (S_ISREG(file_status.st_mode)) puts("this is file");

    // ファイルパスがディレクトリなら，htmlファイルを付加してフルパスに
    if (S_ISDIR(file_status.st_mode)) {
        char html_path[] = "index.html";
        strncat(full_path, html_path, sizeof(html_path));
        puts("this is directory");
    }

    printf("full_path(2): %s \n", full_path);
}

bool check_file_exist(char file_path[]) {
    if (open(file_path, O_RDONLY) > 0) {
        return true;
    }
    return false;
}

void send_message(int socket, char message[]) {
    if (write(socket, message, strlen(message)) < 1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

void send_response_status(int socket, bool file_exist, char file_path[]) {
    if (file_exist == true) {
        send_message(socket, "HTTP/1.1 200 OK\r\n");
        send_message(socket, "\r\n");
        puts("the flie exists");
    }

    if (file_exist == false) {
        send_message(socket, "HTTP/1.0 404 Not Found\r\n");
        puts("the flie  non't exists");
    }
}

void send_file(int socket, char file_path[]) {
    int file_descriptor = open(file_path, O_RDONLY);

    /* ソケットへ書き込む */
    while (1) {
        char buf[4048];

        /* ファイルを読み込む */
        int bytes_read = read(file_descriptor, buf, sizeof(buf));
        if (bytes_read <= 0) break;

        /* ソケットへ書き込む */
        int bytes_wrote = write(socket, buf, bytes_read);
        if (bytes_wrote < 1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
}