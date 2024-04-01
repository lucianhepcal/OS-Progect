#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

void exploreDirectoryTree(DIR *directoryPointer, char *currentPath, int indentationLevel)
{
    struct stat *fileStats = (struct stat *)malloc(sizeof(struct stat));
    if (fileStats == NULL)
        perror("Dynamic allocation failed");
    struct dirent *dirEntry;
    dirEntry = readdir(directoryPointer);

    if (dirEntry == NULL)
    {
        return;
    }
    else
    {
        char *initialPath = malloc((strlen(currentPath) + 3) * sizeof(char));
        strcat(currentPath, "/");
        strcpy(initialPath, currentPath);
        currentPath = realloc(currentPath, (strlen(currentPath) + strlen(dirEntry->d_name) + 1) * sizeof(char));
        strcat(currentPath, dirEntry->d_name);

        int result = stat(currentPath, fileStats);
        for (int i = 0; i <= indentationLevel; i++)
            printf("  ");
        printf("%s\n", dirEntry->d_name);

        if (S_ISDIR(fileStats->st_mode) && strcmp(dirEntry->d_name, ".") && strcmp(dirEntry->d_name, ".."))
        {
            DIR *subDirectory = opendir(currentPath);
            if (subDirectory != NULL)
            {
                exploreDirectoryTree(subDirectory, currentPath, indentationLevel + 1);
                closedir(subDirectory);
            }
        }
        exploreDirectoryTree(directoryPointer, initialPath, indentationLevel);
    }
}

int main(int argc, char **argv)
{
    char *initialPath = strdup("C:\\Users\\lucia\\Desktop\\VS Code\\OS Project\\TheBigFolder");
    if(initialPath == NULL) perror("Dynamic allocation failes");
    DIR *directoryPointer = opendir(initialPath);

    if (directoryPointer == NULL)
    {
        perror("Failed to open directory");
        exit(1);
    }

    exploreDirectoryTree(directoryPointer, initialPath, 0);
    closedir(directoryPointer);
    return 0;
}