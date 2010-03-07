# tgsi.py
# Sphinx extension providing formatting for TGSI opcodes
# (c) Corbin Simpson 2010

import docutils.nodes
import sphinx.addnodes

def parse_opcode(env, sig, signode):
    opcode, desc = sig.split("-", 1)
    opcode = opcode.strip().upper()
    desc = " (%s)" % desc.strip()
    signode += sphinx.addnodes.desc_name(opcode, opcode)
    signode += sphinx.addnodes.desc_annotation(desc, desc)
    return opcode

def setup(app):
    app.add_description_unit("opcode", "opcode", "%s (TGSI opcode)", parse_opcode)
