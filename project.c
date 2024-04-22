#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct FileSnapshot {
    char name[256];
    int size;
    char location[300];
    ino_t inode;
    time_t lastModified;
};

void generateFileSnapshots(DIR *dirp, char *path, int level, struct FileSnapshot *snapshots) {
    struct dirent *dp;
    struct stat buf;
    char *newPath = (char *)malloc(5000);
    
    while ((dp = readdir(dirp)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue; 
        }
        strcpy(newPath, path);
        strcat(newPath, dp->d_name);
        stat(newPath, &buf);
        strcpy(snapshots[level].name, dp->d_name);
        snapshots[level].size = buf.st_size;
        strcpy(snapshots[level].location, newPath);
        snapshots[level].inode = buf.st_ino;
        snapshots[level].lastModified = buf.st_mtime;

        level++;
        if (S_ISDIR(buf.st_mode)) {
            strcat(newPath, "/");
            DIR *newDir = opendir(newPath);
            generateFileSnapshots(newDir, newPath, level, snapshots);
            closedir(newDir);
        }
    }

    free(newPath);
}

void createSnapshots(int argc, char **argv) {
    struct FileSnapshot snapshots[1000];
    for (int i = 0; i < 1000; i++) {
        snapshots[i].size = -1;
    }
    for (int i = 1; i < argc; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork error");
            exit(1);
        } else if (pid == 0) { 
            DIR *dirp = opendir(argv[i]);
            if (dirp == NULL) {
                perror("opendir error");
                exit(1);
            }

            generateFileSnapshots(dirp, argv[i], 0, snapshots);
            closedir(dirp);
            char snapLocation[5000];
            strcpy(snapLocation, argv[i]);
            strcat(snapLocation, "snapshot.txt");
            FILE *filePtr = fopen(snapLocation, "w");
            if (filePtr == NULL) {
                perror("fopen error");
                exit(1);
            }
            for (int j = 0; j < 1000; j++) {
                fprintf(filePtr, "%s %d %s %ld %ld\n", snapshots[j].name, snapshots[j].size, 
                        snapshots[j].location, snapshots[j].inode, snapshots[j].lastModified);
            }
            fclose(filePtr);
            exit(0);
        }
    }
    for (int i = 1; i < argc; i++) {
        wait(NULL);
    }
}

int main(int argc , char **argv) {//TODO debug cu numar mai mare de foldere/fisiere
    if (argc < 2) {
        printf("Usage: %s <directory1> <directory2> ...\n", argv[0]);
        return 0;
    }
    createSnapshots(argc, argv);
    return 0;
}
