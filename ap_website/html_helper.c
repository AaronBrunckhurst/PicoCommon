size_t replace_newlines_with_html(char* dst, const char* input) {
    // Calculate the size needed for the output buffer
    size_t input_length = strlen(input);
    size_t output_length = 0;
    for (size_t i = 0; i < input_length; i++) {
        if (input[i] == '\n') {
            output_length += 4; // Add 4 for "<br>"
        } else {
            output_length += 1; // Add 1 for regular characters
        }
    }    

    // Allocate memory for the output buffer
    // char* output = (char*)malloc(output_length + 1); // Add 1 for the null-terminator
    // if (!output) {
    //     // Memory allocation failed
    //     return NULL;
    // }

    // Perform the replacement
    size_t output_index = 0;
    for (size_t i = 0; i < input_length; i++) {
        if (input[i] == '\n') {
            // Replace '\n' with "<br>"
            dst[output_index++] = '<';
            dst[output_index++] = 'b';
            dst[output_index++] = 'r';
            dst[output_index++] = '>';
        } else {
            // Copy regular characters
            dst[output_index++] = input[i];
        }
    }

    // Null-terminate the output buffer
    dst[output_index] = '\0';

    return output_length;
}