#!/bin/bash
echo v1
aoc hotspot3D_kernel_v1.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_kernel_v1.aocx -I ../timer -l timer.aoclib -L ../timer
# echo v1
# aoc hotspot3D_kernel_v1.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_kernel_v1.aocx &

