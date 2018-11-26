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
    char syntax_file[MAX_PATH];
    sprintf(syntax_file, "%s/%s.cfg", modules_path, module);
    syntax_loader_t runtime(*this, syntax_file, &root);
    name_t module_id = get_ident(module);
    if (modules.find(module_id) == modules.end()) {
        modules.insert(module_id);
        runtime.parse();
    }
}

void syntax_t::load(const import_decl_t& import)
{
    char syntax_file[MAX_PATH];
    sprintf(syntax_file, "%s/%s.cfg", modules_path, import.name);
    if (modules.find(import.name) == modules.end()) {
        try {
            syntax_loader_t runtime(*this, syntax_file, &import.root);
            modules.insert(import.name);
            runtime.parse();
        }
        catch (const std::system_error&) {
            throw undef_module_error{ import.name };
        }
    }
}

NAMESPACE_UBSP_END;