#include "../jvcmd/jvcmd.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {

    jvArgument integer_option = { "int", "Values are considered as int.", .short_name = 'i' };
    jvArgument sentence_option = { "sentence", "Will print a sentence instead of the raw result.", .short_name = 's' };

    jvArgument operation =    { "operation", "Operation evaluated on left and right values.",
                                  .allowed_values = "add sub mult div" };
    jvArgument left_value =   { "left-value", "Left operand",  .is_float = true };
    jvArgument right_value =  { "right-value", "Right operand", .is_float = true };

    jvArgument* options[] = { &integer_option, &sentence_option, NULL };
    jvArgument* pos_args[] = { &operation, &left_value, &right_value, NULL };

    jvcmd_parse_arguments(argc, argv, (jvParsingConfig) {
        .description = "Calculate the result of a binary operation.",
        .options = options,
        .pos_args = pos_args,
        .nb_pos_args_required = 3,
    });

    float lhs = left_value.as_float;
    float rhs = right_value.as_float;
    if (integer_option.specified) {
        lhs = (int) lhs;
        rhs = (int) rhs;
    }
    
    float result;
    char op;
    switch (operation.value[0]) {
    case 'a': /* add */
        result = lhs + rhs;
        op = '+';
        break;
    case 's': /* sub */
        result = lhs - rhs;
        op = '-';
        break;
    case 'm': /* mult */
        result = lhs * rhs;
        op = '*';
        break;
    case 'd': /* div */
        result = lhs / rhs; /* division by zero is handled correctly with float */
        if (integer_option.specified && rhs != 0) result = (int) result;
        op = '/';
        break;
    default: /* should never appear, has been ensured with operation.allowed_values */
        puts("Unkown operation, there is a bug!");
        exit(EXIT_FAILURE);
    }
    
    if (sentence_option.specified) {
        printf("The result of %g %c %g is %g.\n", lhs, op, rhs, result);
    } else {
        printf("%g\n", result);
    }
    
    return 0;
}
