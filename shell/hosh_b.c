#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *inner_cmds[] = {"exit", "quit"};

int main() {
    while (1) {
        //プロンプトを出力
        printf("> ");

        // コマンドを入力
        char cmd[256];
        fgets(cmd, 256, stdin);
        cmd[strlen(cmd) - 1] = '\0';  //\nを削除

        //終了コマンド
        int i;
        for (i = 0; i < 2; i++) {
            if (strncmp(cmd, inner_cmds[i], 256) == 0) {
                return 0;
            }
        }

        //コマンドライン引数リスト作成
        char *pargs[256];
        for (i = 0; i < 256; i++) pargs[i] = NULL;  //pargsの初期化
        char *p;

        p = strtok(cmd, " ");
        // printf("p:%s\n", p);
        for (i = 0; p; i++) {
            pargs[i] = p;
            p = strtok(NULL, " ");
            // printf("i:%d\n", i);
            // printf("pargs[%d]:%s\n", i, pargs[i]);
            // printf("p:%s\n", p);
        }
        // printf("pargs[%d]:%s\n", i, pargs[i]);

        int status;
        int pid = fork();  //子プロセス生成

        if (pid == 0) {
            execv(pargs[0], pargs);
            exit(-1);
        } else if (pid == -1) {
            printf("fork error");
        } else {
            wait(&status);
        }
    }

    return 0;
}
