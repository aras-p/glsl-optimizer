#ifndef vl_bitstream_parser_h
#define vl_bitstream_parser_h

#include "pipe/p_compiler.h"

struct vl_bitstream_parser
{
   unsigned num_bitstreams;
   const unsigned **bitstreams;
   const unsigned *sizes;
   unsigned cur_bitstream;
   unsigned cursor;
};

bool vl_bitstream_parser_init(struct vl_bitstream_parser *parser,
                              unsigned num_bitstreams,
                              const void **bitstreams,
                              const unsigned *sizes);

void vl_bitstream_parser_cleanup(struct vl_bitstream_parser *parser);

unsigned
vl_bitstream_parser_get_bits(struct vl_bitstream_parser *parser,
                             unsigned how_many_bits);

unsigned
vl_bitstream_parser_show_bits(struct vl_bitstream_parser *parser,
                              unsigned how_many_bits);

void vl_bitstream_parser_forward(struct vl_bitstream_parser *parser,
                                 unsigned how_many_bits);

void vl_bitstream_parser_rewind(struct vl_bitstream_parser *parser,
                                unsigned how_many_bits);

#endif /* vl_bitstream_parser_h */
