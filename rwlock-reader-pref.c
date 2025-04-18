#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct _rwlock_t {
    sem_t lock;        // Binary semaphore for protecting the readers count
    sem_t writelock;   // Semaphore to allow one writer or multiple readers
    int readers;       // Number of readers in the critical section
} rwlock_t;

rwlock_t rwlock;

// Initialize reader-writer lock
void rwlock_init(rwlock_t *rw) {
    rw->readers = 0;
    sem_init(&rw->lock, 0, 1);
    sem_init(&rw->writelock, 0, 1);
}

// Acquire read lock (reader preference)
void rwlock_acquire_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers++;
    int current_readers = rw->readers;

    // If first reader, lock the write access
    if (rw->readers == 1) {
        sem_wait(&rw->writelock);
    }
    sem_post(&rw->lock);

    // Log reader count to output file
    FILE *output_file = fopen("output-reader-pref.txt", "a");
    fprintf(output_file, "Reading,Number-of-readers-present:[%d]\n", current_readers);
    fclose(output_file);
}

// Release read lock
void rwlock_release_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers--;

    // If last reader, release the write access
    if (rw->readers == 0) {
        sem_post(&rw->writelock);
    }
    sem_post(&rw->lock);
}

// Acquire write lock
void rwlock_acquire_writelock(rwlock_t *rw) {
    sem_wait(&rw->writelock);

    // Log writer entry to output file
    FILE *output_file = fopen("output-reader-pref.txt", "a");
    fprintf(output_file, "Writing,Number-of-readers-present:[%d]\n", rw->readers);
    fclose(output_file);
}

// Release write lock
void rwlock_release_writelock(rwlock_t *rw) {
    sem_post(&rw->writelock);
}

void *reader(void *arg) {
    rwlock_acquire_readlock(&rwlock);
    
    // Simulate reading from the shared file
    FILE *shared_file = fopen("shared-file.txt", "r");
    if (shared_file) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), shared_file) != NULL) {
            // Simulate reading
        }
        fclose(shared_file);
    }
    
    rwlock_release_readlock(&rwlock);
    return NULL;
}

void *writer(void *arg) {
    rwlock_acquire_writelock(&rwlock);
    
    // Append "Hello world!" to the shared file
    FILE *shared_file = fopen("shared-file.txt", "a");
    if (shared_file) {
        fprintf(shared_file, "Hello world!\n");
        fclose(shared_file);
    }
    
    rwlock_release_writelock(&rwlock);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];
    
    rwlock_init(&rwlock);
    // Clear the output file at the start of execution
FILE *output = fopen("output-reader-pref.txt", "w");
if (output != NULL) {
    fclose(output);
}

    
    // Create reader and writer threads
    for (int i = 0; i < n; i++) pthread_create(&readers[i], NULL, reader, NULL);        
    for (int i = 0; i < m; i++) pthread_create(&writers[i], NULL, writer, NULL);
    
    // Wait for all threads to complete
    for (int i = 0; i < n; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++) pthread_join(writers[i], NULL);
    
    return 0;
}
