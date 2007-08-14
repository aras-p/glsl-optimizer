#ifndef SP_TEX_SAMPLE_H
#define SP_TEX_SAMPLE_H


struct tgsi_sampler;


extern void
sp_get_samples(struct tgsi_sampler *sampler,
               const GLfloat s[QUAD_SIZE],
               const GLfloat t[QUAD_SIZE],
               const GLfloat p[QUAD_SIZE],
               float lodbias,
               GLfloat rgba[NUM_CHANNELS][QUAD_SIZE]);


#endif /* SP_TEX_SAMPLE_H */
