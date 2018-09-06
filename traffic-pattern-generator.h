//
//  traffic-pattern-generator.h
//
//  Sep-06-2018     huyao@nii.ac.jp
//
//  This file is the head file for traffic_pattern_generator.cc.

#ifndef _TPG_H_
#define _TPG_H_

#include <iostream>
#include <iomanip>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#include <list>
#include <vector>

// Linux(x86)
#if defined(__linux__)
#define SYSTEM "Linux"
#include <time.h>
#include <unistd.h>
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0
#endif

#ifndef VERSION
#define VERSION		__DATE__
#endif

#define TYPE_DATA 2

#define	POW(x,y)	int(pow((double)(x),(double)(y)))

#endif /* _TPG_H_ */