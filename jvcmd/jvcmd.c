/*
This is the C implementation for the jvcmd library, written by Julien Vernay ( jvernay.fr ) in 2021.
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

#include "jvcmd.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>

#include "StrView.h"

static const int help_name_padding = 25;


#define SET_IF_NULL(var, value) ((var) == NULL ? (var) = (value) : NULL)


static void print_usage(FILE* f, jvParsingConfig const* config) {
    fprintf(f, "USAGE: %s ", config->program_name);
    jvArgument* const* options = config->options;
    for (jvArgument* option; (option = *options) != NULL; ++options) {
        if (!option->required)
            fputs("[", f);
        fprintf(f, "%s%s", config->options_prefix, option->name);
        if (config->short_options_prefix[0] != '\0' && option->short_name != '\0')
            fprintf(f, "|%s%c", config->short_options_prefix, option->short_name);
        if (option->need_value)
            fputs(" ...", f);
        if (!option->required)
            fputs("]", f);
        fputc(' ', f);
    }
    
    if (config->no_more_options[0] != '\0') {
        fprintf(f, "[%s] ", config->no_more_options);
    }
    
    jvArgument* const* pos_args = config->pos_args;
    int arg_pos = 0;
    for (jvArgument* arg; (arg = *pos_args) != NULL; ++pos_args) {
        if (arg_pos < config->nb_pos_args_required)
            fprintf(f, "<%s> ", arg->name);
        else
            fprintf(f, "[%s] ", arg->name);
        ++arg_pos;
    }
    fputc('\n', f);
}


/* Print the command-line help to stdout and then call exit(0) */
void jvcmd_exit_with_help(jvParsingConfig const* config) {
    if (config->description != NULL)
        puts(config->description);
    print_usage(stdout, config);
    
    puts("\n  Positional Arguments:");
    jvArgument* const* pos_args = config->pos_args;
    int arg_pos = 0;
    for (jvArgument* arg; (arg = *pos_args) != NULL; ++pos_args) {
        int nb_padding = help_name_padding - 3 - strlen(arg->name);
        if (nb_padding < 0)
            nb_padding = 0;
            
        if (arg_pos < config->nb_pos_args_required)
            printf("    <%s> %*s %s\n", arg->name, nb_padding, "", arg->help);
        else
            printf("    [%s] %*s %s\n", arg->name, nb_padding, "", arg->help);
        ++arg_pos;
    }
    
    puts("\n  Options:");
    int short_prefix_length = strlen(config->short_options_prefix);
    int long_prefix_length = strlen(config->options_prefix);
    
    printf("    --jvcmd%*s License attribution for the jvcmd library.\n", help_name_padding-7, "");
    if (!config->no_help)
        printf("    --help%*s Show this message.\n", help_name_padding-6, "");
    
    
    jvArgument* const* options = config->options;
    for (jvArgument* option; (option = *options) != NULL; ++options) {
        fputs("    ", stdout);
        
        int nb_padding = help_name_padding;
        
        if (!option->required) {
            fputs("[", stdout);
            nb_padding -= 1;
        }
        
        printf("%s%s", config->options_prefix, option->name);
        nb_padding -= long_prefix_length + strlen(option->name);
        
        if (config->short_options_prefix[0] != '\0' && option->short_name != '\0') {
            printf("|%s%c", config->short_options_prefix, option->short_name);
            nb_padding -= short_prefix_length + 2;
        }
        
        if (option->need_value) {
            fputs(" ...", stdout);
            nb_padding -= 4;
        }
            
        if (!option->required) {
            fputs("]", stdout);
            nb_padding -= 1;
        }
        
        if (nb_padding < 0)
            nb_padding = 0;
        printf("%*s %s\n", nb_padding, "", option->help);
    }
    
    if (config->epilog != NULL)
        puts(config->epilog);
    
    exit(0);
}

/* Print the error (formatted with printf) to stderr and then call exit(1) */
void jvcmd_exit_with_error(jvParsingConfig const* config, char const* fmt, ...) {
    fprintf(stderr, "ERROR!\n");
    print_usage(stderr, config);
    
    va_list vlist;
    va_start(vlist, fmt);
    vfprintf(stderr, fmt, vlist);
    va_end(vlist);
    
    if (!config->no_help) {
        fprintf(stderr, "\nType '%s --help' for more information.", config->program_name);
    }
    fputs("\n", stderr);
    exit(1);
}


// Returns number of argv used, 'argv[0]' and 'argv[1]' must be defined
static int check_long_options(char** argv, StrView prefix, jvParsingConfig const* config) {
    StrView arg = StrView_make(argv[0]);
    if (!jvstr_starts_with(arg, prefix, 0))
        return 0;
    jvstr_split(&arg, 0, prefix.size); // discard prefix
    
    if (jvstr_equal(arg, STRVIEW_MAKE("jvcmd"))) {
        puts("Copyright (c) 2021 Julien Vernay ( jvernay.fr )");
        puts("This program uses jvcmd, a MIT-licensed C library, for its command-line interface.");
        puts("jvcmd repository: https://github.com/J-Vernay/jvcmd");
        exit(0);
    }
    
    if (!config->no_help && jvstr_equal(arg, STRVIEW_MAKE("help")))
        jvcmd_exit_with_help(config);
    
    jvArgument* const* options = config->options;
    for (jvArgument* option; (option = *options) != NULL; ++options) {
        if (!jvstr_equal(arg, StrView_make(option->name)))
            continue;
        option->specified = 1;
        if (option->need_value) {
            if (argv[1] == NULL)
                jvcmd_exit_with_error(config, "No value provided for option: %s", argv[0]);
            option->value = argv[1];
            return 2;
        } else {
            option->value = "";
            return 1;
        }
    }
    jvcmd_exit_with_error(config, "Unknown option: %s", argv[0]);
    return 0; // never reached, but compiles don't know that.
}

// Returns number of argv used, 'argv[0]' and 'argv[1]' must be defined
static int check_short_options(char** argv, StrView prefix, jvParsingConfig const* config) {
    StrView arg = StrView_make(argv[0]);
    if (!jvstr_starts_with(arg, prefix, 0))
        return 0;
    jvstr_split(&arg, 0, prefix.size); // discard prefix
    
    char c;
    bool chained_short_names = false;
    while (arg.size > 0) {
        char c = arg.begin[0];
        
        if (!config->no_help && c == 'h')
            jvcmd_exit_with_help(config);
        
        jvArgument* const* options = config->options;
        jvArgument* option;
        for (; (option = *options) != NULL; ++options) {
            if (c != option->short_name)
                continue;
            jvstr_split(&arg, 0, 1); // remove short name (= 1 char)
            
            option->specified = true;
            if (option->need_value) {
                if (chained_short_names)
                    jvcmd_exit_with_error(config, "%s%c requires a value, so it cannot be used in group, but you entered: %s",
                                                  config->short_options_prefix, c, argv[0]);
                if (arg.size > 0) { // current short_name was already removed with previous jvstr_split */
                    option->value = arg.begin;
                    return 1;
                } else { // no remaining chars in current argv, using next argv (i.e. -L /usr/lib )
                    if (argv[1] == NULL)
                        jvcmd_exit_with_error(config, "No value provided for option: %s", argv[0]);
                    option->value = argv[1];
                    return 2;
                }
            } else {
                option->value = "";
                chained_short_names = true;
                break; // continue with next char of arg (i.e. -xcf being equivalent to -x -c -f)
            }
        }
        if (option == NULL) { // all options were compared, and none has matched
            // unkown short argument
            jvcmd_exit_with_error(config, "Unknown option: %s%c in %s", config->short_options_prefix, c, argv[0]);
        }
    }
    return 1;
}


static int check_options(char** argv, StrView short_prefix, StrView long_prefix, jvParsingConfig* config) {
    int nb_argv_consumed = 0;
    if (long_prefix.size > 0) {
        nb_argv_consumed = check_long_options(argv, long_prefix, config);
        if (nb_argv_consumed > 0) 
            return nb_argv_consumed;
    }
    if (short_prefix.size > 0) {
        nb_argv_consumed = check_short_options(argv, short_prefix, config);
        if (nb_argv_consumed > 0) 
            return nb_argv_consumed;
    }
    return 0;
}

static bool is_in_space_delimited_list(StrView value, char const* values) {
    StrView values_view = StrView_make(values);
    do {
        // iterating over values
        StrView v = jvstr_split(&values_view, jvstr_find(values_view, ' '), 1);
        if (jvstr_equal(value, v))
            return true;
    } while (values_view.size > 0);
    return false;
}

static void for_all_arguments(jvParsingConfig* config, void(*func)(jvParsingConfig* config, jvArgument* arg)) {
    jvArgument* const* options = config->options;
    for (jvArgument* option; (option = *options) != NULL; ++options)
        (*func)(config, option);
    jvArgument* const* pos_args = config->pos_args;
    for (jvArgument* arg; (arg = *pos_args) != NULL; ++pos_args)
        (*func)(config, arg);
}


static void check_convert_value(jvParsingConfig* config, jvArgument* arg) {
    if (!arg->specified) {
        if (arg->need_value && arg->default_value != NULL) {
            arg->value = arg->default_value;
            arg->specified = true;
        } else if (arg->required) {
            jvcmd_exit_with_error(config, "Option '%s%s' is required but you did not specify it.", config->options_prefix, arg->name);
        }else {
            return;
        }
    }
    if (arg->need_value) {
        if (arg->allowed_values) {
            StrView values = StrView_make(arg->allowed_values);
            bool is_allowed = is_in_space_delimited_list(StrView_make(arg->value), arg->allowed_values);
            
            if (!is_allowed)
                jvcmd_exit_with_error(config, "Invalid value for option '%s%s', '%s' is not in '%s'.",
                                          config->options_prefix, arg->name, arg->value, arg->allowed_values);
        }
        char const* begin = arg->value;
        errno = 0;
        if (arg->is_int) {
            int int_min = arg->int_min, int_max = arg->int_max;
            if (int_min == int_max) {
                int_min = INT_MIN; int_max = INT_MAX;
            }
            char* end;
            long value = strtol(begin, &end, 0);
            if (begin == end)
                jvcmd_exit_with_error(config, "Invalid value for option '%s%s', '%s' is not an integer.",
                                          config->options_prefix, arg->name, arg->value);
            if (errno == ERANGE || value > int_max || value < int_min)
                jvcmd_exit_with_error(config, "Invalid value for option '%s%s', '%s' is out of range. (min value: %d, max value: %d)",
                                          config->options_prefix, arg->name, arg->value, int_min, int_max);
            arg->as_int = (int)value;
        }
        if (arg->is_float) {
            char* end;
            float value = strtof(begin, &end);
            // if either limits or value is NaN, error
            if (!(arg->float_min == arg->float_max) && (value < arg->float_min || value > arg->float_max)) 
                jvcmd_exit_with_error(config, "Invalid value for option '%s%s', '%s' is out of range. (min value: %f, max value: %f)",
                                          config->options_prefix, arg->name, arg->value, arg->float_min, arg->float_max);
            arg->as_float = value;
        }
        if (arg->is_bool) {
            bool is_false = is_in_space_delimited_list(StrView_make(arg->value), config->false_synonyms);
            bool is_true = is_in_space_delimited_list(StrView_make(arg->value), config->true_synonyms);
            
            if (!is_false && !is_true) {
                jvcmd_exit_with_error(config, "Invalid value for option %s%s, '%s' is not a boolean. (accepted: %s %s)",
                                          config->options_prefix, arg->name, arg->value, config->true_synonyms, config->false_synonyms);
            }
            arg->as_bool = is_true;
        }
    }
    if (arg->action) {
        arg->action(config, arg);
    }
}


static void set_actual_need_value(jvParsingConfig* config, jvArgument* arg) {
    arg->need_value = arg->need_value || arg->is_int || arg->is_float || arg->is_bool || (arg->allowed_values != NULL);
}

void jvcmd_parse_arguments(int argc, char** argv, jvParsingConfig config) {
    if (config.program_name == NULL) {
        config.program_name = argv[0];
        argc -= 1;
        argv += 1;
    }
    
    SET_IF_NULL(config.short_options_prefix, "-");
    SET_IF_NULL(config.options_prefix, "--");
    SET_IF_NULL(config.no_more_options, "--");
    SET_IF_NULL(config.true_synonyms, "1 true True TRUE y Y yes Yes YES");
    SET_IF_NULL(config.false_synonyms, "0 false False FALSE n N no No NO");
    
    StrView short_opt_prefix = StrView_make(config.short_options_prefix);
    StrView opt_prefix = StrView_make(config.options_prefix);
    StrView no_more_options = StrView_make(config.no_more_options);
    
    int nb_pos_args_total = 0;
    if (config.pos_args != NULL)
        while (config.pos_args[nb_pos_args_total] != NULL)
            ++nb_pos_args_total;
            
    for_all_arguments(&config, &set_actual_need_value);
    
    int argument_pos = 0;
    bool no_more_options_encountered = false;
    for (int i = 0; i < argc;) {
        int nb_argv_consumed = 0;
        if (!no_more_options_encountered) {
            if (jvstr_equal(StrView_make(argv[i]), no_more_options)) {
                no_more_options_encountered = true;
                nb_argv_consumed = 1;
            } else {
                nb_argv_consumed = check_options(argv + i, short_opt_prefix, opt_prefix, &config);
            }
        }
    
        if (nb_argv_consumed == 0) { 
            // checking positional argument
            if (argument_pos >= nb_pos_args_total) { // no positional arguments were expected
                if (config.action_extra_value == NULL)
                    jvcmd_exit_with_error(&config, "Only %d positional arguments are accepted, but you gave '%s'", nb_pos_args_total, argv[i]);
                config.action_extra_value(argv[i], config.userdata);
            } else {
                config.pos_args[argument_pos]->specified = true;
                config.pos_args[argument_pos]->value = argv[i];
            }
            nb_argv_consumed = 1;
            ++argument_pos;
            if (config.stops_at_last_pos && argument_pos == nb_pos_args_total)
                break; // stop here
        }
        i += nb_argv_consumed;
    }
    
    if (argument_pos < config.nb_pos_args_required)
        jvcmd_exit_with_error(&config, "At least %d positional arguments are required, but you gave %d arguments.", config.nb_pos_args_required, argument_pos);
    
    jvArgument* const* options = config.options;
    for (jvArgument* option; (option = *options) != NULL; ++options)
        check_convert_value(&config, option);
        
    config.options_prefix = ""; // remove option prefix for positional arguments error messages.
    jvArgument* const* pos_args = config.pos_args;
    for (jvArgument* arg; (arg = *pos_args) != NULL; ++pos_args)
        check_convert_value(&config, arg);
}


void jvcmd_discard_extra_values(char const* extra_value, void* userdata) {
    (void)extra_value; (void)userdata;
}



