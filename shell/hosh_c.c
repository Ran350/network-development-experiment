#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *inner_commands[] = {"exit", "quit"};

int main() {
    while (1) {
        //プロンプトを出力
        printf("> ");

        // コマンドを入力
        char command[256];
        fgets(command, 256, stdin);
        command[strlen(command) - 1] = '\0';  //\nを削除

        //終了コマンド
        int i;
        for (i = 0; i < 2; i++) {
            if (strncmp(command, inner_commands[i], 256) == 0) {
                return 0;
            }
        }

        //コマンドライン引数リスト作成
        char *pargs[256];
        for (i = 0; i < 256; i++) pargs[i] = NULL;  //pargsの初期化

        char *p;
        p = strtok(command, " ");
        for (i = 0; p; i++) {
            pargs[i] = p;
            p = strtok(NULL, " ");
        }

        //バックグラウンド実行するかの判定
        int is_background = 0;
        if (i > 0) {
            if (strncmp(pargs[i - 1], "&", 256) == 0) {
                pargs[i - 1] = NULL;
                is_background++;
            }
        }

        int pid = fork();
        int status;

        if (pid == 0) {
            //子プロセス
            execv(pargs[0], pargs);
            exit(-1);
        }

        if (pid == -1) {
            //フォーク失敗
            printf("fork error");
        }

        if (pid > 0) {
            //親プロセス

            //バックグラウンド実行終了確認
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                if (WIFEXITED(status) != 0) {
                    printf("終了[%d]\n", pid);
                }
            }

            if (is_background == 0) {
                //フォアグラウンド実行
                pid = waitpid(-1, &status, 0);
                printf("終了[%d]\n", pid);
            }
        }
    }

    return 0;
}
