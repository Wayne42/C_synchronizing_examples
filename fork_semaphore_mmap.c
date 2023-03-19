#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>

int main()
{
	printf("MAIN RUNNING BEFORE FORK PID: %d \n", getpid());

	/* Create shared Memory for Parent and Child Process */
	size_t data_size = sizeof(int) * 10;
	int *data = mmap(NULL, data_size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	/* Create shared Semaphores for Synchronization */
	sem_t *childlock = mmap(NULL, sizeof(sem_t), PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_t *parentlock = mmap(NULL, sizeof(sem_t), PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	// Init Semaphores with 0 (both are locked)
	// sem = 0 -> locked
	// sem >= 1 -> unlocked
	if (sem_init(childlock, 1, 0) < 0)
	{
		printf("ERROR: sem_init of childlock failed.\n");
		return EXIT_FAILURE;
	}
	if (sem_init(parentlock, 1, 0) < 0)
	{
		printf("ERROR: sem_init of parentlock failed.\n");
		return EXIT_FAILURE;
	}

	/* Create Child Process; Note: Child Process starts after fork line */
	pid_t pid = fork();
	printf("MAIN RUNNING AFTER FORK: %d \n", getpid());
	if (pid == (pid_t)0)
	{ // Child Process

		// Child notifies Parent that it is initialized and ready to compute
		if (sem_post(parentlock) != 0)
		{
			printf("ERROR: sem_post of parentlock failed.\n");
			exit(EXIT_FAILURE);
		}

		printf("Child: waiting for data-ready signal ...\n");

		if (sem_wait(childlock) != 0)
		{
			printf("ERROR: sem_wait of childlock failed.\n");
			exit(EXIT_FAILURE);
		}

		// Start: 2nd Critical Section
		printf("Child: computing ...\n");
		int sum = 0;
		for (int i = 0; i < 10; i++)
		{
			sum += data[i];
		}

		data[0] = sum;
		printf("Child: send result-ready signal.\n");
		// End: 2nd Critical Section

		if (sem_post(parentlock) != 0)
		{
			printf("ERROR: sem_post of parentlock failed.\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{ // Parent Process

		// Parent waits for Childprocess to unlock the Parent.
		// This ensures that the Parent only starts to run after the Child is ready
		if (sem_wait(parentlock) != 0)
		{
			printf("ERROR: sem_wait of parentlock failed.\n");
			exit(EXIT_FAILURE);
		}

		// Start: 1st Critical Section
		printf("Parent: filling in sample data: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10\n");
		for (int i = 0; i < 10; i++)
		{
			data[i] = i + 1;
		}

		printf("Parent: signal data-ready to child and wait\n");
		// End: 1st Critical Section

		if (sem_post(childlock) != 0)
		{
			printf("ERROR: sem_post of childlock failed.\n");
			exit(EXIT_FAILURE);
		}

		if (sem_wait(parentlock) != 0)
		{
			printf("ERROR: sem_wait of parentlock failed.\n");
			exit(EXIT_FAILURE);
		}

		// Start: 3rd Critical Section
		printf("Parent: received result-read signal from child\n"); 
		printf("SUM = %d\n", data[0]);
		printf("Parent: done.\n");
		// End: 3rd Critical Section

		// Free Ressources when not needed anymore...
		if (sem_destroy(parentlock) < 0)
		{
			printf("ERROR: sem_destroy of parentlock failed.\n");
			exit(EXIT_FAILURE);
		}
		if (sem_destroy(childlock) < 0)
		{
			printf("ERROR: sem_destroy of childlock failed.\n");
			exit(EXIT_FAILURE);
		}
		// On success, munmap() returns 0, on failure -1, and errno is set (probably to EINVAL). 
		if (munmap(data, data_size) == -1)
		{
			printf("ERROR: munmap data failed.\n");
			exit(EXIT_FAILURE);
		};
	}

	return EXIT_SUCCESS;
}

/*
https://man7.org/linux/man-pages/man2/mmap.2.html
void *mmap(void *addr, size_t length, int prot, int flags,
				  int fd, off_t offset);
	   mmap() creates a new mapping in the virtual address space of the
	   calling process.  The starting address for the new mapping is
	   specified in addr.  The length argument specifies the length of
	   the mapping (which must be greater than 0).

	   If addr is NULL, then the kernel chooses the (page-aligned)
	   address at which to create the mapping; this is the most portable
	   method of creating a new mapping.  If addr is not NULL, then the
	   kernel takes it as a hint about where to place the mapping; on
	   Linux, the kernel will pick a nearby page boundary (but always
	   above or equal to the value specified by
	   /proc/sys/vm/mmap_min_addr) and attempt to create the mapping
	   there.  If another mapping already exists there, the kernel picks
	   a new address that may or may not depend on the hint.  The
	   address of the new mapping is returned as the result of the call.

	   The contents of a file mapping (as opposed to an anonymous
	   mapping; see MAP_ANONYMOUS below), are initialized using length
	   bytes starting at offset offset in the file (or other object)
	   referred to by the file descriptor fd.  offset must be a multiple
	   of the page size as returned by sysconf(_SC_PAGE_SIZE).

	   After the mmap() call has returned, the file descriptor, fd, can
	   be closed immediately without invalidating the mapping.

	   The prot argument describes the desired memory protection of the
	   mapping (and must not conflict with the open mode of the file).
	   It is either PROT_NONE or the bitwise OR of one or more of the
	   following flags: ...
*/

/*
https://man7.org/linux/man-pages/man3/sem_init.3.html
int sem_init(sem_t *sem, int pshared, unsigned int value);

	   sem_init() initializes the unnamed semaphore at the address
	   pointed to by sem.  The value argument specifies the initial
	   value for the semaphore.

	   The pshared argument indicates whether this semaphore is to be
	   shared between the threads of a process, or between processes.

	   If pshared has the value 0, then the semaphore is shared between
	   the threads of a process, and should be located at some address
	   that is visible to all threads (e.g., a global variable, or a
	   variable allocated dynamically on the heap).

	   If pshared is nonzero, then the semaphore is shared between
	   processes, and should be located in a region of shared memory
	   (see shm_open(3), mmap(2), and shmget(2)).  (Since a child
	   created by fork(2) inherits its parent's memory mappings, it can
	   also access the semaphore.)  Any process that can access the
	   shared memory region can operate on the semaphore using
	   sem_post(3), sem_wait(3), and so on.

	   Initializing a semaphore that has already been initialized
	   results in undefined behavior.
*/
