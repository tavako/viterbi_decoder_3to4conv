#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
int constraint_length = 4;
int input_symbol_states = 8;
int output_symbol_states = 16;
int polynomials[2] = {5, 7}; // trellis and output tables provided
int states_count = 0;
int n = 3;
int k = 4;
const int REJECT_CODE = -10;
FILE *fptr;

struct pair
{
    int current_state;
    int previous_state;
    int input;
    int total_distance;
};

int hammingDistance(int *x, int *y, int len, bool reverse_second)
{
    int distance = 0;
    for (int i = 0; i < len; i++)
    {
        if (reverse_second)
            distance += x[i] != y[len - i - 1];
        else
            distance += x[i] != y[i];
    }
    return distance;
}

int len_coded(int input)
{
    return input * k / n;
}

int len_original(int coded)
{
    return coded * n / k;
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
            printf("/%d,%dstate:%d,output:%d /", i, j, *(trellis_table + i * input_symbol_states + j), *(outputs_table + i * input_symbol_states + j));
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
    return *(outputs_table + current_state * input_symbol_states + input);
}

void decimal_to_bianry(int *bin_output, int input, int len)
{
    for (int i = 0; i < len; i++)
    {
        *(bin_output + len - i - 1) = input % 2;
        input = input / 2;
    }
}

int binary_to_decimal(int *input, int len)
{
    int dec_output = 0;
    int multiplier = 1;
    for (int i = len - 1; i > -1; i--)
    {
        dec_output = dec_output + *(input + i) * multiplier;
        multiplier = multiplier * 2;
    }
    return dec_output;
}

void output_seperate(int *buffer, int current_state, int input, int *outputs_table)
{
    int output = *(outputs_table + current_state * input_symbol_states + input);
    decimal_to_bianry(buffer, output, 4);
}

void encode(int *encoded, int *bits, int len, int *trellis_table, int *outputs_table, bool seperate)
{
    int state = 0;
    // Encode the message bits.
    for (int i = 0; i < len; i++)
    {
        if (seperate)
        {
            int buffer[4];
            output_seperate(buffer, state, binary_to_decimal((bits + i * 3), 3), outputs_table);
            *(encoded + i * 4) = buffer[3];
            *(encoded + i * 4 + 1) = buffer[2];
            *(encoded + i * 4 + 2) = buffer[1];
            *(encoded + i * 4 + 3) = buffer[0];
        }
        else
        {
            *(encoded + i) = output(state, binary_to_decimal((bits + i * 3), 3), outputs_table);
        }
        state = next_state(state, binary_to_decimal((bits + i * 3), 3), trellis_table);
    }

    // Encode (constaint_ - 1) flushing bits.
    if (seperate)
    {
        int buffer[4];
        output_seperate(buffer, state, 0, outputs_table);
        *(encoded + len * 4) = buffer[3];
        *(encoded + len * 4 + 1) = buffer[2];
        *(encoded + len * 4 + 2) = buffer[1];
        *(encoded + len * 4 + 3) = buffer[0];
    }
    else
    {
        *(encoded + len) = output(state, 0, outputs_table);
    }
    state = next_state(state, 0, trellis_table);
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
        int output = hammingDistance(checking_output, bits, 2, false);
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
        int checking_output[4];
        output_seperate(checking_output, current_state, i, outputs_table);
        int distance = hammingDistance(checking_output, bits, 4, true) + *(min_distance + current_state * len + current_bits_iter - 1);
        int destination = *(trellis_table + current_state * input_symbol_states + i);
        if ((*(min_distance + destination * len + current_bits_iter) >= distance) && *(min_distance + current_state * len + (current_bits_iter - 1)) < __INT_MAX__)
        {
            if (distance == *(min_distance + destination * len + current_bits_iter))
            {
                // new multipath
                for (int j = 0; j < states_count; j++)
                {
                    if (*(previous_states + destination * len * states_count + current_bits_iter * states_count + j) == __INT_MAX__)
                    {
                        *(previous_states + destination * len * states_count + current_bits_iter * states_count + j) = current_state;
                        break;
                    }
                }
            }
            else
            {
                *(min_distance + destination * len + current_bits_iter) = distance;
                *(previous_states + destination * len * states_count + current_bits_iter * states_count) = current_state;
                for (int j = 1; j < states_count; j++)
                {
                    *(previous_states + destination * len * states_count + current_bits_iter * states_count + j) = __INT_MAX__;
                }
            }
        }
    }
}

void decode(int *previous_states, int *min_distances, int *bits, int *trellis_table, int *outputs_table, int len)
{

    for (int j = 0; j < len; j++)
    {
        for (int i = 0; i < states_count; i++)
        {
            *(min_distances + i * len + j) = __INT_MAX__;
            for (int k = 0; k < states_count; k++)
            {
                *(previous_states + i * len * states_count + j * states_count + k) = __INT_MAX__;
            }
        }
    }

    *(previous_states) = 0;
    *(min_distances) = 0;

    for (int i = 0; i < len - 1; i++)
    {
        for (int j = 0; j < states_count; j++)
        {
            path_trace(i + 1, j, bits + 4 * i, trellis_table, outputs_table, previous_states, min_distances, len);
        }
    }

    *(min_distances + (len - 1)) = 0;
    for (int i = 1; i < states_count; i++)
    {
        *(min_distances + len * i + (len - 1)) = __INT_MAX__;
    }
}

int find_input(int *buffer, int start_state, int target_state, int *trellis_table, int len, int target_len)
{
    for (int i = 0; i < input_symbol_states; i++)
    {
        if (*(trellis_table + start_state * input_symbol_states + i) == target_state)
        {
            decimal_to_bianry(buffer, i, target_len);
            return i;
        }
    }
    return -404;
}

int generate_squence(int *buffer, int len)
{
    for (int i = 0; i < len; i++)
    {
        *(buffer + i) = rand() % 2;
    }
}

void inject_error(int *bits, int *output, int len, float percentage)
{
    for (int i = 0; i < len; i = i + 1)
    {
        *(output + i) = *(bits + i);
    }
    // knuth algorithm to generate unique non repeasting random numbers
    int number_error = (int)len * percentage;
    int in, im;
    im = 0;

    for (in = 0; in < len && im < number_error; ++in)
    {
        int rn = len - in;
        int rm = number_error - im;
        if (rand() % rn < rm)
        {
            /* Take it */
            im = im + 1;
            *(output + in) = !*(bits + in);
        }
    }
}

void back_track(int *decoded, int len, int *min_distances, int *trellis_table, int *previous_states)
{
    int states[len];
    int min = __INT_MAX__;
    for (int j = 0; j < states_count; j++)
    {
        if (min > *(min_distances + j * len + len - 1))
        {
            min = *(min_distances + j * len + len - 1);
            states[len - 1] = j;
        }
    }

    for (int i = len - 2; i > -1; i--)
    {
        // handle multipath here
        min = __INT_MAX__;
        states[i] = *(previous_states + states[i + 1] * len * states_count + (i + 1) * states_count);
    }
    for (int i = 0; i < len - 1; i++)
    {
        find_input(decoded + i * 3, states[i], states[i + 1], (int *)trellis_table, len, 3);
    }
}
// returns index path to modify
int reservoir_selection(int iter, int target_count)
{
    if (iter < target_count)
    {
        return iter;
    }
    else
    {
        int r = rand() % (iter);
        if (r < target_count)
        {
            return r;
        }
        return REJECT_CODE;
    }
}

void dotxor(int *target, int *a, int *b, int len)
{
    for (int i = 0; i < len; i++)
    {
        *(target + i) = *(a + i) ^ *(b + i);
    }
    return;
}

void shift_left(int *target, int *source, int len)
{
    for (int i = 0; i < len - 1; i++)
    {
        *(target + i) = *(target + i + 1);
    }
    *(target + len - 1) = *(source);
    return;
}
int len_bin_number(int* buffer , int len){
    int counter = 0;
    for (int i=0 ; i<len ; i++){
        if(*(buffer+i) == 1){
        break;
        }
        counter = counter + 1;
    }
    return counter;
}

void crc(int *buffer, int *input, int *codeWord, int len_input, int len_codeWord)
{
    
    memcpy(buffer , input , len_codeWord * sizeof(int));
    int i=len_codeWord-1;
    int len_bin = len_bin_number(buffer , len_codeWord);
    bool break_inner_loop = false;
    while (i < len_input)
    {
        // if leftmost bit is 1 do the xor otherwise shift by 1 until we reach end of sequence
        while (*(buffer) != 1)
        {
            i = i+1;
            if(i<len_input){
            shift_left(buffer, (input + i), len_codeWord);
            len_bin = len_bin -1;
            }
            else {
               // if(len_bin != 0 ){
                break_inner_loop = true;
                break;
            }
        }
        if (!break_inner_loop){
        dotxor(buffer, buffer, codeWord, len_codeWord);
        len_bin = len_bin_number(buffer , len_codeWord);
        }
    }
}

void calc_crc(int *buffer, int *input, int *codeWord, int len_input, int len_codeWord)
{
    int len_extended = len_input + len_codeWord - 1;
    memset(buffer, 0, len_extended * sizeof(int));
    memcpy(buffer, input, len_input * sizeof(int));
    int internal_buffer[len_codeWord];
    crc(internal_buffer, buffer, codeWord, len_extended, len_codeWord);
    memcpy(buffer + len_input, internal_buffer + 1, sizeof(int) * (len_codeWord - 1));
}

bool check_crc(int *input, int *codeWord, int len, int len_codeWord)
{
    int buffer[len_codeWord];
    crc(buffer, input, codeWord, len, len_codeWord);
    for (int i = 0; i < len_codeWord; i++)
    {
        if (buffer[i] == 1)
            return false;
    }
    return true;
}


void back_track_multipath(int *states , int * current_num_paths , int len, int *min_distances, int *trellis_table, int *previous_states, int max_paths_count)
{
    *current_num_paths = 1;
    int min = __INT_MAX__;
    for (int j = 0; j < states_count; j++)
    {
        if (min > *(min_distances + j * len + len - 1))
        {
            min = *(min_distances + j * len + len - 1);
            *(states + len - 1) = j;
        }
    }
    int iter = 0;
    for (int i = len - 2; i > -1; i--)
    {
        // handle multipath here
        int temp_max_path = *current_num_paths;
        for (int j = 0; j < *current_num_paths; j++)
        {
            *(states+j*len + i) = *(previous_states + *(states+j*len + i+1) * len * states_count + (i + 1) * states_count);
            for (int k = 1; k < states_count; k++)
            {
                if (*(previous_states + *(states+j*len + i+1) * len * states_count + (i + 1) * states_count + k) < __INT_MAX__)
                {
                    // new path
                    iter = iter + 1;
                    int loc = reservoir_selection(iter, max_paths_count);
                    if (loc == REJECT_CODE)
                        continue;
                    if (iter < max_paths_count)
                        temp_max_path = temp_max_path + 1;
                    *(states+loc*len + i) = *(previous_states + *(states+j*len + i+1) * len * states_count + (i + 1) * states_count + k);
                    // copy its hoitory that we have diverged from
                    for (int cp_iter = len - 1; cp_iter > i; cp_iter--)
                    {
                        *(states+loc*len + cp_iter) = *(states+j*len + cp_iter);
                    }
                }
                else
                {
                    break;
                }
            }
        }
        *current_num_paths = temp_max_path;
    }
    // for (int i = 0; i < len - 1; i++)
    // {
    //     find_input((decoded + i * 3), states[0][i], states[0][i + 1], (int *)trellis_table, len, 3);
    // }
    
}

void check_states_crc(int *codeWord , int len_codeWord , int * states , int num_paths , int *trellis_table , int num_states , int* buffer_result, bool* not_found , int len_sequence){
    
    bool found = false;
    for(int j=0 ; j<num_paths ; j++){
    
     for (int i = 0; i < num_states - 1; i++)
    {
        find_input((buffer_result + i * 3), *(states+j*num_states+i), *(states+j*num_states+i + 1), (int *)trellis_table,num_states, 3);
    }
    if(check_crc(buffer_result , codeWord , len_sequence , len_codeWord )){
            found = true;
            break;
        }
    } 
    *not_found = !found;
}

void unit_test(int *errors, int number_of_tries, int length_sequence, float percentage_failure, int *trellis_table, int *outputs_table)
{
    int sequence[length_sequence];
    int length_coded = len_coded((length_sequence + constraint_length - 1));
    int encoded[length_coded];
    int faulty_transmitted[length_coded];
    int decoded[length_sequence + constraint_length - 1];
    int previous_states[states_count][length_coded / k + 1];
    int min_distances[states_count][length_coded / k + 1];
    int total_tries = number_of_tries;
    while (number_of_tries > 0)
    {
        number_of_tries = number_of_tries - 1;
        generate_squence(sequence, length_sequence);
        encode(encoded, sequence, length_sequence / n, trellis_table, outputs_table, true);
        inject_error(encoded, faulty_transmitted, length_coded, percentage_failure);
        decode((int *)previous_states, (int *)min_distances, faulty_transmitted, trellis_table, outputs_table, length_coded / k + 1);
      //  back_track_multipath(decoded, length_coded / k + 1, (int *)min_distances, trellis_table, (int *)previous_states);
        errors[total_tries - number_of_tries - 1] = hammingDistance(sequence, decoded, length_sequence, false);
    }
    return;
}

float avg(int *input, int len)
{
    int sum;
    for (int i = 0; i < len; i++)
    {
        sum = sum + *(input + i);
    }
    return (float)sum / len;
}

void unit_test_crc_matching(int * errors ,int number_of_tries, int length_sequence, float percentage_failure, int *trellis_table, int *outputs_table ){
    int max_number_paths = 10;
    int current_num_paths = 0;
    int sequence[length_sequence];
    //we replace the len_codeWord -1 finishing bits with 0s then crc code it
    int extended_sequence[length_sequence];
    //3 zero flushing bits included
    int length_coded = len_coded((length_sequence + constraint_length - 1));
    int encoded[length_coded];
    int faulty_transmitted[length_coded];
    int states[max_number_paths][length_coded/4+1];
    int decoded[length_sequence + constraint_length - 1];
    int previous_states[states_count][length_coded / k + 1][states_count];
    int min_distances[states_count][length_coded / k + 1];
    int total_tries = number_of_tries;
    int codeWord[] = {1,0,1,1,0};
    int len_codeWord = 5;
    bool not_found = false;
    int counter_not_found = 0;
    while (number_of_tries > 0)
    {
        current_num_paths = 0;
        number_of_tries = number_of_tries - 1;
        generate_squence(sequence, length_sequence);
        calc_crc(extended_sequence , sequence  , codeWord , length_sequence - len_codeWord + 1 , len_codeWord);
        encode(encoded, extended_sequence, length_sequence / n, trellis_table, outputs_table, true);
        inject_error(encoded, faulty_transmitted, length_coded, percentage_failure);
        decode((int *)previous_states, (int *)min_distances, faulty_transmitted, trellis_table, outputs_table, length_coded / k + 1);
        back_track_multipath((int*)states,&current_num_paths ,length_coded / k + 1, (int *)min_distances, trellis_table, (int *)previous_states , max_number_paths);
        not_found = false;
        check_states_crc(codeWord , len_codeWord ,(int*) states , current_num_paths, trellis_table , length_coded/k+1 , decoded , &not_found , length_sequence);
        if(not_found)
        counter_not_found += 1;
        errors[total_tries - number_of_tries - 1] = hammingDistance(extended_sequence, decoded, length_sequence, false);
    }
    return;
}

int main()
{
    states_count = 1 << (constraint_length - 1);
    fptr = fopen("./logs.txt","w");
    int trellis_table[states_count][input_symbol_states];
    int outputs_table[8][8] = {{0, 8, 4, 12, 2, 10, 6, 14}, {4, 12, 2, 10, 6, 14, 0, 8}, {1, 9, 5, 13, 3, 11, 7, 15}, {5, 13, 3, 11, 7, 15, 1, 9}, {3, 11, 7, 15, 1, 9, 5, 13}, {7, 15, 1, 9, 5, 13, 3, 11}, {2, 10, 6, 14, 0, 8, 4, 12}, {6, 14, 0, 8, 4, 12, 2, 10}};
    // generate trellis from polynomial
    for (int i = 0; i < states_count; i++)
    {
        for (int j = 0; j < input_symbol_states; j++)
        {
            trellis_table[i][j] = j;
        }
    }
    check_tables(states_count, (int *)trellis_table, (int *)outputs_table);

    // // test a signle case
    // int input_buffer[141];
    // generate_squence(input_buffer, 141);
    // int size_coded = (sizeof(input_buffer) / sizeof(int) + constraint_length - 1) * 4 / 3;
    // int encoded[size_coded];
    // encode(encoded, input_buffer, (sizeof(input_buffer) / sizeof(int)) / 3, (int *)trellis_table, (int *)outputs_table, true);
    // int decoded[144];
    // int previous_states[states_count][size_coded / 4 + 1][states_count];
    // int min_distances[states_count][size_coded / 4 + 1];
    // decode((int *)previous_states, (int *)min_distances, encoded, (int *)trellis_table, (int *)outputs_table, size_coded / 4 + 1);
    // back_track_multipath(decoded, size_coded / 4 + 1, (int *)min_distances, (int *)trellis_table, (int *)previous_states, 10);
    // printf("error %d \n", hammingDistance(input_buffer, decoded, 141, false));
    
    //crc test
    int buffer[12];
    int buffer2[3];
    int input[] = {0,0,1,1,0,1,0,0,0,0};
    int second_input[] = {1,0,0,1,0,1,1,0,1,0,0};
    int codeWord[] = {1, 0, 1};
    crc(buffer2  , second_input , codeWord , 11 , 3);
    calc_crc(buffer, input, codeWord, 9, 3);
    //buffer[2] = !buffer[2];
    bool res = check_crc(buffer, codeWord, 12, 3);
    
    
    // //unit test no crc matching and multipath
    // int number_of_tries = 1000;
    // int errors[number_of_tries];
    // unit_test(errors, number_of_tries, 12, 0.05f, (int *)trellis_table, (int *)outputs_table);
    // printf("avg error : %f \n", avg(errors, number_of_tries));
    // int counter = 0;
    // for (int i = 0; i < number_of_tries; i++)
    // {
    //     if (errors[i] == 0)
    //         counter++;
    // }

    //unit test , crc matching multipath
    int number_of_tries = 1000;
    int errors[number_of_tries];
    unit_test_crc_matching(errors , number_of_tries , 141 , 0.03f , (int *) trellis_table , (int *) outputs_table);
    printf("avg error : %f \n", avg(errors, number_of_tries));
    int counter = 0;
    for (int i = 0; i < number_of_tries; i++)
    {
        if (errors[i] == 0)
            counter++;
    }
    
    fclose(fptr);

    return 0;
}