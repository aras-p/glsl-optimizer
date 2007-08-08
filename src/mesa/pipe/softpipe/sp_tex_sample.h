#ifndef SP_TEX_SAMPLE_H
#define SP_TEX_SAMPLE_H


struct tgsi_sampler;


extern void
sp_get_sample(struct tgsi_sampler *sampler,
              const GLfloat strq[4], GLfloat rgba[4]);


#endif /* SP_TEX_SAMPLE_H */
