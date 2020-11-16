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
#define NUM_ENV_VAR 256 /* 環境変数の個数の上限 */

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
int devide_sentence(char *sentence, char *word_list[], char deviding_char[1]) {
    // string_listの初期化
    int size = sizeof(*word_list) / sizeof(char);
    for (int j = 0; j < size; j++) word_list[j] = NULL;

    // 引数に分割
    char *word; /* 分割済み文字列 */
    int len;    /* word_listの要素数 */
    word = strtok(sentence, deviding_char);
    for (len = 0; word != 0; len++) {
        word_list[len] = word;
        word = strtok(NULL, deviding_char);
    }

    return len;
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

// jobsコマンドを実行
void run_jobs(process_t process_manager) {
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

// fgコマンドを実行
void run_fg(char char_pid[], process_t *process_manager) {
    int pid = (int)strtol(char_pid, NULL, 10);  // char型の数値をint型に変換

    int status;

    child_t *child_list = process_manager->child_list;    /* プロセス管理リスト */
    int len_child_list = process_manager->len_child_list; /* プロセス管理リストの要素数 */

    for (int i = 0; i <= len_child_list; i++) {
        if (pid == child_list[i].pid) {
            foreground_pid = pid;
            waitpid(pid, &status, 0);
        }
    }
    // プロセス管理リスト中の終了したプロセスを終了済状態にする
    update_exited_process(pid, process_manager);
}

// 内部コマンド実行
bool inner_command(char *command_lines[], process_t *process_manager) {
    // コマンドが入力されていないとき
    if (command_lines[0] == NULL) return true;

    // 終了コマンド
    if (strncmp(command_lines[0], inner_commands[0], LEN_COMMAND) == 0) exit(0);
    if (strncmp(command_lines[0], inner_commands[1], LEN_COMMAND) == 0) exit(0);

    // jobsコマンド
    if (strncmp(command_lines[0], inner_commands[2], LEN_COMMAND) == 0) {
        run_jobs(*process_manager);
        return true;
    }

    // fgコマンド
    if (strncmp(command_lines[0], inner_commands[3], LEN_COMMAND) == 0) {
        run_fg(command_lines[1], process_manager);
        return true;
    }

    // 内部コマンドは実行されなかった
    return false;
}

// バックグラウンド実行するかを判定
bool check_background(char *command_lines[], int len_command_lines) {
    if (len_command_lines <= 0) return 0;  // 引数リストが空なら評価しない

    if (strncmp(command_lines[len_command_lines - 1], "&", LEN_COMMAND) == 0) {
        command_lines[len_command_lines - 1] = NULL;
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
    pid_t pid_exited; /* 終了したプロセスのID */
    int status;

    // バックグラウンド実行では子プロセスの終了を待たない
    while ((pid_exited = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status) != 0) {
            // プロセス管理リスト中の終了したプロセスを終了済状態にする
            update_exited_process(pid_exited, &process_manager);
        }
    }
}

// シグナルハンドラを設定
void set_handler() {
    // キーボード割り込み用シグナル
    signal(SIGINT, handler_keybord_interrupt);

    // 子プロセスの実行終了用シグナル
    signal(SIGCHLD, handler_child_exited);
}

// 環境変数を取得
int get_env_list(char *env_list[]) {
    // 環境変数一覧を取得
    char *all_env = getenv("PATH"); /* すべての環境変数 */

    // 環境変数を":"ごとに分割
    int len_env_list; /* 環境変数リストの要素数 */
    len_env_list = devide_sentence(all_env, env_list, ":");

    for (int i = 0; i < len_env_list; i++) printf("%2d %s \n", i, env_list[i]);
    // printf("%d \n", len_env_list);

    return len_env_list;
}

// フルパスを生成
void create_full_path(char *command_lines[], char *env_list[], int len_env_list) {
    // ディレクトリ検索
    for (int i = 0; i < len_env_list; i++) {
        DIR *dir_stream;

        if ((dir_stream = opendir(env_list[i])) == NULL) {
            // printf("%d ", i);
            continue;
        }

        // ファイル検索
        struct dirent *file;
        while ((file = readdir(dir_stream)) != NULL) {
            // コマンド名と名前が一致するファイルを検索
            if (strncmp(file->d_name, command_lines[0], LEN_COMMAND) == 0) {
                // フルパスを生成
                char full_path[LEN_COMMAND];
                strncpy(full_path, env_list[i], LEN_COMMAND);
                strncat(full_path, "/", LEN_COMMAND - 1);
                strncat(full_path, file->d_name, LEN_COMMAND - 1);
                command_lines[0] = full_path;

                closedir(dir_stream);
                return;
            }
        }
        closedir(dir_stream);
    }
}

int main(void) {
    pid_t pid; /* プロセスID */
    int status;

    // シグナルハンドラを設定
    set_handler();

    // 環境変数を取得
    char *env_list[NUM_ENV_VAR]; /* 環境変数リスト */
    int len_env_list;            /* 環境変数リストの要素数 */
    len_env_list = get_env_list(env_list);

    while (1) {
        // プロンプトを出力
        printf(" \n> ");

        // コマンドを入力
        char command[LEN_COMMAND];
        fgets(command, LEN_COMMAND, stdin);
        command[strlen(command) - 1] = '\0';  // \nを削除

        // コマンドライン引数リストを作成
        char *command_lines[LEN_COMMAND]; /* コマンドライン引数リスト */
        int len_command_lines;            /* コマンドライン引数リストの要素数 */
        len_command_lines = devide_sentence(command, command_lines, " ");

        // バックグラウンド実行するかを判定
        bool is_background; /* バックグラウンド実行するか否か */
        is_background = check_background(command_lines, len_command_lines);

        // 内部コマンドを実行
        if (inner_command(command_lines, &process_manager) == true) {
            continue;
        }

        // フルパスを生成
        if (strncmp(command_lines[0], "/", 1) != 0) {
            create_full_path(command_lines, env_list, len_env_list);
        }

        // プロセスを生成
        pid = fork();

        // 子プロセス
        if (pid == 0) {
            setpgid(0, 0);                           // プロセスグループを設定
            execv(command_lines[0], command_lines);  // コマンドを実行
            exit(-1);
        }

        // フォーク失敗
        if (pid == -1) {
            printf("fork error");
        }

        // 親プロセス
        if (pid > 0) {
            printf("開始 %s (pid %d) \n", command_lines[0], pid);

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
