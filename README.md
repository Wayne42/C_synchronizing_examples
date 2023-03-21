# C_synchronizing_examples

Designed to run on Unix Systems (Linux).

Not tested on MacOS!

## Fork with Semaphores and mmap

Simple Parent-Child Orchestration Example using Fork to create a Childprocess.

To run this program, run:

`gcc fork_semaphore_mmap.c -o fsm -lpthread`

and then:

`./fsm`

## N Producer and M Consumer Problem

See: https://www.cs.fsu.edu/~baker/realtime/restricted/notes/prodcons.html

> ### Producer-Consumer Problem
>
> * One or more threads generate data and put it into a buffer
> * One or more threads take data items from the buffer, one at time
> * Only one producer or consumer may access the buffer at any one time
> * Variants, of increasing difficulty:
>   * Single producer & single consumer
>   * Multiple producers & single consumer
>   * Multiple producers & multiple consumers

Multithreading (N+M) with N Producer and M Consumer using Semaphores, Mutex and Circular Buffer

To run this program, run:

`gcc producer_consumer.c -o prodcon -lpthread`

and then:

`./prodcon`

To test this program, run:

```
gcc producer_consumer.c -o prodcon -lpthread && ./prodcon > tests/test.txt && node tests/producer_consumer.js tests/test.txt

```

You will need Node.js for the test.
