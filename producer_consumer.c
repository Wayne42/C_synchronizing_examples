/*
N Producer and M Consumer Problem using Mutex and Semaphores in C
*/

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
// #include <unistd.h> // for sleep

/* N=4 Producer and M=2 Consumer */
/* Choose Number of Producer and Consumer as desired */
#define NR_OF_PRODUCERS 4
#define NR_OF_CONSUMERS 2

/* Upper Bound of Items to work on (produce and consume) until every Thread stops */
#define NR_OF_ITEMS 1337 // must be greater than NR_OF_CONSUMERS + NR_OF_PRODUCERS

/* Size of Buffer */
#define BUFFER_SIZE 7
/* Circular Buffer */
char buffer[BUFFER_SIZE];

typedef struct
{
    sem_t *used_slots;
    sem_t *free_slots;
    pthread_mutex_t *producer_lock;
    pthread_mutex_t *producer_done_check;
    pthread_mutex_t *consumer_lock;
    pthread_mutex_t *consumer_done_check;
    int buffer_in_ptr;
    int buffer_out_ptr;
    int number_of_produced_items;
    int number_of_consumed_items;
} param_t;

/* producing something... for example calculating something expensive */
/* here we just create a random char */
void produce(char *ptr)
{
    *ptr = '0' + (rand() % 9);
    // sleep(2);
}
void lock_other_producer(pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex) != 0)
    {
        printf("ERROR: pthread_mutex_lock on producer failed.\n");
        exit(EXIT_FAILURE);
    }
}
void unlock_other_producer(pthread_mutex_t *mutex)
{
    if (pthread_mutex_unlock(mutex) != 0)
    {
        printf("ERROR: pthread_mutex_unlock on producer failed.\n");
        exit(EXIT_FAILURE);
    }
}
int prod_quit = 0;
void *producer(void *args)
{
    param_t *data = (param_t *)args;
    while (1)
    {
        lock_other_producer(data->producer_done_check);
        // Start Critical Section: Check whether to exit producer
        if (data->number_of_produced_items >= NR_OF_ITEMS)
        {
            unlock_other_producer(data->producer_done_check);
            prod_quit++;
            printf("Producer quit: %d\n", prod_quit);
            return NULL;
        }
        data->number_of_produced_items++;
        // End Critical Section
        unlock_other_producer(data->producer_done_check);

        // Produce Item async
        char item_w;
        produce(&item_w);

        // Check if Buffer has free Slots to fill with Data
        if (sem_wait(data->free_slots) != 0)
        {
            printf("ERROR: sem_wait failed.\n");
            exit(EXIT_FAILURE);
        };

        lock_other_producer(data->producer_lock);

        // Start: Critical Section: Writing to Buffer; Only one Producer at a time
        buffer[data->buffer_in_ptr] = item_w;
        printf("Producer: Wrote %c to buffer at position %d\n", buffer[data->buffer_in_ptr], data->buffer_in_ptr);
        data->buffer_in_ptr = (data->buffer_in_ptr + 1) % BUFFER_SIZE;
        // End: Critical Section

        unlock_other_producer(data->producer_lock);

        // Notify Consumer that there is one more Data Slot filled and ready to consume
        if (sem_post(data->used_slots) != 0)
        {
            printf("ERROR: sem_post failed.\n");
            exit(EXIT_FAILURE);
        };
    }
}

/* consuming something... for example saving to a file */
void consume(char c, int pos)
{
    printf("Consumer: Consuming %c from position %d \n", c, pos);
    // sleep(1);
}
void lock_other_consumer(pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex) != 0)
    {
        printf("ERROR: pthread_mutex_lock on consumer failed.\n");
        exit(EXIT_FAILURE);
    }
}
void unlock_other_consumer(pthread_mutex_t *mutex)
{
    if (pthread_mutex_unlock(mutex) != 0)
    {
        printf("ERROR: pthread_mutex_unlock on consumer failed.\n");
        exit(EXIT_FAILURE);
    }
}
int cons_quit = 0;
void *consumer(void *args)
{
    param_t *data = (param_t *)args;
    while (1)
    {
        lock_other_consumer(data->consumer_done_check);
        // Start Critical Section: Check whether to exit Consumer
        if (data->number_of_consumed_items >= NR_OF_ITEMS)
        {
            unlock_other_consumer(data->consumer_done_check);
            cons_quit++;
            printf("Consumer quit: %d\n", cons_quit);
            return NULL;
        }
        data->number_of_consumed_items++;
        // End Critical Section
        unlock_other_consumer(data->consumer_done_check);

        char item_r;
        int pos;
        if (sem_wait(data->used_slots) != 0)
        {
            printf("ERROR: sem_wait failed.\n");
            exit(EXIT_FAILURE);
        };

        lock_other_consumer(data->consumer_lock);

        // Start: Critical Section: Reading from Buffer; Only one Consumer at a time
        item_r = buffer[data->buffer_out_ptr];
        pos = data->buffer_out_ptr;
        data->buffer_out_ptr = (data->buffer_out_ptr + 1) % BUFFER_SIZE;
        // End: Critical Section

        unlock_other_consumer(data->consumer_lock);

        if (sem_post(data->free_slots) != 0)
        {
            printf("ERROR: sem_post failed.\n");
            exit(EXIT_FAILURE);
        };

        consume(item_r, pos);
    }
}

int main(int argc, const char *argv[])
{
    printf("Main-thread: started.\n");

    /* Declare Semaphores for Synchronization */
    sem_t used_slots;
    sem_t free_slots;

    // Init Semaphores
    if (sem_init(&used_slots, 0, 0) < 0)
    {
        printf("ERROR: sem_init failed.\n");
        return EXIT_FAILURE;
    }
    if (sem_init(&free_slots, 0, BUFFER_SIZE) < 0)
    {
        printf("ERROR: sem_init failed.\n");
        return EXIT_FAILURE;
    }

    // Init Mutex
    pthread_mutex_t producer_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t consumer_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t producer_done_check = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t consumer_done_check = PTHREAD_MUTEX_INITIALIZER;

    // Init shared Data (Shared Data instead of Global Variables)
    param_t *data = malloc(sizeof(param_t));

    data->consumer_lock = &consumer_lock;
    data->producer_lock = &producer_lock;
    data->consumer_done_check = &consumer_done_check;
    data->producer_done_check = &producer_done_check;

    data->used_slots = &used_slots;
    data->free_slots = &free_slots;

    data->buffer_in_ptr = 0;
    data->buffer_out_ptr = 0;
    data->number_of_consumed_items = 0;
    data->number_of_produced_items = 0;

    // Declare Producer Threads
    pthread_t prod_ref[NR_OF_PRODUCERS];
    pthread_t cons_ref[NR_OF_CONSUMERS];

    // Create Producer Threads
    for (int i = 0; i < NR_OF_PRODUCERS; i++)
    {
        if (pthread_create(&prod_ref[i], NULL, producer, data) != 0)
        {
            printf("ERROR: pthread_create failed.\n");
            return EXIT_FAILURE;
        }
    }

    // Create Consumer Threads
    for (int i = 0; i < NR_OF_CONSUMERS; i++)
    {
        if (pthread_create(&cons_ref[i], NULL, consumer, data) != 0)
        {
            printf("ERROR: pthread_create failed.\n");
            return EXIT_FAILURE;
        }
    }

    // printf("Main-thread: waiting for all threads to finish.\n");

    // Join Producer Threads
    for (int i = 0; i < NR_OF_PRODUCERS; i++)
    {
        if (pthread_join(prod_ref[i], NULL) != 0)
        {
            printf("ERROR: pthread_join failed.\n");
            return EXIT_FAILURE;
        }
    }

    // Join Consumer Threads
    for (int i = 0; i < NR_OF_CONSUMERS; i++)
    {
        if (pthread_join(cons_ref[i], NULL) != 0)
        {
            printf("ERROR: pthread_join failed.\n");
            return EXIT_FAILURE;
        }
    }

    // Cleanup
    if (sem_destroy(&used_slots) < 0)
    {
        printf("ERROR: sem_destroy failed.\n");
        return EXIT_FAILURE;
    }
    if (sem_destroy(&free_slots) < 0)
    {
        printf("ERROR: sem_destroy failed.\n");
        return EXIT_FAILURE;
    }
    free(data);

    printf("Main-thread: finished.\n");
    return EXIT_SUCCESS;
}
