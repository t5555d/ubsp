#include "bitstream.h"

NAMESPACE_UBSP_BEGIN;

uint64_t input_stream_t::read_bits(length_t len)
{
    if (len <= 0) throw "Illegal argument to read_bits";
    if (len > BUFFER_BITS) throw "Can't read more than 64 bits at once";

    // read from stream if necessary
    length_t buf_shift = BUFFER_MASK & pos;
    length_t buf_bits = BUFFER_MASK & -pos;
    uint64_t result = buf_bits > 0 ? buffer >> buf_shift : 0;
    if (buf_bits < len) {
        input.read(reinterpret_cast<char *>(&buffer), BUFFER_BYTES);
        result |= buffer << buf_bits;
    }
    result &= (1 << len) - 1;
    pos += len;
    return result;
}

void input_stream_t::byte_align()
{
    pos = (pos + 7) & -8LL;
}

//
// export
//

const export_record_t<input_stream_t> input_stream_t::export_table[] = {
    { "read_bits", input_stream_t::exp_read_bits },
    { "get_position", input_stream_t::exp_get_position },
    { "byte_align", input_stream_t::exp_byte_align },
    { nullptr }
};

number_t input_stream_t::exp_read_bits(input_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 1) throw wrong_argc_error{ "read_bits", 1, argc };
    return in->read_bits(argv[0]);
}

number_t input_stream_t::exp_get_position(input_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 0) throw wrong_argc_error{ "get_position", 0, argc };
    return in->get_position();
}

number_t input_stream_t::exp_byte_align(input_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 0) throw wrong_argc_error{ "byte_align", 0, argc };
    in->byte_align();
    return 0;
}

NAMESPACE_UBSP_END;