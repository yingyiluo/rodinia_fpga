#!/bin/bash
echo v2
aoc hotspot_kernel_v2.cl -g -v --report --board p385a_sch_ax115 -DFP_SINGLE -DBSIZE=128 -DSSIZE=16 -DALTERA -DUSE_AOT -o hotspot_kernel_v2.aocx &
echo v3
aoc hotspot_kernel_v3.cl -g -v --report --board p385a_sch_ax115 -DFP_SINGLE -DSSIZE=16 -DALTERA -DUSE_AOT -o hotspot_kernel_v3.aocx &
echo v4
aoc hotspot_kernel_v4.cl -g -v --report --board p385a_sch_ax115 -DFP_SINGLE -DSSIZE=16 -DUNROLL=4 -DALTERA -DUSE_AOT -o hotspot_kernel_v4.aocx &
echo v5
aoc hotspot_kernel_v5.cl -g -v --report --board p385a_sch_ax115 -DFP_SINGLE -DBSIZE=512 -DSSIZE=16 -DALTERA -DUSE_AOT -o hotspot_kernel_v5.aocx &
echo v1
aoc hotspot_kernel_v1.cl -g -v --report --board p385a_sch_ax115 -DFP_SINGLE -DALTERA -DUSE_AOT -o hotspot_kernel_v1.aocx 
