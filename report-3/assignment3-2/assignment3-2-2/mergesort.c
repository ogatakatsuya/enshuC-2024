#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_ITEMS 10000

void mergeSort(int numbers[], int temp[], int array_size);
void m_sort(int numbers[], int temp[], int left, int right);
void merge(int numbers[], int temp[], int left, int mid, int right);
void parallelMergeSort(int numbers[], int temp[], int left, int right);

int numbers[NUM_ITEMS];
int temp[NUM_ITEMS];

int main()
{
  int i;

  // Seed random number generator
  srand(getpid());

  // Fill array with random integers
  for (i = 0; i < NUM_ITEMS; i++)
    numbers[i] = rand();

  // Measure time for parallel merge sort
  clock_t start = clock();

  // Perform parallel merge sort on array
  parallelMergeSort(numbers, temp, 0, NUM_ITEMS - 1);

  clock_t end = clock();
  double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

  printf("Done with sort.\n");

  for (i = 0; i < NUM_ITEMS; i++)
    printf("%i\n", numbers[i]);

  printf("Time spent for parallel merge sort: %f seconds\n", time_spent);

  return 0;
}

void m_sort(int numbers[], int temp[], int left, int right){
  int mid;

  if (right > left){
    mid = (right + left) / 2;
        
    m_sort(numbers, temp, left, mid);
    m_sort(numbers, temp, mid+1, right);

    merge(numbers, temp, left, mid+1, right);
  }
}

void parallelMergeSort(int numbers[], int temp[], int left, int right)
{
  int mid;

  if (right > left)
  {
    mid = (right + left) / 2;

    pid_t pid1, pid2;
    int pipefd[2];

    // Create a pipe for communication between child processes
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork first child process
    pid1 = fork();
    if (pid1 == 0) {
        // Child process 1: Sort the left half
        close(pipefd[0]); // Close unused read end of pipe
        m_sort(numbers, temp, left, mid);
        write(pipefd[1], numbers, (mid - left + 1) * sizeof(int));
        close(pipefd[1]); // Close write end of pipe
        exit(EXIT_SUCCESS);
    } else if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Parent process: Sort the right half
    m_sort(numbers, temp, mid + 1, right);

    // Wait for the first child process to complete
    close(pipefd[1]); // Close write end of pipe
    waitpid(pid1, NULL, 0);

    // Read sorted left half from the pipe
    int leftHalfSize = mid - left + 1;
    read(pipefd[0], numbers, leftHalfSize * sizeof(int));
    close(pipefd[0]); // Close read end of pipe

    // Merge the sorted halves
    merge(numbers, temp, left, mid + 1, right);
  }
}

void merge(int numbers[], int temp[], int left, int mid, int right)
{
  int i, left_end, num_elements, tmp_pos;

  left_end = mid - 1;
  tmp_pos = left;
  num_elements = right - left + 1;

  while ((left <= left_end) && (mid <= right))
  {
    if (numbers[left] <= numbers[mid])
    {
      temp[tmp_pos] = numbers[left];
      tmp_pos = tmp_pos + 1;
      left = left +1;
    }
    else
    {
      temp[tmp_pos] = numbers[mid];
      tmp_pos = tmp_pos + 1;
      mid = mid + 1;
    }
  }

  while (left <= left_end)
  {
    temp[tmp_pos] = numbers[left];
    left = left + 1;
    tmp_pos = tmp_pos + 1;
  }
  while (mid <= right)
  {
    temp[tmp_pos] = numbers[mid];
    mid = mid + 1;
    tmp_pos = tmp_pos + 1;
  }

  for (i=0; i < num_elements; i++)
  {
    numbers[right] = temp[right];
    right = right - 1;
  }
}
