#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX 100


unsigned int buffer[MAX];
int fill_ptr = 0;
int use_ptr = 0;
int count = 0;
int done = 0; // Flag to indicate producer is done


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;

//add an item to the buffer
void put(unsigned int value) {
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % MAX;
    count++;
}

// remove an item from the buffer
unsigned int get() {
    unsigned int tmp = buffer[use_ptr];
    use_ptr = (use_ptr + 1) % MAX;
    count--;
    return tmp;
}

//    Producer thread function
void *producer(void *arg) {
    FILE *inputFile = fopen("input-part1.txt", "r");
    if (inputFile == NULL) {
        perror("Error opening input file");
        exit(1);
    }

    unsigned int value;
    while (fscanf(inputFile, "%u", &value) != EOF) {
        pthread_mutex_lock(&mutex); // Lock the mutex

        // Wait if the buffer is full
        while (count == MAX) {
            pthread_cond_wait(&empty, &mutex);
        }

        put(value); // Add value to the buffer

        // Signal the consumer and unlock
        pthread_cond_signal(&fill);
        pthread_mutex_unlock(&mutex);

        // Stop adding more if the value is 0
        if (value == 0) {
            break;
        }
    }

    fclose(inputFile);
    pthread_mutex_lock(&mutex);
    done = 1; // Indicate that producer has finished producing
    pthread_cond_signal(&fill); // Signal the consumer to finish processing
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// Consumer thread function
void *consumer(void *arg) {
    FILE *outputFile = fopen("output-part1.txt", "w");
    if (outputFile == NULL) {
        perror("Error opening output file");
        exit(1);
    }

    while (1) {
        pthread_mutex_lock(&mutex); // Lock the mutex

        // Wait if the buffer is empty and the producer is not done
        while (count == 0 && !done) {
            pthread_cond_wait(&fill, &mutex);
        }

        // Break the loop if the buffer is empty and the producer is done
        if (count == 0 && done) {
            pthread_mutex_unlock(&mutex); // Unlock the mutex
            break;
        }

        unsigned int tmp = get(); 

        // If the value is 0, do not consume it or show it in the buffer state
        if (tmp == 0) {
            pthread_mutex_unlock(&mutex); // Unlock the mutex
            break;
        }

        // Print the consumed value and buffer state to the output file
        fprintf(outputFile, "Consumed:[%u],Buffer-State:[", tmp);
        for (int i = 0; i < count; i++) {
            int index = (use_ptr + i) % MAX;
            // Print the buffer state without the `0`
            if (buffer[index] != 0) {
                fprintf(outputFile, "%u", buffer[index]);
                if (i < count - 1 && buffer[(use_ptr + i + 1) % MAX] != 0) {
                    fprintf(outputFile, ",");
                }
            }
        }
        fprintf(outputFile, "]\n");

        pthread_cond_signal(&empty); // Signal the producer
        pthread_mutex_unlock(&mutex); // Unlock the mutex
    }

    fclose(outputFile);
    return NULL;
}

int main() {
    pthread_t prod, cons;

    // Create producer and consumer threads
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    // Wait for threads to finish
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    return 0;
}
