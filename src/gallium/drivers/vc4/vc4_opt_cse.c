/*
 * Copyright © 2014 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file vc4_opt_cse.c
 *
 * Implements CSE for QIR without control flow.
 *
 * For each operation that writes a destination (and isn't just a MOV), put it
 * in the hash table of all instructions that do so.  When faced with another
 * one, look it up in the hash table by its opcode and operands.  If there's
 * an entry in the table, then just reuse the entry's destination as the
 * source of a MOV instead of reproducing the computation.  That MOV will then
 * get cleaned up by copy propagation.
 */

#include "vc4_qir.h"

#include "util/hash_table.h"
#include "util/ralloc.h"

static bool debug;

struct inst_key {
        enum qop op;
        struct qreg src[4];
        /**
         * If the instruction depends on the flags, how many QOP_SFs have been
         * seen before this instruction, or if it depends on r4, how many r4
         * writes have been seen.
         */
        uint32_t implicit_arg_update_count;
};

static bool
inst_key_equals(const void *a, const void *b)
{
        const struct inst_key *key_a = a;
        const struct inst_key *key_b = b;

        return memcmp(key_a, key_b, sizeof(*key_a)) == 0;
}

static struct qinst *
vc4_find_cse(struct hash_table *ht, struct qinst *inst, uint32_t sf_count,
             uint32_t r4_count)
{
        if (inst->dst.file != QFILE_TEMP ||
            inst->op == QOP_MOV ||
            qir_get_op_nsrc(inst->op) > 4) {
                return NULL;
        }

        struct inst_key key;
        memset(&key, 0, sizeof(key));
        key.op = inst->op;
        memcpy(key.src, inst->src,
               qir_get_op_nsrc(inst->op) * sizeof(key.src[0]));
        if (qir_depends_on_flags(inst))
                key.implicit_arg_update_count = sf_count;
        if (qir_reads_r4(inst))
                key.implicit_arg_update_count = r4_count;

        uint32_t hash = _mesa_hash_data(&key, sizeof(key));
        struct hash_entry *entry =
                _mesa_hash_table_search(ht, hash, &key);

        if (entry) {
                if (debug) {
                        fprintf(stderr, "CSE found match:\n");

                        fprintf(stderr, "  Original inst: ");
                        qir_dump_inst(entry->data);
                        fprintf(stderr, "\n");

                        fprintf(stderr, "  Our inst:      ");
                        qir_dump_inst(inst);
                        fprintf(stderr, "\n");
                }

                return entry->data;
        }

        struct inst_key *alloc_key = ralloc(ht, struct inst_key);
        if (!alloc_key)
                return NULL;
        memcpy(alloc_key, &key, sizeof(*alloc_key));
        _mesa_hash_table_insert(ht, hash, alloc_key, inst);

        if (debug) {
                fprintf(stderr, "Added to CSE HT: ");
                qir_dump_inst(inst);
                fprintf(stderr, "\n");
        }

        return NULL;
}

bool
qir_opt_cse(struct vc4_compile *c)
{
        bool progress = false;
        struct simple_node *node, *t;
        struct qinst *last_sf = NULL;
        uint32_t sf_count = 0, r4_count = 0;

        return false;
        struct hash_table *ht = _mesa_hash_table_create(NULL, inst_key_equals);
        if (!ht)
                return false;

        foreach_s(node, t, &c->instructions) {
                struct qinst *inst = (struct qinst *)node;

                if (qir_has_side_effects(inst)) {
                        if (inst->op == QOP_TLB_DISCARD_SETUP)
                                last_sf = NULL;
                        continue;
                }

                if (inst->op == QOP_SF) {
                        if (last_sf &&
                            qir_reg_equals(last_sf->src[0], inst->src[0])) {
                                if (debug) {
                                        fprintf(stderr,
                                                "Removing redundant SF: ");
                                        qir_dump_inst(inst);
                                        fprintf(stderr, "\n");
                                }
                                remove_from_list(&inst->link);
                                progress = true;
                                continue;
                        } else {
                                last_sf = inst;
                                sf_count++;
                        }
                } else {
                        struct qinst *cse = vc4_find_cse(ht, inst,
                                                         sf_count, r4_count);
                        if (cse) {
                                inst->src[0] = cse->dst;
                                for (int i = 1; i < qir_get_op_nsrc(inst->op);
                                     i++)
                                        inst->src[i] = c->undef;
                                inst->op = QOP_MOV;
                                progress = true;

                                if (debug) {
                                        fprintf(stderr, "  Turned into:   ");
                                        qir_dump_inst(inst);
                                        fprintf(stderr, "\n");
                                }
                        }
                }

                if (qir_writes_r4(inst))
                        r4_count++;
        }

        ralloc_free(ht);

        return progress;
}
