#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
//b
typedef struct _rwlock_t {
    sem_t lock;          // Semaphore for protecting the readers count
    sem_t writelock;     // Semaphore to allow only one writer or block readers when a writer is active
    int readers;         // Number of readers currently in the critical section
    int writers_waiting; // Number of writers waiting
} rwlock_t;

rwlock_t rwlock;

// Initialize reader-writer lock
void rwlock_init(rwlock_t *rw) {
    rw->readers = 0;
    rw->writers_waiting = 0;
    sem_init(&rw->lock, 0, 1);
    sem_init(&rw->writelock, 0, 1);
}

// Acquire read lock with writer preference logic
void rwlock_acquire_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    while (rw->writers_waiting > 0) {
        // Wait if there are writers waiting to give priority to writers
        sem_post(&rw->lock);
        sem_wait(&rw->lock);
    }
    rw->readers++;
    int current_readers = rw->readers;  // Capture current reader count
    if (rw->readers == 1) {
        sem_wait(&rw->writelock); // First reader locks the writers out
    }
    sem_post(&rw->lock);

    // Log reading status
    FILE *output = fopen("output-writer-pref.txt", "a");
    if (output != NULL) {
        fprintf(output, "Reading,Number-of-readers-present:[%d]\n", current_readers);
        fclose(output);
    }
}

// Release read lock
void rwlock_release_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0) {
        sem_post(&rw->writelock); // Last reader releases the writer lock
    }
    sem_post(&rw->lock);
}

// Acquire write lock with writer preference logic
void rwlock_acquire_writelock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->writers_waiting++; // Increment the count of waiting writers
    sem_post(&rw->lock);

    sem_wait(&rw->writelock); // Acquire the write lock

    sem_wait(&rw->lock);
    rw->writers_waiting--; // Decrement the count of waiting writers when the writer starts
    sem_post(&rw->lock);

    // Log writing status
    FILE *output = fopen("output-writer-pref.txt", "a");
    if (output != NULL) {
        fprintf(output, "Writing,Number-of-readers-present:[%d]\n", rw->readers);
        fclose(output);
    }
}

// Release write lock
void rwlock_release_writelock(rwlock_t *rw) {
    sem_post(&rw->writelock);
}

// Reader function
void *reader(void *arg) {
    rwlock_acquire_readlock(&rwlock);

    // Simulate reading from the shared file
    FILE *file = fopen("shared-file.txt", "r");
    if (file != NULL) {
        fclose(file);
    }

    rwlock_release_readlock(&rwlock);
    return NULL;
}

// Writer function
void *writer(void *arg) {
    rwlock_acquire_writelock(&rwlock);

   
    FILE *file = fopen("shared-file.txt", "a");
    if (file != NULL) {
        fprintf(file, "Hello world!\n");
        fclose(file);
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
FILE *output = fopen("output-writer-pref.txt", "w");
if (output != NULL) {
    fclose(output);
}

    // Create reader and writer threads
    for (long i = 0; i < n; i++) pthread_create(&readers[i], NULL, reader, NULL);
    for (long i = 0; i < m; i++) pthread_create(&writers[i], NULL, writer, NULL);

    // Wait for all threads to complete
    for (int i = 0; i < n; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++) pthread_join(writers[i], NULL);

    return 0;
}



