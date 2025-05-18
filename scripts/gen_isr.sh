#!/bin/bash

set -e

if [ $# -le 1 ]; then
    echo "Syntax: gen_isr.sh <isr_gen.c> <isr_gen.inc>"
    exit 1
fi

ISR_GEN_C=$1
ISR_GEN_ASM=$2

ISRS_ERRCODE="8 10 11 12 13 14 17 21 29 30"

echo "// !!! THIS FILE IS AUTO - GENERATED !!!" > $ISR_GEN_C

echo "" >> $ISR_GEN_C

echo "#include <idt.h>" >> $ISR_GEN_C
echo "#include <gdt.h>" >> $ISR_GEN_C

echo "" >> $ISR_GEN_C

for i in $(seq 0 255); do

    echo "void __attribute__((cdecl)) ISR${i}();" >> $ISR_GEN_C

done

echo "" >> $ISR_GEN_C

echo "void ISR_InitializeGates()" >> $ISR_GEN_C
echo "{" >> $ISR_GEN_C

for i in $(seq 0 255); do

    echo "    IDT_SetGate(${i}, ISR${i}, GDT_CODE_SEG, IDT_FLAG_RING0 | IDT_FLAG_GATE_32BIT_INT);" >> $ISR_GEN_C

done

echo "}" >> $ISR_GEN_C

echo "; !!! THIS FILE IS AUTO - GENERATED !!!" > $ISR_GEN_ASM

for i in $(seq 0 255); do
    if echo "$ISRS_ERRCODE" | grep -q "\b${i}\b"; then
        echo "ISR_ERRCODE ${i}" >> $ISR_GEN_ASM
    else
        echo "ISR_NOERRCODE ${i}" >> $ISR_GEN_ASM
    fi
done
