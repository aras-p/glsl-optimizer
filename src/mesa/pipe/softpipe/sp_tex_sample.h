#ifndef SP_TEX_SAMPLE_H
#define SP_TEX_SAMPLE_H


struct tgsi_sampler;


extern void
sp_get_samples(struct tgsi_sampler *sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               float lodbias,
               float rgba[NUM_CHANNELS][QUAD_SIZE]);


#endif /* SP_TEX_SAMPLE_H */
