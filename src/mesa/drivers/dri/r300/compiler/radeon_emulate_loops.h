

#ifndef RADEON_EMULATE_LOOPS_H
#define RADEON_EMULATE_LOOPS_H

#define MAX_ITERATIONS 8

struct radeon_compiler;

void rc_emulate_loops(struct radeon_compiler *c, unsigned int max_instructions);

#endif /* RADEON_EMULATE_LOOPS_H */
