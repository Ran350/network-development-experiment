#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_PROCESS 256 /* プロセス数の上限 */
#define LEN_COMMAND 256 /* コマンドの文字数の上限 */

char *inner_commands[] = {"exit", "quit", "jobs", "fg"};

typedef struct {
    int pid;            /* プロセスID */
    bool is_running;    /* 実行中か否か */
    bool is_background; /* バックグラウンド実行か否か */
} child_t;

typedef struct {
    child_t child_list[NUM_PROCESS]; /* プロセス管理リスト */
    int len_child_list;              /* プロセス管理リストの要素数 */
    int num_running_child;           /* 実行中のプロセスの個数 */
} process_t;

//コマンドライン引数リスト作成
void create_pargs(char *pargs[], int *len_pargs, char command[]) {
    //pargsの初期化
    int i;
    for (i = 0; i < LEN_COMMAND; i++) pargs[i] = NULL;

    //引数に分割
    char *p;
    p = strtok(command, " ");
    for (i = 0; p; i++) {
        pargs[i] = p;
        p = strtok(NULL, " ");
    }

    *len_pargs = i;
}

//プロセス管理リスト中の終了したプロセスを終了済状態にする
void update_exited_process(int pid, process_t *process_manager) {
    printf("終了[%d]\n", pid);

    child_t *child_list = process_manager->child_list;    /* プロセス管理リスト */
    int len_child_list = process_manager->len_child_list; /* プロセス管理リストの要素数 */

    int i;
    for (i = 0; i < len_child_list; i++) {
        if (child_list[i].pid == pid) break;
    }
    child_list[i].is_running = false;

    process_manager->num_running_child -= 1;
}

//jobsコマンド
void jobs(process_t process_manager) {
    printf("PID \n");

    child_t *child_list = process_manager.child_list;    /* プロセス管理リスト */
    int len_child_list = process_manager.len_child_list; /* プロセス管理リストの要素数 */

    int i;
    for (i = 1; i <= len_child_list; i++) {
        if (child_list[i].is_running == true) {
            printf("%d ", i);
            printf("%d ", child_list[i].pid);
            printf("run:%d ", child_list[i].is_running);
            printf("bg:%d ", child_list[i].is_background);
            printf("\n");
        }
    }
}

//fgコマンド
void fg(char pid[], process_t *process_manager) {
    int fg_pid = (int)strtol(pid, NULL, 10);  //char型の数値をint型に変換

    int status;

    child_t *child_list = process_manager->child_list;    /* プロセス管理リスト */
    int len_child_list = process_manager->len_child_list; /* プロセス管理リストの要素数 */

    int i;
    for (i = 0; i <= len_child_list; i++) {
        if (fg_pid == child_list[i].pid) {
            waitpid(fg_pid, &status, 0);
        }
    }
    //プロセス管理リスト中の終了したプロセスを終了済状態にする
    update_exited_process(fg_pid, process_manager);
}

//内部コマンド実行
bool inner_command(char *pargs[], process_t *process_manager) {
    //コマンドが入力されていないとき
    if (pargs[0] == NULL) return true;

    //終了コマンド
    if (strncmp(pargs[0], inner_commands[0], LEN_COMMAND) == 0) exit(-1);
    if (strncmp(pargs[0], inner_commands[1], LEN_COMMAND) == 0) exit(-1);

    //jobsコマンド
    if (strncmp(pargs[0], inner_commands[2], LEN_COMMAND) == 0) {
        jobs(*process_manager);
        return true;
    }

    //fgコマンド
    if (strncmp(pargs[0], inner_commands[3], LEN_COMMAND) == 0) {
        fg(pargs[1], process_manager);
        return true;
    }

    //内部コマンドは実行されなかった
    return false;
}

//バックグラウンド実行するか判定する
bool check_background(char *pargs[], int len_pargs) {
    if (len_pargs <= 0) return 0;  //引数リストが空なら評価しない

    if (strncmp(pargs[len_pargs - 1], "&", LEN_COMMAND) == 0) {
        pargs[len_pargs - 1] = NULL;
        return true;
    }

    return false;
}

//プロセスマネージャに新しいプロセスを書き加える
void write_new_process(int pid, bool is_background, process_t *process_manager) {
    process_manager->len_child_list += 1;
    process_manager->num_running_child += 1;

    child_t *child_list = process_manager->child_list;    /* プロセス管理リスト */
    int len_child_list = process_manager->len_child_list; /* プロセス管理リストの要素数 */

    child_list[len_child_list].pid = pid;
    child_list[len_child_list].is_running = true;
    child_list[len_child_list].is_background = is_background;

    // printf("len_list:%d ", len_child_list);
    // printf("num_run:%d ", process_manager->num_running_child);
    // printf("pid:%d ", process_list[len_child_list].pid);
    // printf("is_run:%d ", process_list[len_child_list].is_running);
    // printf("is_bg:%d \n", process_list[len_child_list].is_background);
}

int main(void) {
    process_t process_manager = {{0, 0, 0}, 0, 0};
    /* 
    ---process_managerの構造---
    process_manager:{
        child_t child_list[]:{
            int pid
            bool is_running
            bool is_background
        }
        int len_child_list
        int num_running_child
    }
    */

    int pid; /* プロセスID */
    int status;

    while (true) {
        //プロンプトを出力
        printf("\n> ");

        // コマンドを入力
        char command[LEN_COMMAND];
        fgets(command, LEN_COMMAND, stdin);
        command[strlen(command) - 1] = '\0';  //\nを削除

        //コマンドライン引数リスト作成
        char *pargs[LEN_COMMAND]; /* コマンドライン引数リスト */
        int len_pargs;            /* コマンドライン引数リストの要素数 */
        create_pargs(pargs, &len_pargs, command);

        //内部コマンド実行
        if (inner_command(pargs, &process_manager) == true) {
            continue;
        }

        //バックグラウンド実行するか判定する
        bool is_background = check_background(pargs, len_pargs);

        //プロセス生成
        pid = fork();

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

            //プロセスマネージャに新しいプロセスを書き加える
            write_new_process(pid, is_background, &process_manager);

            //子プロセスのどれかが終了するまで待機
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                if (WIFEXITED(status) != 0) {
                    //プロセス管理リスト中の終了したプロセスを終了済状態にする
                    update_exited_process(pid, &process_manager);
                }
            }

            //フォアグラウンド実行
            if (is_background == false) {
                pid = waitpid(-1, &status, 0);

                //プロセス管理リスト中の終了したプロセスを終了済状態にする
                update_exited_process(pid, &process_manager);
            }
        }
    }

    return 0;
}
