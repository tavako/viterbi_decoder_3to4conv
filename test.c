#include <stdio.h>
#include <stdlib.h>
#define M 10
#define N 100

#define typename(x) _Generic((x),        /* Get the name of a type */             \
                                                                                  \
        _Bool: "_Bool",                  unsigned char: "unsigned char",          \
         char: "char",                     signed char: "signed char",            \
    short int: "short int",         unsigned short int: "unsigned short int",     \
          int: "int",                     unsigned int: "unsigned int",           \
     long int: "long int",           unsigned long int: "unsigned long int",      \
long long int: "long long int", unsigned long long int: "unsigned long long int", \
        float: "float",                         double: "double",                 \
  long double: "long double",                   char *: "pointer to char",        \
       void *: "pointer to void",                int *: "pointer to int",         \
       int **: "double pointer to int",                int ***: "triple pointer to int",         \
      default: "other")

void check(int *input){
  printf("input was : %d \n" , *(input+5));
}

void decimal_to_bianry(int *output , int input , int len ){
    for (int i=0 ;  i<len ; i++){
        *(output+i) = input%2;
        input = input/2; 
    }
} 
int binary_to_decimal(int* input , int len){
    int dec_output = 0;
    int multiplier = 1;
    for(int i=0 ; i < len ; i++){
        dec_output = dec_output + *(input+i) * multiplier;
        multiplier = multiplier*2;
    }
    return dec_output;
}

int main(){
int disp[2][4] = {
    {10, 11, 12, 13},
    {14, 15, 16, 17}
};
int output[10];
decimal_to_bianry(output , 23 , 10);
int result = binary_to_decimal(output , 10) ; 
printf("input was : %u \n" , **(disp));

}