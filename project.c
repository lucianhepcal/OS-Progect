#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <sys/wait.h>

#define PATH_MAX 4096
#define MAX_ENTRIES 100

struct FileMetadata {
    char entry_name[PATH_MAX];
    time_t last_modified;
    off_t size;
    mode_t permissions;
};

void update_snapshot(const char *dir_path, struct FileMetadata *old_snapshot, const char *isolated_space) {
    printf("Updating snapshot for directory: %s\n", dir_path);

    char snapshot_path[PATH_MAX];
    snprintf(snapshot_path, PATH_MAX, "%s/Snapshot.txt", dir_path);

    int snapshot_fd = open(snapshot_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (snapshot_fd == -1) {
        perror("Failed to open snapshot file");
        exit(EXIT_FAILURE);
    }

    dprintf(snapshot_fd, "Snapshot of directory: %s\n", dir_path);
    dprintf(snapshot_fd, "---------------------------------\n");

    DIR *dir;
    struct dirent *entry;
    dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    //struct FileMetadata new_snapshot[MAX_ENTRIES];
    int num_entries = 0;

    while ((entry = readdir(dir)) != NULL && num_entries < MAX_ENTRIES) {

        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            struct stat st;
            char entry_path[PATH_MAX];
            snprintf(entry_path, PATH_MAX, "%s/%s", dir_path, entry->d_name);

            if (stat(entry_path, &st) == -1) {
                perror("Error getting file status");
                exit(EXIT_FAILURE);
            }

            if(strcmp(entry->d_name, "Snapshot.txt") == 0) continue;
            struct FileMetadata metadata;
            strcpy(metadata.entry_name, entry->d_name);
            metadata.last_modified = st.st_mtime;
            metadata.size = st.st_size;
            int Pipe[2];
            pipe(Pipe);
            pid_t dpid = fork();
            if(dpid == -1){
                perror("fork faile");
                exit(EXIT_FAILURE);
            } else if(dpid == 0) {
                close(Pipe[0]);

                if((S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) && stat(entry_path, &st) != -1 && ((st.st_mode & S_IRUSR) || (st.st_mode & S_IWUSR) || (st.st_mode & S_IXUSR))){
                    
                    metadata.permissions = st.st_mode;
                } else {
                    metadata.permissions = 0;
                }

                write(Pipe[1], &metadata.permissions, sizeof(mode_t));
                
                close(Pipe[1]);
                exit(EXIT_SUCCESS);
            } else {
                close(Pipe[1]);

                mode_t child_permissions;
                read(Pipe[0], &child_permissions, sizeof(mode_t));
                metadata.permissions = child_permissions;
                close(Pipe[0]);
            }
            
            

            //new_snapshot[num_entries++] = metadata;
            
            if(metadata.permissions == 0){
                char newPath[PATH_MAX];
                snprintf(newPath, PATH_MAX, "%s/%s", isolated_space, entry->d_name);
                rename(entry_path, newPath);
            }

            num_entries++;
            dprintf(snapshot_fd, "Entry: %s\n", metadata.entry_name);
            dprintf(snapshot_fd, "Size: %lld bytes\n", (long long)metadata.size);
            dprintf(snapshot_fd, "Permissions: %o\n", metadata.permissions);
            dprintf(snapshot_fd, "Last Modified: %s\n", ctime(&metadata.last_modified));
            dprintf(snapshot_fd, "\n");

            // Recurse into subdirectories
            if (S_ISDIR(st.st_mode)) {
                update_snapshot(entry_path, old_snapshot, isolated_space);
            }
        }
    }

    closedir(dir);
    close(snapshot_fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 12) {
        printf("Usage: %s -s isolated_space_dir dir1 [dir2 ... dir10]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Initial update of snapshots
    for (int i = 2; i < argc; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Child process
            // Execute update_snapshot for the directory*/
            update_snapshot(argv[i], NULL, argv[1]);
            exit(EXIT_SUCCESS);
        }
    }

    // Wait for all child processes to finish
    int status;
    pid_t wpid;
    while ((wpid = wait(&status)) > 0) {
        printf("The child process with PID %d has ended with code %d.\n", wpid, WEXITSTATUS(status));
    }

    return 0;
}
