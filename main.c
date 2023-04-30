#include <stdio.h>
#include <stdbool.h>
int constraint_length = 3;
int input_symbol_states = 2;
int output_symbol_states = 4;
int polynomials[2] = {5, 7};
int states_count = 0;

struct pair
{
    int current_state;
    int previous_state;
    int input;
    int total_distance;
};

int hammingDistance(int *x,int *y, int len)
{
    int distance = 0;
    for (int i = 0; i < len; i++)
    {
        distance += x[i] != y[i];
    }
    return distance;
}

int hammingWeight(int i)
{
    int r = 0;
    for (int j = 0; j < 32; j++)
    {
        if (i < 0)
            r++;
        i <<= 1;
    }
    return r;
}

void check_tables(int states_count, int *trellis_table, int *outputs_table)
{
    for (int i = 0; i < states_count; i++)
    {
        for (int j = 0; j < input_symbol_states; j++)
        {
            printf("%d,output : %d ", *(trellis_table + i * input_symbol_states + j), *(outputs_table + i * input_symbol_states * 2 + j * 2 + 0) * 2 + *(outputs_table + i * input_symbol_states * 2 + j * 2 + 1));
        }
        printf("\n");
    }
}

int next_state(int current_state, int input, int *trellis_table)
{
    return *(trellis_table + current_state * input_symbol_states + input);
}

int output(int current_state, int input, int *outputs_table)
{
    return *(outputs_table + current_state * input_symbol_states * 2 + input * 2 + 0) * 2 + *(outputs_table + current_state * input_symbol_states * 2 + input * 2 + 1);
}

void output_seperate(int *buffer, int current_state, int input, int *outputs_table)
{
    buffer[0] = *(outputs_table + current_state * input_symbol_states * 2 + input * 2 + 0);
    buffer[1] = *(outputs_table + current_state * input_symbol_states * 2 + input * 2 + 1);
    return;
}

void encode(int *encoded, int *bits, int len, int *trellis_table, int *outputs_table, bool seperate)
{
    int state = 0;
    // Encode the message bits.
    for (int i = 0; i < len; i++)
    {
        if (seperate)
        {
            int buffer[2];
            output_seperate(buffer, state, *(bits + i), outputs_table);
            *(encoded + i * 2) = buffer[0];
            *(encoded + i * 2 + 1) = buffer[1];
        }
        else
        {
            *(encoded + i) = output(state, *(bits + i), outputs_table);
        }
        state = next_state(state, *(bits + i), trellis_table);
    }

    // Encode (constaint_ - 1) flushing bits.
    for (int i = len; i < len + constraint_length - 1; i++)
    {
        if (seperate)
        {
            int buffer[2];
            output_seperate(buffer, state, 0, outputs_table);
            *(encoded + i * 2) = buffer[0];
            *(encoded + i * 2 + 1) = buffer[1];
        }
        else
        {
            *(encoded + i) = output(state, 0, outputs_table);
        }
        state = next_state(state, 0, trellis_table);
    }
}

void path_metric(struct pair *result, int prev_state,int total_distance, int *bits, int *trellis_table, int *outputs_table)
{

    struct pair p1, p2;
    int min1 = __INT_MAX__;
    int min1_input,output1 = 0;
    int min1_next_state , min2_next_state = 0 ;
    int min2 = __INT_MAX__;
    int min2_input,output2 = 0;
    for (int i = 0; i < input_symbol_states; i++)
    {
        int checking_output[] = {*(outputs_table + prev_state * input_symbol_states * 2 + i * 2 + 0), *(outputs_table + prev_state * input_symbol_states * 2 + i * 2 + 1)};
        int output = hammingDistance(checking_output, bits, 2);
        if (output < min1)
        {
            output2 = output1;
            min2 = min1;
            min2_input = min1_input;
            min2_next_state  = min1_next_state;
            output1 = total_distance + output;
            min1 = output;
            min1_input = i;
            min1_next_state =  *(trellis_table + prev_state * input_symbol_states + i) ; 
            continue;
        }
        else if (output < min2)
        {
            output2 = total_distance + output;
            min2 = output;
            min2_input = i;
            min2_next_state  = *(trellis_table + prev_state * input_symbol_states + i);
            continue;
        }
    }
    p1.total_distance = output1;
    p1.current_state = min1_next_state;
    p1.input = min1_input;
    p1.previous_state = prev_state;
    p2.total_distance = output2;
    p2.current_state = min2_next_state;
    p2.input = min2_input;
    p2.previous_state = prev_state;

    result[0] = p1;
    result[1] = p2;
}
void copy_array(struct pair *dest , struct pair * source , int len){
for(int i=0;i<len;i++){
     (dest+i)->current_state = (source+i)->current_state;
     (dest+i)->previous_state = (source+i)->previous_state;
     (dest+i)->total_distance = (source+i)->total_distance;
     (dest+i)->input = (source+i)->input;
     
}
}
void Decode(struct pair * ans_buffer, int *bits, int *trellis_table, int *outputs_table, int len)
{
    struct pair path1[len+1];
    struct pair path2[len+1];
    struct pair start;
    start.previous_state = 0;
    start.total_distance = 0;
    path1[0] = start; 
    path2[0] = start;
    struct pair results[2];
    struct pair results_trace[2];
    path_metric(results, path1[0].previous_state , path1[0].total_distance , bits, trellis_table, outputs_table);
    path1[1] = results[0];
    path2[1] = results[1];
    for (int i = 1; i < len; i++)
    {
        path_metric(results, path1[i].current_state,path1[i].total_distance, bits+i*2, trellis_table, outputs_table);
        path1[i+1] = results[0];
        path_metric(results_trace , path2[i].current_state ,path2[i].total_distance, bits+i*2 , trellis_table , outputs_table);
        path2[i+1] = results_trace[0];
        
        if (results[1].total_distance < results_trace[0].total_distance){
            copy_array(path2  , path1 , i+1);
        }else if(results_trace[1].total_distance < results[0].total_distance){
            copy_array(path1  , path2 , i+1);
        } 
        
    }
    if (path1[len].total_distance < path2[len].total_distance){
        copy_array(ans_buffer , path1 , len+1);
    }
    else{
        copy_array(ans_buffer , path2 , len+1);
    }
}
int main()
{
    states_count = 1 << (constraint_length - 1);
    int trellis_table[states_count][input_symbol_states];
    int outputs_table[states_count][input_symbol_states][sizeof(polynomials) / sizeof(int)];
    int input_buffer[] = {1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1};
    int current_state = 0;
    int polynomial = 0;
    int input = 0;
    // generate trellis from polynomial
    for (int i = 0; i < states_count; i++)
    {
        for (int j = 0; j < input_symbol_states; j++)
        {
            current_state = i;
            trellis_table[i][j] = (current_state >> 1) | (j << (constraint_length - 2));
            for (int k = 0; k < sizeof(polynomials) / sizeof(typeof(polynomials[0])); k++)
            {
                outputs_table[i][j][k] = hammingWeight(((current_state & ((1 << (constraint_length - 1)) - 1)) | j << (constraint_length - 1)) & polynomials[k]) % 2;
            }
        }
    }
    check_tables(states_count  , trellis_table , outputs_table);
    int count = sizeof(input_buffer) / sizeof(int) + constraint_length - 1;
    count = count * 2;
    int encoded[count];
    encode((int *)encoded, input_buffer, sizeof(input_buffer) / sizeof(int), (int *)trellis_table, (int *)outputs_table, true);
    for (int i = 0; i < count; i++)
    {
        printf("%d", encoded[i]);
    }
    printf("\n");
    *(encoded + 8) = 0;
    
    struct pair ans_buffer[count + 1];
    Decode(ans_buffer  , encoded  ,(int*)trellis_table ,(int*)outputs_table  , count/2);
    int result[count/2];
    for(int i=1 ; i<count/2-1 ; i++){
        printf("%d," , ans_buffer[i].input);
        result[i-1] = ans_buffer[i].input;
    }
    printf("\n");
    printf("error rate:%d \n" , hammingDistance(input_buffer ,result , count/2-1 ));
    return 0;
}