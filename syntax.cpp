#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

#include "syntax.h"
#include "syntax-loader.h"

NAMESPACE_UBSP_BEGIN;

syntax_t::syntax_t()
{
    modules_path[0] = '\0';
    first_free = nullptr;
}

void syntax_t::find_modules(const char *start_path)
{
    // TODO cross platform implementation
    char abs_path[MAX_PATH];
    if (0 == GetFullPathNameA(start_path, MAX_PATH, abs_path, NULL))
        throw std::system_error(GetLastError(), std::system_category());

    while (true) {
        char *last_slash = strrchr(abs_path, '\\');
        if (!last_slash) break;
        *last_slash = '\0';

        sprintf(modules_path, "%s/modules", abs_path);

        struct stat info;
        if (stat(modules_path, &info) == 0 && info.st_mode & S_IFDIR)
            return;
    }
    modules_path[0] = '\0';
    throw "Can not find modules path";
}

void syntax_t::load(const char *module)
{
    name_t module_id = get_ident(module);
    if (loaded_modules.find(module_id) != loaded_modules.end())
        return;

    char syntax_path[MAX_PATH];
    sprintf(syntax_path, "%s/%s.cfg", modules_path, module);
    FILE *file = fopen(syntax_path, "r");
    if (file == nullptr)
        throw undef_module_error{ module_id };

    syntax_loader_t runtime(*this, file, &root);
    loaded_modules.insert(module_id);
    runtime.parse();
}

const function_info_t *syntax_t::get_function(name_t name) const
{
    auto it = function_info.find(name);
    return it != function_info.end() ? &it->second : nullptr;
}

const variable_info_t *syntax_t::get_variable(name_t name) const
{
    auto it = variable_info.find(name);
    return it != variable_info.end() ? &it->second : nullptr;
}

NAMESPACE_UBSP_END;