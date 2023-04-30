#include <stdio.h>
#include <stdlib.h>
#define M 10
#define N 100

struct pair{
int a; 
int b; 
int c;
};
typedef struct pair pair ;
int main(){
 int vektor[10];
int in, im;

im = 0;

for (in = 0; in < N && im < M; ++in) {
  int rn = N - in;
  int rm = M - im;
  if (rand() % rn < rm)    
    /* Take it */
    vektor[im++] = in + 1; /* +1 since your range begins from 1 */
}

}