// Copyright 2015, Google, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This is required on Mac OS X for getting PRI* macros #defined.
#define __STDC_FORMAT_MACROS

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

const size_t mem_size = 1 << 30;
//const int toggles = 540000;
const int cache_size_M = 8;
const int cache_num_bytes = cache_size_M * 1024 * 1024;
const int cache_num_64ints = cache_num_bytes / 8;
const int cache_arr_size = 20 * cache_num_64ints;

char *g_mem;

char *pick_addr() {
  size_t offset = (rand() << 12) % (mem_size - 20 * cache_num_bytes);
  return g_mem + offset;
}

class Timer {
  struct timeval start_time_;

 public:
  Timer() {
    // Note that we use gettimeofday() (with microsecond resolution)
    // rather than clock_gettime() (with nanosecond resolution) so
    // that this works on Mac OS X, because OS X doesn't provide
    // clock_gettime() and we don't really need nanosecond resolution.
    int rc = gettimeofday(&start_time_, NULL);
    assert(rc == 0);
  }

  double get_diff() {
    struct timeval end_time;
    int rc = gettimeofday(&end_time, NULL);
    assert(rc == 0);
    return (end_time.tv_sec - start_time_.tv_sec
            + (double) (end_time.tv_usec - start_time_.tv_usec) / 1e6);
  }
};

static void toggle(int iterations, int addr_count) {
  uint64_t* addr = (uint64_t*) pick_addr();
  uint64_t sum = 0;
  for (int k = 0; k < iterations; ++k) {  
    for (int i = 0; i < cache_arr_size; ++i) {
      sum += addr[i] + 1;
    }
    
    // Sanity check.  We don't expect this to fail, because reading
    // these rows refreshes them.
    if (sum != 0) {
      printf("error: sum=%" PRIu64 "\n", sum);
      exit(1);
    }
  }
}

void main_prog() {
  g_mem = (char *) mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE, -1, 0);
  assert(g_mem != MAP_FAILED);

  printf("clear\n");
  memset(g_mem, 0xff, mem_size);

  Timer t;
  int iter = 0;
  for (;;) {
    printf("Iteration %i (after %.2fs)\n", iter++, t.get_diff());
    toggle(100, 8);

    Timer check_timer;
    uint64_t *end = (uint64_t *) (g_mem + mem_size);
    uint64_t *ptr;
    int errors = 0;
    for (ptr = (uint64_t *) g_mem; ptr < end; ptr++) {
      uint64_t got = *ptr;
      if (got != ~(uint64_t) 0) {
        printf("error at %p: got 0x%" PRIx64 "\n", ptr, got);
        errors++;
      }
    }
    printf("  Checking for bit flips took %f sec\n", check_timer.get_diff());
    if (errors)
      exit(1);
  }
}


int main() {
  // In case we are running as PID 1, we fork() a subprocess to run
  // the test in.  Otherwise, if process 1 exits or crashes, this will
  // cause a kernel panic (which can cause a reboot or just obscure
  // log output and prevent console scrollback from working).
  int pid = fork();
  if (pid == 0) {
    main_prog();
    _exit(1);
  }

  int status;
  if (waitpid(pid, &status, 0) == pid) {
    printf("** exited with status %i (0x%x)\n", status, status);
  }

  for (;;) {
    sleep(999);
  }
  return 0;
}
