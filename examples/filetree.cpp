#include "../jvcmd/jvcmd.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void check_file_exists(jvParsingConfig* config, jvArgument* arg) {
    if (!fs::is_directory(arg->value))
        jvcmd_exit_with_error(config, "Invalid value for option '%s': '%s' is not a path to a directory.", arg->name, arg->value);
}


int main(int argc, char** argv) {

    jvArgument full_path       = { "full-path", "Print full path.", 'f' };
    
    jvArgument follow_symlinks = { "follow-symlink", "Follow symbolic links for directories." };
    follow_symlinks.is_bool = true;
    follow_symlinks.default_value = "false";
    
    jvArgument max_depth       = { "max-depth", "How much the iteration can be nested.", 'L' };
    max_depth.is_int = true;
    max_depth.int_min = 1;
    max_depth.int_max = 50;
    max_depth.default_value = "5";
    
    jvArgument root_directory  = { "root", "Root directory to be iterated over." };
    root_directory.need_value = true;
    root_directory.default_value = ".";
    root_directory.action = check_file_exists;
                                 
    
    jvArgument* options[] = { &follow_symlinks, &full_path, &max_depth, NULL };
    jvArgument* pos_args[] = { &root_directory, NULL };
    
    jvcmd_parse_arguments(argc, argv, jvParsingConfig{
        .description = "Iterate recursively over a directory and print its files.",
        .options = options,
        .pos_args = pos_args,
        .nb_pos_args_required = 0,
    });
    
    fs::path root = fs::absolute(root_directory.value).lexically_normal();
    
    auto dir_option = (follow_symlinks.as_bool ? fs::directory_options::follow_directory_symlink : fs::directory_options::none);
    fs::recursive_directory_iterator iter = { root, dir_option }, end = {};
    
    std::cout << root.string() << '\n';
    while (iter != end) {
        int current_depth = iter.depth();
        if (current_depth >= max_depth.as_int) {
            iter.pop(); // skip this directory because too nested
            continue;
        }
        for (int i = 0; i < current_depth; ++i) {
            std::cout << "    ";
        }
        if (full_path.specified)
            std::cout << "  - " << iter->path().string();
        else
            std::cout << "  - " << fs::relative(iter->path(), root).string();
        if (iter->is_directory())
            std::cout << "/";
        std::cout << "\n";
        ++iter;
    }

    return 0;
}
