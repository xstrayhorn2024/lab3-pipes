/*
 * Chaining three processes: cat scores | grep <arg> | sort
 *
 * Process tree:
 *   P1 (parent)      -> execs "cat scores", stdout -> pipe1
 *    └─ P2 (child)   -> execs "grep <arg>", stdin <- pipe1, stdout -> pipe2
 *        └─ P3 (grandchild) -> execs "sort", stdin <- pipe2
 *
 * Every process closes all four pipe ends after dup2() so that each
 * reader sees EOF when its writer finishes (otherwise sort would hang).
 *
 * No wait() is used: P1 and P2 replace themselves with cat/grep via
 * execvp(), so no code runs after exec to call wait(). Orphaned
 * children are reparented to init, which reaps them.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    int pipe1[2]; // cat  -> grep
    int pipe2[2]; // grep -> sort
    pid_t pid;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <search term>\n", argv[0]);
        return 1;
    }

    char *cat_args[]  = {"cat", "scores", NULL};
    char *grep_args[] = {"grep", argv[1], NULL};
    char *sort_args[] = {"sort", NULL};

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1)
    {
        perror("pipe");
        return 1;
    }

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        // Child (P2) creates the grandchild (P3)
        pid_t pid2 = fork();
        if (pid2 < 0)
        {
            perror("fork");
            exit(1);
        }

        if (pid2 == 0)
        {
            // Grandchild (P3): sort, stdin <- pipe2
            dup2(pipe2[0], 0);
            close(pipe1[0]); close(pipe1[1]);
            close(pipe2[0]); close(pipe2[1]);
            execvp("sort", sort_args);
            perror("execvp sort");
            exit(1);
        }
        else
        {
            // Child (P2): grep, stdin <- pipe1, stdout -> pipe2
            dup2(pipe1[0], 0);
            dup2(pipe2[1], 1);
            close(pipe1[0]); close(pipe1[1]);
            close(pipe2[0]); close(pipe2[1]);
            execvp("grep", grep_args);
            perror("execvp grep");
            exit(1);
        }
    }
    else
    {
        // Parent (P1): cat scores, stdout -> pipe1
        dup2(pipe1[1], 1);
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        execvp("cat", cat_args);
        perror("execvp cat");
        exit(1);
    }
    return 0;
}
