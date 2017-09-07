//
//  mkpkt.h
//
//  Oct-02-1999     jouraku@am.ics.keio.ac.jp
//
//  $Log: mkpkt.h,v $
//  Revision 1.2  2003/03/29 06:22:12  jouraku
//
//  1. $B6a@\8r?.$N%H%i%U%#%C%/%Q%?!<%s(B 'only_neighbor($B2>(B)'$B$rDI2C(B
//      $B$3$l$O!"(B90%$B$N3NN($GNY@\%N!<%I$,L\E*%N!<%I$H$J$j!";D$j(B10%$B$G!"(B
//      $B%i%s%@%`$KA*Br$5$l$?%N!<%I$,L\E*%N!<%I$H$J$k!#(B
//
//  Revision 1.1.1.1  2001/02/10 13:37:03  jouraku
//
//  1. æÂ‹€Õ— INS(Interconnection Network Simulator)•‚•∏•Â°º•Î
//
//  Revision 1.3  2000/08/21 09:34:01  jouraku
//
//  1. ¿∞øÙ§Œ§Ÿ§≠æË§Úµ·§·§Î•ﬁ•Ø•Ì POW(x,y)§Úƒ…≤√
//
//  Revision 1.2  2000/08/21 07:02:49  jouraku
//
//  1. #include <assert.h>§Úƒ…≤√
//
//  Revision 1.1.1.1  2000/07/20 05:47:11  jouraku
//
//  torus(k-ary n-cube) simulator
//
//  Revision 1.4  1999/12/04 22:19:47  jouraku
//
//  ¥ÿøÙbitreversal§«,•Õ•√•»•Ô°º•Ø•µ•§•∫§ÚMAX_NODE§«§ §ØNpu*Npu
//  §«…Ω§π§≥§»§À§Ë§Í,•Õ•√•»•Ô°º•Ø•µ•§•∫§¨∏«ƒÍ§µ§Ï§Î…‘∂ÒπÁ§ÚΩ§¿µ
//
//  Revision 1.3  1999/12/04 03:08:32  jouraku
//
//  ¥ÿøÙbitreversal§À§Ë§Íµ·§·§È§Ï§Î•«•π•∆•£•Õ°º•∑•Á•Û•Œ°º•…»÷πÊ§¨•Ω°º•π
//  •Œ°º•…»÷πÊ§Œ•”•√•»§Úµ’ΩÁ§À§∑§ø§‚§Œ§À§ §√§∆§§§ §§¥÷∞„§§§ÚΩ§¿µ
//
//  Revision 1.2  1999/10/15 17:13:09  jouraku
//
//   ∏ª˙•≥°º•…§ÚEUC§À —ππ
//

#ifndef _MKPKT_H_
#define _MKPKT_H_

#include <iostream>
#include <iomanip>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#include <list>
#include <vector>

// HP(PA-RISC)
#if defined(__hpux__)
#define SYSTEM "HPUX"
#include <unistd.h>
#include <time.h>

// SunOS 5.x
#elif defined(__sun__) && defined(__svr4__)
#define SYSTEM "SunOS 5.x"
#include <time.h>

// SunOS 4.x
#elif defined(__sun__)
#define SYSTEM "SunOS 4.x"
#include <sys/types.h>
#include <sys/time.h>
extern "C"
{
	void	srand48(long);
	double	drand48();
	int	getopt(int, char**, char*);
	extern	char	*optarg;
	extern	int	optind, opterr;
}

// FreeBSD(x86)
#elif defined(__FreeBSD__)
#define SYSTEM "FreeBSD"
#include <time.h>
#include <unistd.h>  // for 3.x RELEASE

// Linux(x86)
#elif defined(__linux__)
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

#endif /* _MKPKT_H_ */




