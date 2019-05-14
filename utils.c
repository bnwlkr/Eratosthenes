//
//  utils.c
//  
//
//  Created by Ben Walker on 2019-05-14.
//

#include "utils.h"

void print(char * array, int size) {
  printf("[");
  for (int i = 0; i < size; i++) {
    if (array[i] == 0) {
      printf(" %d ", i);
    }
  }
  printf("]\n");
}
