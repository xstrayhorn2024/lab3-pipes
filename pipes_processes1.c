// C program to demonstrate use of fork() and a 2-way pipe
// P1 (parent) sends a string to P2 (child) over fd1.
// P2 appends "howard.edu", prints it, prompts for a second
// string, appends that too, and sends the result back over fd2.
// P1 appends "gobison.org" and prints the final string.
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>

int main()
{
    // We use two pipes
    // First pipe (fd1) to send input string from parent to child
    // Second pipe (fd2) to send concatenated string from child to parent

    int fd1[2];  // Used to store two ends of first pipe
    int fd2[2];  // Used to store two ends of second pipe

    char fixed_str[] = "howard.edu";
    char fixed_str2[] = "gobison.org";
    char input_str[100];
    pid_t p;

    if (pipe(fd1)==-1)
    {
        fprintf(stderr, "Pipe Failed" );
        return 1;
    }
    if (pipe(fd2)==-1)
    {
        fprintf(stderr, "Pipe Failed" );
        return 1;
    }

    printf("Enter a string to concatenate:");
    fflush(stdout);
    if (scanf("%99s", input_str) != 1)
    {
        fprintf(stderr, "No input\n");
        return 1;
    }
    fflush(stdout);
    p = fork();

    if (p < 0)
    {
        fprintf(stderr, "fork Failed" );
        return 1;
    }

    // Parent process (P1)
    else if (p > 0)
    {
        char final_str[300];

        close(fd1[0]);  // Close reading end of first pipe
        close(fd2[1]);  // Close writing end of second pipe

        // Write input string to child and close writing end
        write(fd1[1], input_str, strlen(input_str)+1);
        close(fd1[1]);

        // Wait for child to finish
        wait(NULL);

        // Read the string sent back by the child
        if (read(fd2[0], final_str, sizeof(final_str)) <= 0)
            final_str[0] = '\0';
        close(fd2[0]);
        final_str[sizeof(final_str) - 1 - strlen(fixed_str2)] = '\0';

        // Concatenate "gobison.org"
        strcat(final_str, fixed_str2);

        printf("Concatenated string %s\n", final_str);
    }

    // Child process (P2)
    else
    {
        close(fd1[1]);  // Close writing end of first pipe
        close(fd2[0]);  // Close reading end of second pipe

        // Read a string using first pipe
        char concat_str[300];
        if (read(fd1[0], concat_str, 100) <= 0)
            concat_str[0] = '\0';
        concat_str[99] = '\0';
        close(fd1[0]);

        // Concatenate a fixed string with it
        strcat(concat_str, fixed_str);

        printf("Concatenated string %s\n", concat_str);

        // Prompt user for a second string and append it
        char second_str[100];
        printf("Enter a string to concatenate:");
        fflush(stdout);
        if (scanf("%99s", second_str) != 1)
            second_str[0] = '\0';
        strcat(concat_str, second_str);

        // Send it back to the parent
        write(fd2[1], concat_str, strlen(concat_str)+1);
        close(fd2[1]);

        exit(0);
    }
    return 0;
}
