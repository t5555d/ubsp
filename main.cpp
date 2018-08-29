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

    nalu_stream_t nalu_stream(byte_stream);
    rbsp_stream_t rbsp_stream(nalu_stream);

    machine_t machine;
    machine.export_native_object("nalu", nalu_stream, nalu_stream_t::export_table);
    machine.export_native_object("rbsp", rbsp_stream, rbsp_stream_t::export_table);
    machine.export_native_object("math", native_math);

    try {
        machine.load(syntax);
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
    catch (infer_var_error e) {
        std::cerr << "Failed to infer variable: " << e.name << std::endl;
    }
    catch (wrong_index_error e) {
        std::cerr << "Wrong index on dimension " << e.dim << ": " << e.index << ">= " << e.size << std::endl;
    }
    catch (const char *e) {
        std::cerr << "Error: " << e << std::endl;
    }

    return 0;
}
