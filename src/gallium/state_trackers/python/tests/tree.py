#!/usr/bin/env python
# 
# See also:
#  http://www.ailab.si/orange/doc/ofb/c_otherclass.htm

import os.path
import sys

import orange
import orngTree

for arg in sys.argv[1:]:
    name, ext = os.path.splitext(arg)

    data = orange.ExampleTable(arg)

    tree = orngTree.TreeLearner(data, sameMajorityPruning=1, mForPruning=2)

    orngTree.printTxt(tree)

    file(name+'.txt', 'wt').write(orngTree.dumpTree(tree) + '\n')

    orngTree.printDot(tree, fileName=name+'.dot', nodeShape='ellipse', leafShape='box')
