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

    const char *command = argv[1];
    if (0 == strcmp(command, "format")) {
        syntax_t syntax;
        syntax.find_modules(argv[0]);
        syntax.load(argv[2]);
        syntax_output_t output(std::cout);
        syntax.process(output);
        return 0;
    }

    if (0 != strcmp(command, "parse")) {
        std::cerr << "Unknown command: " << command << std::endl;
        return -1;
    }

    const char *stream_path = argv[2];

    std::basic_ifstream<byte_t> byte_stream;
    byte_stream.open(stream_path, std::ifstream::binary | std::ifstream::in);
    if (!byte_stream.good()) {
        std::cerr << "Failed to open file " << stream_path << std::endl;
        return -1;
    }

    const char *module = strrchr(stream_path, '.');
    if (module == nullptr) {
        std::cerr << "Can not extract extension from " << stream_path << std::endl;
        return 2;
    }
    module++;

    nalu_stream_t nalu_stream(byte_stream);
    rbsp_stream_t rbsp_stream(nalu_stream);

    syntax_t syntax;
    machine_t machine(syntax);
    machine.export_native_object("nalu", nalu_stream, nalu_stream_t::export_table);
    machine.export_native_object("rbsp", rbsp_stream, rbsp_stream_t::export_table);
    machine.export_native_object("math", native_math);

    try {
        syntax.find_modules(argv[0]);
        syntax.load(module);
        machine.execute();
    }
    catch (undef_module_error e) {
        std::cerr << "Undefined module: " << e.name << std::endl;
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
