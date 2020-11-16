#include <dirent.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_PROCESS 256 /* プロセス数の上限 */
#define LEN_COMMAND 256 /* コマンドの文字数の上限 */
#define LEN_ENV 2048    /* 環境変数の文字数の上限 */
#define NUM_ENV_VAR 128 /* 環境変数の個数の上限 */

char *inner_commands[] = {"exit", "q", "jobs", "fg"};

typedef struct {
    pid_t pid;          /* プロセスID */
    bool is_running;    /* 実行中か否か */
    bool is_background; /* バックグラウンド実行か否か */
} child_t;

typedef struct {
    child_t child_list[NUM_PROCESS]; /* プロセス管理リスト */
    int len_child_list;              /* プロセス管理リストの要素数 */
    int num_running_child;           /* 実行中のプロセスの個数 */
} process_t;

process_t process_manager = {{0, 0, 0}, 0, 0}; /* プロセスマネージャ */
pid_t foreground_pid = 0;                      /* プロセスID */

// 文字列をある文字で分割し配列に格納する
int devide_string(char *string, char *string_list[], char deviding_char[1]) {
    // string_listの初期化
    int len = sizeof(*string_list) / sizeof(char);
    for (int j = 0; j < len; j++) string_list[j] = NULL;

    // 引数に分割
    char *p;
    p = strtok(string, deviding_char);
    int i;
    for (i = 0; p != 0; i++) {
        string_list[i] = p;
        p = strtok(NULL, deviding_char);
    }

    return i;  // string_listの要素数を返す
}

// プロセス管理リスト中の終了したプロセスを終了済状態にする
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

// jobsコマンド
void jobs(process_t process_manager) {
    printf("PID \n");

    child_t *child_list = process_manager.child_list;    /* プロセス管理リスト */
    int len_child_list = process_manager.len_child_list; /* プロセス管理リストの要素数 */

    for (int i = 1; i <= len_child_list; i++) {
        //  if (child_list[i].is_running == true) {
        printf("%d ", i);
        printf("%d ", child_list[i].pid);
        printf("run:%d ", child_list[i].is_running);
        printf("bg:%d ", child_list[i].is_background);
        printf("\n");
        //  }
    }
}

// fgコマンド
void fg(char pid[], process_t *process_manager) {
    int fg_pid = (int)strtol(pid, NULL, 10);  // char型の数値をint型に変換

    int status;

    child_t *child_list = process_manager->child_list;    /* プロセス管理リスト */
    int len_child_list = process_manager->len_child_list; /* プロセス管理リストの要素数 */

    for (int i = 0; i <= len_child_list; i++) {
        if (fg_pid == child_list[i].pid) {
            foreground_pid = fg_pid;
            waitpid(fg_pid, &status, 0);
        }
    }
    // プロセス管理リスト中の終了したプロセスを終了済状態にする
    update_exited_process(fg_pid, process_manager);
}

// 内部コマンド実行
bool inner_command(char *pargs[], process_t *process_manager) {
    // コマンドが入力されていないとき
    if (pargs[0] == NULL) return true;

    // 終了コマンド
    if (strncmp(pargs[0], inner_commands[0], LEN_COMMAND) == 0) exit(0);
    if (strncmp(pargs[0], inner_commands[1], LEN_COMMAND) == 0) exit(0);

    // jobsコマンド
    if (strncmp(pargs[0], inner_commands[2], LEN_COMMAND) == 0) {
        jobs(*process_manager);
        return true;
    }

    // fgコマンド
    if (strncmp(pargs[0], inner_commands[3], LEN_COMMAND) == 0) {
        fg(pargs[1], process_manager);
        return true;
    }

    // 内部コマンドは実行されなかった
    return false;
}

// バックグラウンド実行するか判定する
bool check_background(char *pargs[], int len_pargs) {
    if (len_pargs <= 0) return 0;  // 引数リストが空なら評価しない

    if (strncmp(pargs[len_pargs - 1], "&", LEN_COMMAND) == 0) {
        pargs[len_pargs - 1] = NULL;
        return true;
    }

    return false;
}

// プロセスマネージャに新しいプロセスを書き加える
void write_new_process(int pid, bool is_background, process_t *process_manager) {
    process_manager->len_child_list += 1;
    process_manager->num_running_child += 1;

    child_t *child_list = process_manager->child_list;    /* プロセス管理リスト */
    int len_child_list = process_manager->len_child_list; /* プロセス管理リストの要素数 */

    child_list[len_child_list].pid = pid;
    child_list[len_child_list].is_running = true;
    child_list[len_child_list].is_background = is_background;
}

// キーボード割り込み用シグナルハンドラ
void handler_keybord_interrupt(int sig) {
    if (process_manager.num_running_child != 0) {
        kill(foreground_pid, sig);
    }
}

// 子プロセスの実行終了用シグナルハンドラ
void handler_child_exited(int sig) {
    pid_t pid_exited; /* プロセスID */
    int status;

    // バックグラウンド実行では子プロセスの終了を待たない
    while ((pid_exited = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status) != 0) {
            // プロセス管理リスト中の終了したプロセスを終了済状態にする
            update_exited_process(pid_exited, &process_manager);
        }
    }
}

// シグナルハンドラの設定
void set_handler() {
    // キーボード割り込み用シグナル
    signal(SIGINT, handler_keybord_interrupt);

    //  // 子プロセスの実行終了用シグナル
    signal(SIGCHLD, handler_child_exited);
}

// 環境変数を取得する
int get_env_list(char *env_list[]){
    // 環境変数を取得する
    char *all_env = getenv("PATH"); /* すべての環境変数 */
    // printf("%s \n", env);

    // 環境変数を":"ごとに分割
    int len_env_list = devide_string(all_env, env_list, ":");
    for (int i = 0; i < len_env_list; i++) printf("%2d %s \n", i, env_list[i]);
    // printf("%d \n", len_env_list);

    return len_env_list;
}

// フルパスを生成
void create_full_path(char *pargs, char *env_list[]) {
}

int main(void) {
    pid_t pid; /* プロセスID */
    int status;

    // シグナルハンドラの設定
    set_handler();

    // 環境変数の設定
    char *env_list[NUM_ENV_VAR]; /* 環境変数リスト */
    int len_env_list=get_env_list(env_list); /* 環境変数の個数 */


    while (1) {
        // プロンプトを出力
        printf("> ");

        // コマンドを入力
        char command[LEN_COMMAND];
        fgets(command, LEN_COMMAND, stdin);
        command[strlen(command) - 1] = '\0';  // \nを削除

        // コマンドライン引数リスト作成
        char *pargs[LEN_COMMAND]; /* コマンドライン引数リスト */
        int len_pargs;            /* コマンドライン引数リストの要素数 */
        len_pargs = devide_string(command, pargs, " ");

        // バックグラウンド実行するか判定する
        bool is_background = check_background(pargs, len_pargs);

        // 内部コマンド実行
        if (inner_command(pargs, &process_manager) == true) {
            continue;
        }

        // フルパスを生成
        if (strncmp(pargs[0], "/", 1) != 0) {
            bool is_file_found = false;

            // ディレクトリ内探査
            for (int i = 0; i < len_env_list; i++) {
                DIR *dir_stream;

                if ((dir_stream = opendir(env_list[i])) == NULL) {
                    // printf("%d ", i);
                    continue;
                }

                // ファイル内探査
                struct dirent *file;
                int j = 0;
                while ((file = readdir(dir_stream)) != NULL) {
                    // printf("%d %s \n", j++, file->d_name);

                    if (strncmp(file->d_name, pargs[0], LEN_COMMAND) == 0) {
                        // フルパスを生成
                        char full_path[LEN_COMMAND];
                        strncpy(full_path, env_list[i], LEN_COMMAND);
                        strncat(full_path, "/", LEN_COMMAND - 1);
                        strncat(full_path, file->d_name, LEN_COMMAND - 1);
                        // printf("%s\n", full_path);
                        pargs[0] = full_path;
                        // printf("%s\n", pargs[0]);

                        is_file_found = true;
                        break;
                    }
                }

                closedir(dir_stream);
                if (is_file_found == true) break;
            }
        }

        // プロセス生成
        pid = fork();

        // 子プロセス
        if (pid == 0) {
            setpgid(0, 0);  // プロセスグループの設定
            execv(pargs[0], pargs);
            exit(-1);
        }

        // フォーク失敗
        if (pid == -1) {
            printf("fork error");
        }

        // 親プロセス
        if (pid > 0) {
            printf("開始 %s (pid %d) \n", pargs[0], pid);

            // プロセスマネージャに新しいプロセスを書き加える
            write_new_process(pid, is_background, &process_manager);

            // フォアグラウンド実行
            if (is_background == false) {
                foreground_pid = pid;
                pid = waitpid(pid, &status, 0);

                // プロセス管理リスト中の終了したプロセスを終了済状態にする
                update_exited_process(pid, &process_manager);
            }
        }
    }

    return 0;
}
