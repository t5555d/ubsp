#include "bitstream.h"
#include <intrin.h>

NAMESPACE_UBSP_BEGIN;

//
// NALU stream (network abstraction layer unit)
//

nalu_stream_t::nalu_stream_t(byte_stream_t& bs) :
    input(bs) 
{
}

bool nalu_stream_t::next_nalu()
{
    // navigate to the end of current NALU
    input.seekg(nalu_end_pos);

    // seek NALU start:
    zero_count = 0;
    while (true) {
        auto byte = input.get();
        if (input.eof()) {
            nalu_end_pos = input.tellg();
            return false;
        }
        if (byte == 1 && zero_count >= 2)
            break;
        zero_count = byte == 0 ? zero_count + 1 : 0;
    }

    size_t nalu_beg_pos = input.tellg();

    // seek NALU end:
    zero_count = 0;
    while (true) {
        auto byte = input.get();
        if (input.eof()) {
            input.clear();
            input.seekg(0, std::ios::end);
            nalu_end_pos = input.tellg();
            break;
        }
        if (byte <= 1 && zero_count >= 2) {
            nalu_end_pos = size_t(input.tellg()) - 3;
            break;
        }
        zero_count = byte == 0 ? zero_count + 1 : 0;
    }

    input.seekg(nalu_beg_pos);
    zero_count = 0;
    return true;
}

int nalu_stream_t::read_byte()
{
    if (!more_nalu_data()) return -1;
    auto byte = input.get();
    if (byte == 3 && zero_count >= 2) {
        zero_count = 0;
        byte = input.get();
    }
    zero_count = byte == 0 ? zero_count + 1 : 0;
    return byte;
}

size_t nalu_stream_t::skip_nalu_data()
{
    size_t init_pos = input.tellg();
    input.seekg(nalu_end_pos);
    return size_t(input.tellg()) - init_pos;
}

//
// RBSP stream (raw byte stream payload)
//

rbsp_stream_t::rbsp_stream_t(nalu_stream_t& bs) :
    input(bs)
{
}

size_t rbsp_stream_t::fill_buffer(int count)
{
    size_t buf_size = fill_pos - read_pos;
    if (buf_size >= count)
        return buf_size;

    while (fill_pos - read_pos < BUFFER_SIZE) {
        auto byte = input.read_byte();
        if (byte < 0) break;
        buffer[fill_pos++ & BUFFER_MASK] = byte;
    }

    return fill_pos - read_pos;
}

int rbsp_stream_t::read_byte()
{
    return fill_buffer(1) ? buffer[read_pos++ & BUFFER_MASK] : -1;
}

uint64_t rbsp_stream_t::read_bits(int length)
{
    if (length < 0)
        throw "read_bits with negative (uninitialized) length";
    if (length == 0)
        return 0;
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
            throw "no more data in the byte stream";

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
    fill_buffer(1);
    return read_pos < fill_pos;
}

bool rbsp_stream_t::more_rbsp_data()
{
    fill_buffer(2);
    if (bit_pos & 7) {
        byte_t bits = bit_buf << (bit_pos & 7);
        return bits != 0x80 || read_pos < fill_pos;
    }
    else {
        byte_t bits = buffer[read_pos & BUFFER_MASK];
        return bits != 0x80 || read_pos + 1 < fill_pos;
    }
}

size_t rbsp_stream_t::skip_rbsp_data()
{
    // flush bit position
    bit_pos = (bit_pos + 7) & ~7;

    // flush buffer
    fill_pos = read_pos = 0;
    sei_init = sei_size = 0;

    return input.skip_nalu_data();
}

void rbsp_stream_t::init_sei_payload(size_t payload_size)
{
    if (!byte_aligned())
        throw "Should be byte aligned at sei_payload";
    sei_init = read_pos;
    sei_size = payload_size;
}

bool rbsp_stream_t::more_data_in_payload()
{
    if (read_pos < sei_init) return false;
    if (read_pos > sei_init + sei_size) return false;
    if (read_pos < sei_init + sei_size) return true;
    return (bit_pos & 7) != 0;
}

bool rbsp_stream_t::payload_extension_present()
{
    fill_buffer(1);
    if (bit_pos & 7) {
        byte_t bits = bit_buf << (bit_pos & 7);
        return bits != 0x80 || read_pos < sei_init + sei_size;
    }
    else {
        byte_t bits = buffer[read_pos & BUFFER_MASK];
        return bits != 0x80 || read_pos + 1 < sei_init + sei_size;
    }
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
    return (1LL << length) - 1 + code;
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

const export_record_t<nalu_stream_t> nalu_stream_t::export_table[] = {
    EXPORT_METHOD(nalu_stream_t, next_nalu),
    { nullptr }
};

const export_record_t<rbsp_stream_t> rbsp_stream_t::export_table[] = {
    EXPORT_METHOD(rbsp_stream_t, read_bits),
    EXPORT_METHOD(rbsp_stream_t, next_bits),
    EXPORT_METHOD(rbsp_stream_t, read_unsigned),
    EXPORT_METHOD(rbsp_stream_t, read_signed),
    EXPORT_METHOD(rbsp_stream_t, read_unsigned_exp_golomb),
    EXPORT_METHOD(rbsp_stream_t, read_signed_exp_golomb),
    EXPORT_METHOD(rbsp_stream_t, get_position),
    EXPORT_METHOD(rbsp_stream_t, byte_aligned),
    EXPORT_METHOD(rbsp_stream_t, more_rbsp_trailing_data),
    EXPORT_METHOD(rbsp_stream_t, more_rbsp_data),
    EXPORT_METHOD(rbsp_stream_t, skip_rbsp_data),
    EXPORT_METHOD(rbsp_stream_t, init_sei_payload),
    EXPORT_METHOD(rbsp_stream_t, more_data_in_payload),
    EXPORT_METHOD(rbsp_stream_t, payload_extension_present),
    { nullptr }
};

NAMESPACE_UBSP_END;