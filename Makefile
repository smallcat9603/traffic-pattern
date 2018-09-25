#
# Makefile	Sep-25-2018 huyao@nii.ac.jp
#
# GNU make
#
# v1.0

#compiler
CC = g++

#object files
objects = circuit-switch-table.o traffic-pattern-generator.o

all : cst tpg

cst : circuit-switch-table.o 
	${CC} circuit-switch-table.o -o cst.out

tpg : traffic-pattern-generator.o
	${CC} traffic-pattern-generator.o -o tpg.out

traffic-pattern-generator.o : traffic-pattern-generator.h

clean : 
	-rm ${objects}

