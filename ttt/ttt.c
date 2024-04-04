#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ttt.h"
#include <ctype.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "../list.h"
#include "mcts.h"
#include "negamax.h"

// Bitwise AND with 0x1f to clear the 5th and 6th bits
#define CTRL_KEY(k) ((k) &0x1f)

struct task {
    jmp_buf env;
    struct list_head list;
    char task_name[10];
};

struct arg {
    char *task_name;
};

static LIST_HEAD(tasklist);
static void (**tasks)(void *);
static struct arg *args;
static int ntasks;
static jmp_buf sched;
static struct task *cur_task;
char task_table[N_GRIDS];
int move_record[N_GRIDS];
int move_count = 0;
struct termios orig_termios;
int rawmode = 0;
int won = 0;
int game_stop = 0;

static void record_move(int move)
{
    move_record[move_count++] = move;
}

static void print_moves()
{
    printf("Moves: ");
    for (int i = 0; i < move_count; i++) {
        printf("%c%d", 'A' + GET_COL(move_record[i]),
               1 + GET_ROW(move_record[i]));
        if (i < move_count - 1) {
            printf(" -> ");
        }
    }
    printf("\n");
}

static int get_input(char player)
{
    char *line = NULL;
    size_t line_length = 0;
    int parseX = 1;

    int x = -1, y = -1;
    while (x < 0 || x > (BOARD_SIZE - 1) || y < 0 || y > (BOARD_SIZE - 1)) {
        printf("%c> ", player);
        int r = getline(&line, &line_length, stdin);
        if (r == -1)
            exit(1);
        if (r < 2)
            continue;
        x = 0;
        y = 0;
        parseX = 1;
        for (int i = 0; i < (r - 1); i++) {
            if (isalpha(line[i]) && parseX) {
                x = x * 26 + (tolower(line[i]) - 'a' + 1);
                if (x > BOARD_SIZE) {
                    // could be any value in [BOARD_SIZE + 1, INT_MAX]
                    x = BOARD_SIZE + 1;
                    printf("Invalid operation: index exceeds board size\n");
                    break;
                }
                continue;
            }
            // input does not have leading alphabets
            if (x == 0) {
                printf("Invalid operation: No leading alphabet\n");
                y = 0;
                break;
            }
            parseX = 0;
            if (isdigit(line[i])) {
                y = y * 10 + line[i] - '0';
                if (y > BOARD_SIZE) {
                    // could be any value in [BOARD_SIZE + 1, INT_MAX]
                    y = BOARD_SIZE + 1;
                    printf("Invalid operation: index exceeds board size\n");
                    break;
                }
                continue;
            }
            // any other character is invalid
            // any non-digit char during digit parsing is invalid
            // TODO: Error message could be better by separating these two cases
            printf("Invalid operation\n");
            x = y = 0;
            break;
        }
        x -= 1;
        y -= 1;
    }
    free(line);
    return GET_INDEX(y, x);
}

int ttt()
{
    srand(time(NULL));
    char table[N_GRIDS];
    memset(table, ' ', N_GRIDS);
    char turn = 'X';
    char ai = 'O';

    memset(move_record, 0, N_GRIDS * sizeof(int));
    move_count = 0;

    while (1) {
        char win = check_win(table);
        if (win == 'D') {
            draw_board(table);
            printf("It is a draw!\n");
            break;
        } else if (win != ' ') {
            draw_board(table);
            printf("%c won!\n", win);
            break;
        }

        if (turn == ai) {
            int move = mcts(table, ai);
            if (move != -1) {
                table[move] = ai;
                record_move(move);
            }
        } else {
            draw_board(table);
            int move;
            while (1) {
                move = get_input(turn);
                if (table[move] == ' ') {
                    break;
                }
                printf("Invalid operation: the position has been marked\n");
            }
            table[move] = turn;
            record_move(move);
        }
        turn = turn == 'X' ? 'O' : 'X';
    }
    print_moves();

    return 0;
}

static void task_add(struct task *task)
{
    list_add_tail(&task->list, &tasklist);
}

static int task_len(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        len++;
    return len;
}

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    rawmode = 0;
}

void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    rawmode = 1;
}

static void task_switch()
{
    if (!list_empty(&tasklist)) {
        struct task *t = list_first_entry(&tasklist, struct task, list);
        list_del(&t->list);
        cur_task = t;
        if (task_len(&tasklist) > 0) {
            longjmp(t->env, 1);
        }
    }
}

void schedule(void)
{
    static int i;

    setjmp(sched);

    while (ntasks-- > 0) {
        struct arg arg = args[i];
        tasks[i++](&arg);
        printf("Never reached\n");
    }

    task_switch();
    if (rawmode == 1)
        disable_raw_mode();

    /* Assign back initial value */
    i = 0;
}

void kb_event_task(void *arg)
{
    char c;
    struct task *task = malloc(sizeof(struct task));
    strncpy(task->task_name, ((struct arg *) arg)->task_name,
            sizeof(task->task_name));
    task->task_name[sizeof(task->task_name) - 1] = '\0';
    INIT_LIST_HEAD(&task->list);

    if (setjmp(task->env) == 0) {
        task_add(task);
        longjmp(sched, 1);
    }
    task = cur_task;

    setjmp(task->env);
    task = cur_task;

    if (game_stop)
        task_switch();

    enable_raw_mode();

    if (read(STDIN_FILENO, &c, 1) != 1) {
        task_add(task);
    } else {
        switch (c) {
        case CTRL_KEY('q'):
            game_stop = 1;
            printf("Ctrl+Q detected!\n\r");
            break;
        case CTRL_KEY('p'):
            stop_draw = 1;
            printf("Ctrl+P detected!\n\r");
            break;
        }
    }
    disable_raw_mode();
    task_switch();
}


/* Using MCTS algorithm */
void ai_task_0(void *arg)
{
    srand(time(NULL));
    char ai = 'O';
    struct task *task = malloc(sizeof(struct task));
    strncpy(task->task_name, ((struct arg *) arg)->task_name,
            sizeof(task->task_name));
    task->task_name[sizeof(task->task_name) - 1] = '\0';
    INIT_LIST_HEAD(&task->list);

    if (setjmp(task->env) == 0) {
        task_add(task);
        longjmp(sched, 1);
    }
    task = cur_task;

    setjmp(task->env);
    task = cur_task;

    if (game_stop)
        task_switch();

    /* Check win or lose */
    char win = check_win(task_table);
    if (win == 'D') {
        draw_board(task_table);
        printf("It is a draw!\n");
        goto print_result;
    } else if (win != ' ') {
        if (!won) {
            draw_board(task_table);
            printf("%c: %s won!\n", win, task->task_name);
            won = 1;
        }
        goto print_result;
    }

    /* Make a move */
    int move = mcts(task_table, ai);
    if (move != -1) {
        task_table[move] = ai;
        record_move(move);
    }

    task_add(task);
    task_switch();

print_result:
    longjmp(sched, 1);
}



/* Using negamax algorithm */
void ai_task_1(void *arg)
{
    srand(time(NULL));
    char ai = 'X';
    struct task *task = malloc(sizeof(struct task));
    strncpy(task->task_name, ((struct arg *) arg)->task_name,
            sizeof(task->task_name));
    task->task_name[sizeof(task->task_name) - 1] = '\0';
    INIT_LIST_HEAD(&task->list);

    negamax_init();

    if (setjmp(task->env) == 0) {
        task_add(task);
        longjmp(sched, 1);
    }
    task = cur_task;

    setjmp(task->env);
    task = cur_task;

    if (game_stop)
        task_switch();

    /* Check win or lose */
    char win = check_win(task_table);
    if (win == 'D') {
        draw_board(task_table);
        printf("It is a draw!\n");
        goto print_result;
    } else if (win != ' ') {
        if (!won) {
            draw_board(task_table);
            printf("%c: %s won!\n", win, task->task_name);
            won = 1;
        }
        goto print_result;
    }

    /* Make a move */
    int move = negamax_predict(task_table, ai).move;
    if (move != -1) {
        task_table[move] = ai;
        record_move(move);
    }
    draw_board(task_table);

    task_add(task);
    task_switch();

print_result:
    longjmp(sched, 1);
}

int corutine_ai(void)
{
    void (*registered_task[])(void *) = {ai_task_0, ai_task_1, kb_event_task};
    struct arg arg0 = {.task_name = "MCST_task0"};
    struct arg arg1 = {.task_name = "Negamax_task1"};
    struct arg arg2 = {.task_name = "kb_task"};
    struct arg registered_arg[] = {arg0, arg1, arg2};
    tasks = registered_task;
    args = registered_arg;
    ntasks = ARRAY_SIZE(registered_task);

    /* Clear table */
    memset(task_table, ' ', N_GRIDS);
    draw_board(task_table);
    memset(move_record, 0, N_GRIDS * sizeof(int));
    move_count = 0;

    /* Start AI vs AI */
    schedule();

    won = 0;
    return 0;
}
