#!/bin/bash
echo v0
aoc hotspot3D_kernel_v0.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_v0_2048_ii.aocx -I ../timer -l timer.aoclib -L ../timer &
echo v1
aoc hotspot3D_kernel_v1.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_v1_2048_ii.aocx -I ../timer -l timer.aoclib -L ../timer &
echo v3
aoc hotspot3D_kernel_v3.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_v3_2048_ii.aocx -I ../timer -l timer.aoclib -L ../timer &








#
#echo v1
#aoc hotspot3D_kernel_v1.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_kernel_v1.aocx -I ../timer -l timer.aoclib -L ../timer
