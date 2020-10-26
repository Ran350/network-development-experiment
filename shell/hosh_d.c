#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *inner_commands[] = {"exit", "quit", "jobs", "fg"};

typedef struct {
    int pid;
    bool is_running;     //実行中 -> true
    bool is_background;  //バックグラウンド実行 -> true
} child_t;

//コマンドライン引数リスト作成
void create_pargs(char *pargs[], int *len_pargs, char command[]) {
    int i;
    for (i = 0; i < 256; i++) pargs[i] = NULL;  //pargsの初期化

    char *p;
    p = strtok(command, " ");
    for (i = 0; p; i++) {
        pargs[i] = p;
        p = strtok(NULL, " ");
    }
    *len_pargs = i;
}

//プロセス管理リスト中の終了したプロセスを終了済状態にする
void update_exited_process(child_t child_list[256], int pid, int *num_running_child, int len_child_list) {
    printf("終了[%d]\n", pid);

    int i;

    for (i = 0; i < len_child_list; i++) {
        if (child_list[i].pid == pid) break;
    }
    child_list[i].is_running = false;
    *num_running_child -= 1;
}

//jobsコマンド
void jobs(child_t child_list[], int len_child_list) {
    int i;

    printf("PID \n");
    for (i = 1; i <= len_child_list; i++) {
        // if (child_list[i].is_running == true) {
        printf("%d ", i);
        printf("%d ", child_list[i].pid);
        printf("run:%d ", child_list[i].is_running);
        printf("bg:%d ", child_list[i].is_background);
        printf("\n");
        // }
    }
}

//fgコマンド
void fg(child_t child_list[], int len_child_list, char pid[], int *num_running_child) {
    int fg_pid = (int)strtol(pid, NULL, 10);  //char型の数値をint型に変換

    int status;
    int i;

    for (i = 0; i <= len_child_list; i++) {
        if (fg_pid == child_list[i].pid) {
            waitpid(fg_pid, &status, 0);
        }
    }
    //プロセス管理リスト中の終了したプロセスを終了済状態にする
    update_exited_process(child_list, fg_pid, num_running_child, len_child_list);
}

//内部コマンド実行
bool inner_command(char *pargs[], child_t *child_list, int len_child_list, int *num_running_child) {
    //コマンドが入力されていないとき
    if (pargs[0] == NULL) return true;

    //終了コマンド
    if (strncmp(pargs[0], inner_commands[0], 256) == 0) exit(-1);
    if (strncmp(pargs[0], inner_commands[1], 256) == 0) exit(-1);

    //jobsコマンド
    if (strncmp(pargs[0], inner_commands[2], 256) == 0) {
        jobs(child_list, len_child_list);
        return true;
    }

    //fgコマンド
    if (strncmp(pargs[0], inner_commands[3], 256) == 0) {
        fg(child_list, len_child_list, pargs[1], num_running_child);
        return true;
    }

    //内部コマンドは実行されなかった
    return false;
}

//バックグラウンド実行するか判定する
bool judge_background(char *pargs[], int len_pargs) {
    if (len_pargs > 0) {
        if (strncmp(pargs[len_pargs - 1], "&", 256) == 0) {
            pargs[len_pargs - 1] = NULL;
            return true;
        }
    }
    return false;
}

//プロセス管理リストに新しいプロセスを追記
void add_child_process(child_t child_list[256], int *len_child_list, int *num_running_child, int pid, bool is_background) {
    *len_child_list += 1;
    *num_running_child += 1;
    // printf("tail:%d ", *len_child_list);
    // printf("nrun:%d ", *num_running_child);
    child_list[*len_child_list].pid = pid;
    child_list[*len_child_list].is_running = true;
    child_list[*len_child_list].is_background = is_background;
    // printf("pid:%d ", child_list[*len_child_list].pid);
    // printf("isrun:%d ", child_list[*len_child_list].is_running);
    // printf("isbg:%d \n", child_list[*len_child_list].is_background);
}

int main() {
    child_t child_list[256];    //プロセス管理リスト
    int len_child_list = 0;     //プロセス管理リストの要素数
    int num_running_child = 0;  //実行中のプロセスの個数

    int pid;  //プロセスID
    int status;

    while (true) {
        //プロンプトを出力
        printf("\n> ");

        // コマンドを入力
        char command[256];
        fgets(command, 256, stdin);
        command[strlen(command) - 1] = '\0';  //\nを削除

        char *pargs[256];                          //コマンドライン引数リスト
        int len_pargs;                             //コマンドライン引数リストの要素数
        create_pargs(pargs, &len_pargs, command);  //コマンドライン引数リスト作成

        //内部コマンド実行
        if (inner_command(pargs, child_list, len_child_list, &num_running_child) == true) {
            continue;
        }

        //バックグラウンド実行するか判定する
        bool is_background = judge_background(pargs, len_pargs);

        pid = fork();  //プロセス生成

        //子プロセス
        if (pid == 0) {
            execv(pargs[0], pargs);
            exit(-1);
        }

        //フォーク失敗
        if (pid == -1) {
            printf("fork error");
        }

        //親プロセス
        if (pid > 0) {
            printf("開始 %s (pid %d) \n", pargs[0], pid);

            //プロセス管理リストに新しいプロセスを追記
            add_child_process(child_list, &len_child_list, &num_running_child, pid, is_background);

            //子プロセスのどれかが終了するまで待機
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                if (WIFEXITED(status) != 0) {
                    //プロセス管理リスト中の終了したプロセスを終了済状態にする
                    update_exited_process(child_list, pid, &num_running_child, len_child_list);
                }
            }

            //フォアグラウンド実行
            if (is_background == false) {
                pid = waitpid(-1, &status, 0);

                //プロセス管理リスト中の終了したプロセスを終了済状態にする
                update_exited_process(child_list, pid, &num_running_child, len_child_list);
            }
        }
    }

    return 0;
}
