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
        char cmd[255];
        fgets(cmd, 255, stdin);
        cmd[strlen(cmd) - 1] = '\0';  //\nを削除

        //終了コマンド
        int i;
        for (i = 0; i < 2; i++) {
            if (strcmp(cmd, inner_cmds[i]) == 0) {
                return 0;
            }
        }

        char *pargs[2] = {cmd, NULL};
        int *status;
        int pid = fork();

        if (pid == 0) {
            execv(pargs[0], pargs);
            exit(-1);
        } else if (pid > 0) {
            wait(status);
        } else {
            printf("error");
        }
    }

    return 0;
}
