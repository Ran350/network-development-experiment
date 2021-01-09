#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define PATH_HTDOCS "/home/hoshi/ドキュメント/ritsumei/ネットワーク開発実験/network-development-experiment/web_server/htdocs"
#define RESPONSE_NOTFOUND "HTTP/1.0 404 Not Found\r\n\r\n"

int search_open_file(char *path_name, int file_d);
int path_is_dir(char *path_name, int file_d);
int path_is_file(char *path_name, int file_d);

void path_create(char *buf, char *path_name);

void http_response(int sock);
void send_http_response(int sock, int file_d);
void space_divide(char *http_status[], char *buf);

int main(void) {
    int sock, sock0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t sock_len;
    int err;

    // socket making
    if ((sock0 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // socket setting

    // localhostをIPアドレスにする
    // struct in_addr addr;
    // struct addrinfo hints, *res;
    // char buf_hostname[32];
    // char *hostname = "localhost";
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_family = AF_INET;
    // if ((err = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
    //   printf("error %d\n", err);
    //   return 1;
    // }
    // addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    // inet_ntop(AF_INET, &addr, buf_hostname, sizeof(buf_hostname));

    // server address setting
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind
    if (bind(sock0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(sock0, 5)) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    sock_len = sizeof(client_addr);

    // freeaddrinfo(res);

    while (1) {
        puts("--------------------------------------------------------------");

        // accept http request
        if ((sock = accept(sock0, (struct sockaddr *)&client_addr, &sock_len)) ==
            -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        puts("Connected");

        http_response(sock);

        // finish connection
        close(sock);

        puts("--------------------------------------------------------------");
    }
    /* listen するsocketの終了 */
    if (close(sock0) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
    return 1;
}

void http_response(int sock) {
    puts("************************HTTP REREQUEST************************");
    // リクエストのread
    sleep(1);
    char http_request[2048];
    char buf[9196] = "abc";
    while (1) {
        memset(http_request, 0, 2048);
        int n;
        n = read(sock, http_request, 2048);
        strncat(buf, http_request, 2048);
        printf("%s", http_request);
        if (n < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (n < 2048) {
            break;
        }
    }
    puts("**************************************************************");

    // レスポンスの動作
    puts("Server side work");

    // PATH
    char path_name[256] = PATH_HTDOCS;
    path_create(buf, path_name);
    // パスの一番後ろは / なしになっているはず
    printf("request pathname: %s\n", path_name);

    // htmlを抽出
    int file_d;

    file_d = search_open_file(path_name, file_d);

    // どのファイルを開いたか確認する
    printf("open pathname: %s\n", path_name);

    // http response send
    send_http_response(sock, file_d);

    char response[2048];
    while (1) {
        memset(response, 0, 2048);
        int n = read(file_d, response, 2048);
        int err = write(sock, response, n);
        printf("%s", response);
        if (err < 1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        if (n < 2048) {
            break;
        }
    }
    puts("send response");
}

void send_http_response(int sock, int file_d) {
    if (file_d < 1) {
        // openできなかった時
        write(sock, RESPONSE_NOTFOUND, 128);
        printf("HTTP/1.0 404 Not Found\r\n\r\n");
    } else {
        // openできた時
        char status_msg[] = "HTTP/1.0 200 OK\r\n";
        write(sock, status_msg, sizeof(status_msg));
        write(sock, "\r\n", 2);
        printf("HTTP/1.0 200 OK\r\n\r\n");
    }
}

int search_open_file(char *path_name, int file_d) {
    // ファイルかディレクトリかを確認する
    struct stat status_path;
    if (stat(path_name, &status_path) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    // ディレクトリの時
    if (S_ISDIR(status_path.st_mode)) {
        if ((file_d = path_is_dir(path_name, file_d)) < 0) {
            exit(EXIT_FAILURE);
        }
    }
    // ファイルの時
    if (S_ISREG(status_path.st_mode)) {
        if ((file_d = path_is_file(path_name, file_d)) < 0) {
            exit(EXIT_FAILURE);
        }
    }
    return file_d;
}

int path_is_dir(char *path_name, int file_d) {
    // index.html setting
    char index_html[16] = "/index.html";
    strncat(path_name, index_html, 16);

    // file open
    file_d = open(path_name, O_RDONLY);
    if (file_d < 1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    puts("Success dir and file open");
    return file_d;
}

int path_is_file(char *path_name, int file_d) {
    file_d = open(path_name, O_RDONLY);
    if (file_d < 1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    puts("Success file open");
    return file_d;
}

void path_create(char *buf, char *path_name) {
    // パスを分析
    sscanf(buf, "%[^\n]", buf);

    char *http_status[5];
    space_divide(http_status, buf);
    char content_dir[128];
    strncpy(content_dir, http_status[1], sizeof(content_dir));

    // パス生成
    strncat(path_name, content_dir, sizeof(content_dir));

    int path_last = strlen(path_name);
    if (path_name[path_last - 1] == '/') {
        path_name[path_last - 1] = '\0';
    }
    puts("path create");
}

void space_divide(char *http_status[], char *buf) {
    int i;

    for (i = 0; *buf != '\0'; i++) {
        while (*buf == ' ') {
            *buf = '\0';
            buf++;
        }

        if (*buf == '\0') {
            break;
        }
        http_status[i] = buf;
        while (*buf != '\0' && *buf != ' ') {
            buf++;
        }
    }

    http_status[i] = 0;
}
