#include "syntax.h"
#include "syntax-output.h"
#include "bitstream.h"
#include "machine.h"
#include <iostream>
#include <fstream>

int main(int argc, const char *argv[])
{
    using namespace ubsp;

    extern int yydebug;
    //yydebug = 1;

    syntax_t syntax;
    syntax.load(argv[1]);

    std::basic_ifstream<byte_t> byte_stream;
    byte_stream.open(argv[2], std::ifstream::binary | std::ifstream::in);
    if (!byte_stream.good()) {
        std::cerr << "Failed to open file " << argv[2] << std::endl;
        return -1;
    }

    rbsp_stream_t rbsp_stream(byte_stream);

    machine_t machine;
    machine.export_native_object("input", rbsp_stream, rbsp_stream_t::export_table);

    try {
        machine.load(syntax.get_tree_root());
        machine.execute();
    }
    catch (undef_method_error e) {
        std::cerr << "Undefined method: " << e.object << '.' << e.method << std::endl;
    }
    catch (undef_func_error e) {
        std::cerr << "Undefined function: " << e.name << std::endl;
    }
    catch (undef_var_error e) {
        std::cerr << "Undefined variable: " << e.name << std::endl;
    }

    return 0;
}
