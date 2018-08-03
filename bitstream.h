#ifndef _UBSP_BITSTREAM_
#define _UBSP_BITSTREAM_

#include "ubsp-fwd.h"
#include "native.h"
#include <istream>

NAMESPACE_UBSP_BEGIN;

class input_stream_t
{
    typedef int64_t length_t;
public:
    static const export_record_t<input_stream_t> export_table[];

    input_stream_t(std::istream& in) :
        input(in), buffer(0), pos(0) {}

    length_t get_position() const { return pos; }
    uint64_t read_bits(length_t length);
    void byte_align();

private:
    static number_t exp_get_position(input_stream_t *in, int nargs, number_t args[MAX_ARGS]);
    static number_t exp_read_bits(input_stream_t *in, int nargs, number_t args[MAX_ARGS]);
    static number_t exp_byte_align(input_stream_t *in, int nargs, number_t args[MAX_ARGS]);

private:
    typedef uint64_t buffer_t;
    static constexpr length_t BUFFER_BYTES = sizeof(buffer_t);
    static constexpr length_t BUFFER_BITS = sizeof(buffer_t) * 8;
    static constexpr length_t BUFFER_MASK = BUFFER_BITS - 1;

    std::istream& input;
    buffer_t buffer;
    length_t pos;
};

NAMESPACE_UBSP_END;

#endif // _UBSP_BITSTREAM_