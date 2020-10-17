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
        for (i = 0; p; i++) {
            pargs[i] = p;
            p = strtok(NULL, " ");
        }

        //バックグラウンド実行するかの判定
        int is_bg = 0;
        if (i > 0) {
            if (strncmp(pargs[i - 1], "&", 256) == 0) {
                pargs[i - 1] = NULL;
                is_bg++;
            }
        }

        int pid = fork();
        int status;

        if (pid == 0) {
            //子プロセス
            execv(pargs[0], pargs);
            exit(-1);

        } else if (pid == -1) {
            //フォーク失敗
            printf("fork error");

        } else {
            //親プロセス

            if (is_bg == 1) {
                //バックグラウンド実行する
                while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                    if (WIFEXITED(status) != 0) {
                        printf("終了[% d]\n", pid);
                    }
                }

            } else if (is_bg == 0) {
                //バックグラウンド実行しない
                wait(&status);
            }
        }
    }

    return 0;
}
