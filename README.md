# jvcmd: Command-line arguments parsing in C99

**jvcmd** is a C99 library to parse command-line arguments. Its aim is to be readable and simple.
It is licensed under the MIT license, please see the "Building and Licensing" paragraph.
The library is documented in the header `<jvcmd/jvcmd.h>`. 
It relies on C99 designated initializers to keep the configuration short.
However this is optional and you can use the library without designated initializers (i.e. in C++).
See `<examples/filetree.cpp>`.

If this library is useful for you, please give me feedback. =)

## Examples
From `<examples/calc.h>`:
```c
int main(int argc, char** argv) {
    jvArgument integer_option = { "int", "Values are considered as int.", 'i' };
    jvArgument sentence_option = { "sentence", "Will print a sentence instead of the raw result.", 's' };

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
    ... actual logic ...
}
```

The following messages are generated:
```
>>> ./calc
ERROR!
USAGE: ./calc [--int|-i] [--sentence|-s] [--] <operation> <left-value> <right-value> 
At least 3 positional arguments are required, but you gave 0 arguments.
Type './calc --help' for more information.

>>> ./calc --help
Calculate the result of a binary operation.
USAGE: ./calc [--int|-i] [--sentence|-s] [--] <operation> <left-value> <right-value> 

  Positional Arguments:
    <operation>               Operation evaluated on left and right values.
    <left-value>              Left operand
    <right-value>             Right operand

  Options:
    --jvcmd                   License attribution for the jvcmd library.
    --help                    Show this message.
    [--int|-i]                Values are considered as int.
    [--sentence|-s]           Will print a sentence instead of the raw result.

>>> ./calc a b c
ERROR!
USAGE: ./calc [int|-i] [sentence|-s] [--] <operation> <left-value> <right-value> 
Invalid value for option 'operation', 'a' is not in 'add sub mult div'.
Type './calc --help' for more information.

>>> ./calc add 2
ERROR!
USAGE: ./calc [--int|-i] [--sentence|-s] [--] <operation> <left-value> <right-value> 
At least 3 positional arguments are required, but you gave 2 arguments.
Type './calc --help' for more information.
```

## Building and Licensing

To compile the examples from the command line, you can do:
```
gcc jvcmd/*.c examples/calc.c -std=c99 -o calc
g++ jvcmd/*.c examples/filetree.cpp -std=c++17 -o filetree
```

The simplest way to include this library in your project is to put the `jvcmd/*` files among your project source files.
You may have noticed that the library adds automatically the option `--jvcmd`.
This corresponds to the proper copyright notice required by this library's license.
Keeping the `--jvcmd` option and printing it in the `--help` message is sufficient (as done by default).
If you modify the help message to fit your needs, you still need to put a notice for the `--jvcmd` option.


