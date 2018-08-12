#include "bitstream.h"
#include <intrin.h>

NAMESPACE_UBSP_BEGIN;

//
// RBSP stream (raw byte stream payload)
//

rbsp_stream_t::rbsp_stream_t(byte_stream_t& bs) :
    input(bs)
{
}

size_t rbsp_stream_t::fill_buffer(int count)
{
    size_t buf_size = fill_pos - read_pos;
    if (buf_size >= count)
        return buf_size;

    byte_t buf[BUFFER_SIZE];
    size_t pos1 = input.tellg();
    input.read(buf, BUFFER_SIZE - buf_size);
    size_t pos2 = input.tellg();
    size_t read = pos2 - pos1;

    for (int i = 0; i < read; i++) {
        byte_t b = buf[i];
        if (zero_count < 2 || b != 3)
            buffer[fill_pos++ & BUFFER_MASK] = b;
        zero_count = b == 0 ? zero_count + 1 : 0;
    }
    return fill_pos - read_pos;
}

int rbsp_stream_t::read_byte()
{
    return fill_buffer(1) ? buffer[read_pos++ & BUFFER_MASK] : -1;
}

bool rbsp_stream_t::more_data_in_byte_stream()
{
    return fill_buffer(1) > 0;
}

uint64_t rbsp_stream_t::read_bits(int length)
{
    if (length < 0)
        throw "read_bits with negative (uninitialized) length";
    uint64_t result = 0;

    int count = 0;
    if (bit_pos & 7) {
        count = 8 - (bit_pos & 7);
        int mask = 0xFF >> (bit_pos & 7);
        result = bit_buf & mask;
    }

    while (count < length) {
        int byte = read_byte();
        if (byte < 0)
            throw std::logic_error("Not enough bits in the bitstream");

        bit_buf = byte;
        result = (result << 8) | bit_buf;
        count += 8;
    }

    bit_pos += length;
    result >>= count - length;
    return result;
}

int64_t rbsp_stream_t::read_signed(int length)
{
    uint64_t code = read_bits(length);
    int shift = 64 - length;
    return int64_t(code << shift) >> shift;
}

uint64_t rbsp_stream_t::next_bits(int length)
{
    fill_buffer((length + 7) / 8);
    rbsp_stream_t temp = *this;
    return temp.read_bits(length);
}

bool rbsp_stream_t::more_rbsp_trailing_data()
{
    if (bit_pos & 7) return true; // some bits left
    size_t buf_size = fill_buffer(3);
    if (buf_size == 0) return false; // no bytes left
    if (buf_size < 3) return true; // few bytes left
    bool end = buffer[read_pos & BUFFER_MASK] == 0
            && buffer[(read_pos + 1) & BUFFER_MASK] == 0
            && buffer[(read_pos + 2) & BUFFER_MASK] <= 1;
    return !end;
}

bool rbsp_stream_t::more_rbsp_data()
{
    fill_buffer(4);
    rbsp_stream_t temp = *this;
    byte_t bits = bit_pos ? bit_buf << bit_pos : temp.read_byte();
    if (bits & 0x7F) return true;
    if (temp.read_byte()) return true;
    if (temp.read_byte()) return true;
    return (temp.read_byte() & 0xFE) != 0;
}

int rbsp_stream_t::read_exp_golomb_prefix()
{
    int length = 0;
    while (read_bits(1) == 0)
        length++;
    return length;
}

uint64_t rbsp_stream_t::read_unsigned_exp_golomb()
{
    int length = read_exp_golomb_prefix();
    uint64_t code = read_bits(length);
    return (UINT64_MAX >> (64 - length)) + code;
}

int64_t rbsp_stream_t::read_signed_exp_golomb()
{
    int length = read_exp_golomb_prefix();
    uint64_t code = read_bits(length);
    uint64_t sign = code & 1;
    int64_t value = (INT64_MAX >> (63 - length)) + (code >> 1);
    return sign ? -value : value;
}


//
// export
//

const export_record_t<rbsp_stream_t> rbsp_stream_t::export_table[] = {
    { "read_bits", rbsp_stream_t::exp_read_bits },
    { "next_bits", rbsp_stream_t::exp_next_bits },
    { "read_unsigned", rbsp_stream_t::exp_read_unsigned },
    { "read_signed", rbsp_stream_t::exp_read_signed },
    { "read_unsigned_exp_golomb", rbsp_stream_t::exp_read_unsigned_exp_golomb },
    { "read_signed_exp_golomb", rbsp_stream_t::exp_read_signed_exp_golomb },
    { "get_position", rbsp_stream_t::exp_get_position },
    { "byte_aligned", rbsp_stream_t::exp_byte_aligned },
    { "more_data_in_byte_stream", rbsp_stream_t::exp_more_data_in_byte_stream },
    { "more_rbsp_trailing_data", rbsp_stream_t::exp_more_rbsp_trailing_data },
    { "more_rbsp_data", rbsp_stream_t::exp_more_rbsp_data },
    { nullptr }
};

number_t rbsp_stream_t::exp_read_bits(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 1) throw wrong_argc_error{ "read_bits", 1, argc };
    return in->read_bits((int)argv[0]);
}

number_t rbsp_stream_t::exp_read_unsigned(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 1) throw wrong_argc_error{ "read_unsigned", 1, argc };
    return in->read_unsigned((int)argv[0]);
}

number_t rbsp_stream_t::exp_read_signed(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 1) throw wrong_argc_error{ "read_signed", 1, argc };
    return in->read_signed((int)argv[0]);
}

number_t rbsp_stream_t::exp_read_unsigned_exp_golomb(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc > 1) throw wrong_argc_error{ "read_unsigned_exp_golomb", 0, argc };
    return in->read_unsigned_exp_golomb();
}

number_t rbsp_stream_t::exp_read_signed_exp_golomb(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc > 1) throw wrong_argc_error{ "read_signed_exp_golomb", 0, argc };
    return in->read_signed_exp_golomb();
}

number_t rbsp_stream_t::exp_next_bits(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 1) throw wrong_argc_error{ "next_bits", 1, argc };
    return in->next_bits((int)argv[0]);
}

number_t rbsp_stream_t::exp_get_position(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 0) throw wrong_argc_error{ "get_position", 0, argc };
    return in->get_position();
}

number_t rbsp_stream_t::exp_byte_aligned(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 0) throw wrong_argc_error{ "byte_aligned", 0, argc };
    return in->byte_aligned();
}

number_t rbsp_stream_t::exp_more_data_in_byte_stream(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 0) throw wrong_argc_error{ "more_data_in_byte_stream", 0, argc };
    return in->more_data_in_byte_stream();
}

number_t rbsp_stream_t::exp_more_rbsp_trailing_data(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 0) throw wrong_argc_error{ "more_rbsp_trailing_data", 0, argc };
    return in->more_rbsp_trailing_data();
}

number_t rbsp_stream_t::exp_more_rbsp_data(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS])
{
    if (argc != 0) throw wrong_argc_error{ "more_rbsp_data", 0, argc };
    return in->more_rbsp_data();
}

NAMESPACE_UBSP_END;