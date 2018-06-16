#include "syntax.h"
#include "syntax-output.h"
#include <iostream>

int main(int argc, const char *argv[])
{
    using namespace ubsp;

    extern int yydebug;
    //yydebug = 1;

    syntax_t syntax;
    syntax.load(argv[1]);

    syntax_output_t output(std::cout);
    syntax.process(output);

    return 0;
}
