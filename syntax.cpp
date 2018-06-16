#include <stdio.h>
#include "syntax.h"
#include "syntax-loader.h"

NAMESPACE_UBSP_BEGIN;

syntax_t::syntax_t()
{
    last = &root;
    first_free = nullptr;
}

void syntax_t::load(const char *syntax_file)
{
    syntax_loader_t runtime(*this, syntax_file);
    runtime.parse();
}

NAMESPACE_UBSP_END;