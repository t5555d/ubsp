#include "syntax.h"
#include "syntax-output.h"
#include "syntax-analyze.h"
#include "bitstream.h"
#include "machine.h"
#include "error.h"
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

    if (0 == strcmp(command, "analyze")) {
        syntax_t syntax;
        syntax.find_modules(argv[0]);
        syntax.load(argv[2]);
        try {
            syntax_analyzer_t analyzer(syntax);
            analyzer.analyze();
            analyzer.print_variables(std::cout);
        }
        catch (const error_t& e) {
            e.explain(std::cerr);
        }
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
    machine.export_native_module("nalu", nalu_stream, nalu_stream_t::export_table);
    machine.export_native_module("rbsp", rbsp_stream, rbsp_stream_t::export_table);
    machine.export_native_module("math", native_math);

    try {
        syntax.find_modules(argv[0]);
        syntax.load(module);
        syntax.analyze();
        machine.execute();
    }
    catch (const error_t& e) {
        e.explain(std::cerr);
    }

    return 0;
}
