# C_synchronizing_examples

Designed to run on Unix Systems (Linux). 

Not tested on MacOS!

## Fork with Semaphores and mmap

Simple Parent-Child Orchestration Example using Mainthread and Childthread. 

To run this program, run:

`gcc fork_semaphore_mmap.c -o fsm -lpthread`

and then:

`./fsm`

## N Producer and M Consumer Problem

Multithreading (N+M) with N Producer and M Consumer

To run this program, run:

`gcc producer_consumer.c -o prodcon -lpthread`

and then: 

`./prodcon`

To test this program, run:

```
gcc producer_consumer.c -o prodcon -lpthread && ./prodcon > tests/test.txt && node tests/producer_consumer.js tests/test.txt

```

You will need Node.js for the test.
