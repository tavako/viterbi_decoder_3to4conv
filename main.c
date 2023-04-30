#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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

int hammingDistance(int *x, int *y, int len)
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

void path_metric(struct pair *result, int prev_state, int total_distance, int *bits, int *trellis_table, int *outputs_table)
{

    struct pair p1, p2;
    int min1 = __INT_MAX__;
    int min1_input, output1 = 0;
    int min1_next_state, min2_next_state = 0;
    int min2 = __INT_MAX__;
    int min2_input, output2 = 0;
    for (int i = 0; i < input_symbol_states; i++)
    {
        int checking_output[] = {*(outputs_table + prev_state * input_symbol_states * 2 + i * 2 + 0), *(outputs_table + prev_state * input_symbol_states * 2 + i * 2 + 1)};
        int output = hammingDistance(checking_output, bits, 2);
        if (output < min1)
        {
            output2 = output1;
            min2 = min1;
            min2_input = min1_input;
            min2_next_state = min1_next_state;
            output1 = total_distance + output;
            min1 = output;
            min1_input = i;
            min1_next_state = *(trellis_table + prev_state * input_symbol_states + i);
            continue;
        }
        else if (output < min2)
        {
            output2 = total_distance + output;
            min2 = output;
            min2_input = i;
            min2_next_state = *(trellis_table + prev_state * input_symbol_states + i);
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

void copy_array(struct pair *dest, struct pair *source, int len)
{
    for (int i = 0; i < len; i++)
    {
        (dest + i)->current_state = (source + i)->current_state;
        (dest + i)->previous_state = (source + i)->previous_state;
        (dest + i)->total_distance = (source + i)->total_distance;
        (dest + i)->input = (source + i)->input;
    }
}

void path_trace(int current_bits_iter, int current_state, int *bits, int *trellis_table, int *outputs_table, int *previous_states, int *min_distance, int len)
{
    for (int i = 0; i < input_symbol_states; i++)
    {
        int checking_output[] = {*(outputs_table + current_state * input_symbol_states * 2 + i * 2 + 0), *(outputs_table + current_state * input_symbol_states * 2 + i * 2 + 1)};
        int distance = hammingDistance(checking_output, bits, 2) + *(min_distance + current_state * len + current_bits_iter - 1);
        int destination = *(trellis_table + current_state * input_symbol_states + i);
        if (*(min_distance + destination * len + current_bits_iter) > distance)
        {
            *(min_distance + destination * len + current_bits_iter) = distance;
            *(previous_states + destination * len + current_bits_iter) = current_state;
        }
    }
}

void decode(int *previous_states, int *min_distances, int *bits, int *trellis_table, int *outputs_table, int len)
{

    for (int i = 0; i < states_count; i++)
    {
        *(previous_states + i * len) = 0;
        *(min_distances + i * len) = 0;
    }
    for (int j = 1; j < len; j++)
    {
        for (int i = 0; i < states_count; i++)
        {
            *(previous_states + i * len + j) = __INT_MAX__;
            *(min_distances + i * len + j) = __INT_MAX__;
        }
    }
    for (int i = 0; i < len; i++)
    {
        for (int j = 0; j < states_count; j++)
        {
            path_trace(i + 1, j, bits + 2 * i, trellis_table, outputs_table, previous_states, min_distances, len);
        }
    }
}

int find_input(int start_state, int target_state, int *trellis_table, int len)
{
    for (int i = 0; i < input_symbol_states; i++)
    {
        if (*(trellis_table + start_state * input_symbol_states + i) == target_state)
        {
            return i;
        }
    }
    return -10;
}

int generate_squence(int *buffer, int len)
{
    for (int i = 0; i < len; i++)
    {
        *(buffer + i) = rand() % 2;
    }
}

void inject_error(int *bits,int *output ,  int len, float percentage)
{
    for (int i=0 ; i<len ; i=i+1){
        *(output+i) = *(bits+i);
    }
    //knuth algorithm to generate unique non repeasting random numbers
    int number_error = (int)len * percentage;
    int in, im;
    im = 0;

    for (in = 0; in < len && im < number_error; ++in)
    {
        int rn = len - in;
        int rm = number_error - im;
        if (rand() % rn < rm){
            /* Take it */
            im = im+1;
            *(output + in ) = !*(bits + in );
        }
    }
}

void back_track(int* decoded,int len , int * min_distances , int * trellis_table , int *previous_states){
int states[len];
    int min = __INT_MAX__;
    for (int j = 0; j < states_count; j++)
    {
        if (min > *(min_distances + j*len + len - 1))
        {
            min = *(min_distances + j*len + len - 1);
            states[len - 1] = j;
        }
    }

    for (int i = len - 2; i > -1; i--)
    {
        min = __INT_MAX__;
        states[i] = *(previous_states+states[i + 1]*len + (i + 1));
    }
    for (int i = 0; i < len - 1; i++)
    {
        decoded[i] = find_input(states[i], states[i + 1], (int *)trellis_table, len);
    }

}

void unit_test(int* errors , int number_of_tries, int length_sequence, float percentage_failure, int *trellis_table, int *outputs_table)
{
    int sequence[length_sequence];
    int length_coded = (length_sequence + 2) * 2;
    int encoded[length_coded];
    int faulty_transmitted[length_coded];
    int decoded[length_sequence] ; 
    int previous_states[states_count][length_coded];
    int min_distances[states_count][length_coded];
    int error ; 
    while (number_of_tries > 0)
    {
        number_of_tries = number_of_tries - 1;
        generate_squence(sequence, length_sequence);
        encode(encoded, sequence, length_sequence, trellis_table, outputs_table, true);
        inject_error(encoded ,faulty_transmitted , length_coded , percentage_failure);
        decode((int*)previous_states , (int*)min_distances , faulty_transmitted , trellis_table , outputs_table , length_coded/2);
        back_track(decoded , length_coded/2 , (int*)min_distances , trellis_table , (int*)previous_states);
        error = hammingDistance(sequence , decoded , length_sequence);
    }
    return;
}
int main()
{
    states_count = 1 << (constraint_length - 1);
    int trellis_table[states_count][input_symbol_states];
    int outputs_table[states_count][input_symbol_states][sizeof(polynomials) / sizeof(int)];
    int input_buffer[] = {1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1};
    int current_state = 0;
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
    check_tables(states_count,(int*) trellis_table,(int*) outputs_table);
    int number_of_tries  = 1;
    int errors[number_of_tries];
    unit_test(errors , number_of_tries , 10 , 0.3f , (int*) trellis_table , (int*)outputs_table);
    // int count = sizeof(input_buffer) / sizeof(int) + constraint_length - 1;
    // count = count * 2;
    // int encoded[count];
    // encode((int *)encoded, input_buffer, sizeof(input_buffer) / sizeof(int), (int *)trellis_table, (int *)outputs_table, true);
    // for (int i = 0; i < count; i++)
    // {
    //     printf("%d", encoded[i]);
    // }
    // printf("\n");

    // encoded[3] = 0;
    // encoded[8] = 0;
    // int len = count / 2;
    // int previous_states[states_count][len];
    // int min_distances[states_count][len];
    // decode(previous_states, min_distances, encoded, (int *)trellis_table, (int *)outputs_table, count / 2);
    // // backtrack
    // int decoded[len];
    // back_track(decoded , len , min_distances , trellis_table , previous_states);
    // //    printf("\n");
    // //    printf("error rate:%d \n", hammingDistance(input_buffer, result, count / 2 - 1));
    return 0;
}