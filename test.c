#include <stdio.h>
struct pair{
int a; 
int b; 
int c;
};
typedef struct pair pair ;
int main(){
   pair all[4];
   all[1].a = 2;
   printf("number:%d \n" , (all+1)->a);
}