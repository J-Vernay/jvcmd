/*
This is the C header for the jvcmd library, written by Julien Vernay ( jvernay.fr ) in 2021.
It contains both the API and the documentation.
jvcmd is a library to parse the command line arguments (argc and argv of the C main function).
The library is available under the MIT License, whose terms are below.
Attribution is handled by the library: it will add the --jvcmd option automatically.
If you disable the automatic --help option, you are then responsible to
notify the user that they can use the --jvcmd option to see the copyright notice of jvcmd.

MIT License

Copyright (c) 2021 Julien Vernay ( jvernay.fr )

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef JV_CMD
#define JV_CMD

#include <stdbool.h>

struct jvParsingConfig;

typedef struct jvArgument {
    /* CONFIG: These fields will be read, each field can be zero-initialized. */
    char const* name;           /* long name */
    char const* help;           /* description of the message */
    char        short_name;     /* short name, 0 if no short name */
    bool        required   : 1; /* 1 if error must be triggered if this argument is omitted */
    bool        need_value : 1; /* 1 if the option must be followed by a value (error if no values),
                                     set automatically if any of is_* is true, or if allowed_values is defined */
    bool        is_int     : 1; /* 1 if the value must be parsed as int (error if not int) */
    bool        is_float   : 1; /* 1 if the value must be parsed as float (error if not float)  */
    bool        is_bool    : 1; /* 1 if the value must be parsed as bool (error if not bool) */
    float       float_min, float_max; /* used if is_float = true and float_min != float_max */
    int         int_min, int_max; /* used if is_int = true and int_min != int_max */
    char const* allowed_values; /* space-delimited allowed values, or NULL if everything is allowed */
    char const* default_value;  /* NULL if no default value, else will be put into 'value' if argument was not specified. */ 
    
    void* userdata; /* Not used by the library, intended for 'action' callback */
    void (*action) (struct jvParsingConfig* config, struct jvArgument* argument); /* Called after OUTPUT values are written to. NULL if nothing to do. */
    
    /* OUTPUT: These fields will be written to. */
    char const* value;     /* Value specified by the user, NULL if option not specified, "" if need_value = 0 and was specified */
    bool        specified; /* 1 if value specified by the user, 0 else.  */
    int         as_int;    /* Value converted as integer if is_int = 1, else it is set to 0. */
    float       as_float;  /* Value converted as float if is_float = 1, else it is set to 0. */
    bool        as_bool;   /*if need_value = 1, conversion to true/false, */
} jvArgument;

typedef struct jvParsingConfig {
    bool        no_help : 1;            /* Do not generate -h/--help
                                           NOTE: You will be charged of notifying the user that they can use --jvcmd
                                                 to see the copyright notice of the jvcmd library. */
    bool        stops_at_last_pos  : 1; /* Stops parsing when the last positional argument is found */
    
    char const* program_name;   /* if NULL (default), argv[0] is considered as the program name.
                                       if non-NULL, argv[0] is considered an option like any other argv[...] */
    char const* description;    /* text to print before generated help, may be NULL */
    char const* usage;          /* if NULL, will be generated by jvstr */
    char const* epilog;         /* text to print after generated help, may be NULL */
    
    char const* short_options_prefix; /* if NULL, "-" is used. if empty, short options are disabled */
    char const* options_prefix;  /* if NULL, "--" is used */
    char const* no_more_options; /* next arguments are only positional. if NULL, "--" is used */
    jvArgument* const* options;  /* options, must be NULL-terminated */
    jvArgument* const* pos_args; /* positional arguments, must be NULL-terminated. 'short_name', 'required' and 'need_value' are ignored. */
    int nb_pos_args_required;    /* number of positional arguments required as minimum */        
    
    char const* true_synonyms; /* space-delimited true-ish values for boolean arguments, if NULL "1 true True TRUE y Y yes Yes YES" is used */
    char const* false_synonyms; /* space-delimited false-ish values for boolean arguments, if NULL "0 false False FALSE n N no No NO" is used */
    
    /* Called when a value is found which was not introduced by an argument, and all positional arguments were processed 
       If NULL, extra arguments will be considered as an error.
       If you just want to discard extra arguments, pass it '&jvcmd_discard_extra_values' */
    void (*action_extra_value) (char const* extra_value, void* userdata); 
    void* userdata; /* Passed to 'action_extra_arg' for user logic */
} jvParsingConfig;



/* Parse the program arguments. */
void jvcmd_parse_arguments(int argc, char** argv, jvParsingConfig config);

/* Print the command-line help to stdout and then call exit(0) */
void jvcmd_exit_with_help(jvParsingConfig const* config);
/* Print the error (formatted with printf) to stderr and then call exit(1) */
void jvcmd_exit_with_error(jvParsingConfig const* config, char const* fmt, ...);

/* Do nothing. Can be used to initialize jvParsingConfig.action_extra_value */
void jvcmd_discard_extra_values(char const* extra_value, void* userdata);


#endif