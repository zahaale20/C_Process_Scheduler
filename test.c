#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    FILE *file;
    char command[1024]; // Assuming command length is less than 1024 characters

    // Open the 'commands' file
    file = fopen("commands", "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    int i = 0;

    // Read and execute each command line by line
    while (fgets(command, sizeof(command), file) != NULL) {
        i++;
        // Remove the newline character at the end of the command
        command[strcspn(command, "\n")] = 0;

        printf("test #%d: %s\n", i, command);

        int status = system(command); // Execute command

        if (status == -1) { // System call failed
            printf("System call failed to execute command.\n");
            fclose(file);
            return EXIT_FAILURE;
        } else { // Check exit status
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) { // Success
                printf("success: %s\n\n", command);
            } else { // Failure
                printf("error: %s\n\n", command);
            }
        }
    }

    fclose(file); // Close the file
    return EXIT_SUCCESS;
}