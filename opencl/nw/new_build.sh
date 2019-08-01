#!/bin/bash
echo v0
aoc nw_kernel_v0.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DBSIZE=16 -DALTERA -DUSE_AOT -o nw_kernel_v0.aocx &
echo v2
aoc nw_kernel_v2.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DBSIZE=64 -DALTERA -DUSE_AOT -o nw_kernel_v2.aocx &
echo v3
aoc nw_kernel_v3.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DBSIZE=256 -DALTERA -DUSE_AOT -o nw_kernel_v3.aocx &
echo v5
aoc nw_kernel_v5.cl -g -v --report --board p385a_sch_ax115 -DFP_SINGLE -DBSIZE=128 -DPAR=64 -DALTERA -DUSE_AOT -o nw_kernel_v5.aocx &
