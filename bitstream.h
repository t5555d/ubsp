#ifndef _UBSP_BITSTREAM_
#define _UBSP_BITSTREAM_

#include "ubsp-fwd.h"
#include "native.h"
#include <istream>

NAMESPACE_UBSP_BEGIN;

typedef std::basic_istream<uint8_t> byte_stream_t;

// simple nal unit stream with nal unit navigation
// and emulation prevention bytes processing
class nalu_stream_t
{
public:
    static const export_record_t<nalu_stream_t> export_table[];

    nalu_stream_t(byte_stream_t& bs);
    
    size_t get_position() const { return input.tellg(); }
    
    bool next_nalu();
    bool more_nalu_data() const { return get_position() < nalu_end_pos; }
    size_t skip_nalu_data();
    int read_byte();

private:
    byte_stream_t& input;
    size_t nalu_end_pos = 0;
    int32_t zero_count = 0;

    static number_t exp_next_nalu(nalu_stream_t *in, int argc, number_t argv[MAX_ARGS]);
};

// simple RBSP bitwise stream with look ahead and support for various coding schemes
class rbsp_stream_t
{
public:
    static const export_record_t<rbsp_stream_t> export_table[];

    rbsp_stream_t(nalu_stream_t& bs);

    size_t get_position() const { return bit_pos; }
    bool byte_aligned() const { return (bit_pos & 7) == 0; }

    bool more_rbsp_trailing_data();
    bool more_rbsp_data();
    size_t skip_rbsp_data();

    // sei support:
    void init_sei_payload(size_t payload_size);
    bool more_data_in_payload();
    bool payload_extension_present();

    uint64_t read_bits(int length);
    uint64_t next_bits(int length);

    uint64_t read_unsigned(int length) { return read_bits(length); }
    int64_t read_signed(int length);
    uint64_t read_unsigned_exp_golomb();
    int64_t read_signed_exp_golomb();

private:

    // byte-wise members:
    static constexpr size_t BUFFER_SIZE = 32;
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;
    byte_t buffer[BUFFER_SIZE];

    nalu_stream_t& input;
    size_t read_pos = 0, fill_pos = 0;
    size_t sei_init = 0, sei_size = 0;
    size_t fill_buffer(int count);
    int read_byte();

    // bit-wise members:
    size_t bit_pos = 0;
    byte_t bit_buf = 0;
    int read_exp_golomb_prefix();

    // export functions:
    static number_t exp_read_bits(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_next_bits(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_read_unsigned(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_read_signed(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_read_unsigned_exp_golomb(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_read_signed_exp_golomb(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_get_position(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_byte_aligned(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_more_rbsp_trailing_data(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_more_rbsp_data(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_skip_rbsp_data(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_init_sei_payload(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_more_data_in_payload(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);
    static number_t exp_payload_extension_present(rbsp_stream_t *in, int argc, number_t argv[MAX_ARGS]);

};

NAMESPACE_UBSP_END;

#endif // _UBSP_BITSTREAM_