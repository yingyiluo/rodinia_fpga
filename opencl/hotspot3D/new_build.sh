#!/bin/bash
echo v0
aoc hotspot3D_kernel_v0.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_kernel_v0.aocx &
echo v1
aoc hotspot3D_kernel_v1.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_kernel_v1.aocx &
echo v2
aoc hotspot3D_kernel_v2.cl -g -v --report --board p385a_sch_ax115  -DSSIZE=4 -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_kernel_v2.aocx &
echo v3
aoc hotspot3D_kernel_v3.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_kernel_v3.aocx &
echo v5
aoc hotspot3D_kernel_v5.cl -g -v --report --board p385a_sch_ax115  -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot3D_kernel_v5.aocx &
