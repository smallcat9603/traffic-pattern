//
//  mkpkt.cc
//
//  Jan-27-1997	nisimura@aa.cs.keio.ac.jp
//  Oct-02-1999 jouraku@am.ics.keio.ac.jp
//
//  $Id: mkpkt.cc,v 1.6 2004/06/23 08:35:13 jouraku Exp $

#include "mkpkt.h"

using namespace std;

#define TRFC_NUM	10	//$BA*Br2DG=$J%H%i%U%#%C%/%Q%?!<%s$N?t(B //huyao 161228

static	const	char*	SCCSID = "@(#)mkpkt " VERSION " for " SYSTEM;

static	int	Clk	= 0;	//$B%/%m%C%/?t(B
static	int	Dsts = 1;	//$B%G%9%F%#%M!<%7%g%s?t(B
static	int	Node_Num = 1024; //$B%N!<%I?t(B
static	int	Npu	= 32;	//$B%"%l%$%5%$%:(B
static	int	Sgm	= 0;	//$BI8=`JP:9(B
static  int 	Trf = 0; 	//$B%H%i%U%#%C%/%Q%?!<%s(B
static	int	Dimm	= 2; 	//$B<!85?t(B
static	int	Hnode	= 5;	//$B%[%C%H%9%]%C%H%N!<%I$N?t(B
static  vector<int> Hspots;	//$B%[%C%H%9%]%C%H%N!<%IHV9f$r3JG<$9$kG[Ns(B
static	bool	Is_fixed = false;
static	long	Fixed_seed = 6712500;	//$B%[%C%H%9%]%C%H%N!<%I$r7hDj$9$k(Bseed
static  long    Seed = long(time(NULL));
static  int     Data_Size   = 1; // $B%G!<%?%5%$%:(B
static	int	Itvl_Flit = 1000; //$B%U%j%C%H$"$?$j$N@8@.4V3V(B
static  int     Itvl_Pkt  = Itvl_Flit*Data_Size; // $B%Q%1%C%H$"$?$j$N@8@.4V3V(B
static	void	usage(char* myname);
static	void	mkpkt(void);
static	void	uniform(int src, vector<int> &dst);
static	void	matrix(int src, vector<int> &dst);
static	void	shuffle(int src, vector<int> &dst); //huyao161228
static	void	butterfly(int src, vector<int> &dst); //huyao161228
static	void	complement(int src, vector<int> &dst); //huyao161228
static	void	tornado(int src, vector<int> &dst); //huyao161228
static	void	alltoall(int src, vector<int> &dst); //huyao170117
static	int	ata = 0;	//huyao170117
static	void	reversal(int src, vector<int> &dst);
static	void	hotspot(int src, vector<int> &dst);
static  void    only_neighbor(int src, vector<int> &dst);
static	void	mkhotspot(void);

void    (*traffic[10])(int, vector<int>&) = {uniform, matrix, reversal, hotspot, only_neighbor, shuffle, butterfly, complement, tornado, alltoall}; //huyao 170117

static	double	nr(void);


int main(int argc, char *argv[])
{
	int	c;

	while ((c = getopt(argc, argv, "c:d:fj:n:i:a:s:t:vhS:D:")) != EOF)
	{
		switch (c)
		{
		case 'S':
			Seed = atoi(optarg);
			break;
		case 'c':
			Clk = atoi(optarg);
			break;
		case 'f':
			Is_fixed = true;
			break;
		case 'd':
			Dsts = atoi(optarg);
			break;
		case 'D':
			Data_Size = atoi(optarg);
			break;
		case 'i':
			Itvl_Flit = atoi(optarg);
			break;
		case 'j':
			Dimm = atoi(optarg);
			break;
		case 'n':
			Hnode = atoi(optarg);
			break;
		case 'a':
			Npu = atoi(optarg);
			break;
		case 's':
			Sgm = atoi(optarg);
			break;
		case 't':
			Trf = atoi(optarg);
			if (Trf >= TRFC_NUM)
			{
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 'v':
			cerr << "version: " << SCCSID + 4 << endl;
		case 'h':
		case '?':
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}
	if (optind != argc)   //optind$B$O0z?t$N(Bindex
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	//
	// $B0z?t;XDj2DG=$JJQ?t$N=i4|2=(B
	//
	Node_Num = POW(Npu, Dimm);
	Itvl_Pkt  = Itvl_Flit*Data_Size;

	cout << Itvl_Flit << endl;
	mkpkt();

	return EXIT_SUCCESS;
}

void usage(char* myname)
{
	cerr << "usage: " << myname << " [-cdips]" << endl;
	cerr << "\t-c<n>\t: Clock" << endl;
	cerr << "\t-d<n>\t: Destinations" << endl;
	cerr << "\t-i<n>\t: Interval" << endl;
	cerr << "\t-t<n>\t: Traffic Pattern" << endl;
	cerr << "\t-f   \t: use fixed hotspot nodes" << endl;
	cerr << "\t-j<n>\t: set the number of dimension" << endl;
	cerr << "\t-n<n>\t: set the number of hotspot nodes" << endl;
	cerr << "\t-a<n>\t: set the number of array size" << endl;
	cerr << "\t-s<n>\t: Sigma^2" << endl;
}


void mkpkt(void)
{
	vector<int> cnt;
	vector<int> dst;
	int	clk;      //$B%/%m%C%/?t(B
	int	src;      //$B%=!<%9%N!<%I(B
	int	i;        //$B%$%s%?!<%P%k(B

	cout << "# Clk       " << Clk << endl;  //$B%Q%1%C%H$r@8@.$9$k%/%m%C%/?t(B
	cout << "# Dsts      " << Dsts << endl; //$B%G%9%F%#%M!<%7%g%s$N?t(B
	cout << "# Data_Size " << Data_Size << endl; //$B%G!<%?%5%$%:(B
	cout << "# Itvl_Flit " << Itvl_Flit << endl; //$B%U%j%C%H$"$?$j$N@8@.4V3V(B
	cout << "# Itvl_Pkt  " << Itvl_Pkt << endl; //$B%Q%1%C%H$"$?$j$N@8@.4V3V(B
	cout << "# Npu       " << Npu << endl;  //$B3F<!85$N%W%m%;%C%5?t(B
	cout << "# Dimm      " << Dimm << endl; //$B<!85?t(B
	cout << "# Sgm       " << Sgm << endl;  //$BI8=`JP:9(B?
	cout << "# Seed      " << Seed << endl;  //$BI8=`JP:9(B?

	//	srand48((long)time(NULL));
	srand48(Seed);

	cnt.resize(Node_Num);
	dst.resize(Dsts);
#if RAND == 1
	for (i = 0; i < Node_Num; i++)
	{
		cnt[i] = (int)(drand48() * Itvl_Pkt); // drand48$B$O(B[0.0 - 1.0]$B$rJV$9(B
	}
#endif
	if (Trf == 3)
	{
		Hspots.resize(Hnode+1);
		for (int i=0; i < Hnode; i++)
		{
			Hspots[i] = -1;
		}
		if (Is_fixed)
		{
			srand48((long)Fixed_seed);
			mkhotspot();
			srand48(Seed);
			//			srand48((long)time(NULL));
		}
		else
		{
			mkhotspot();
		}
	}

	for (clk = 0; clk < Clk; clk++)
	{
		for (src = 0; src < Node_Num; src++)
		{

#if RAND == 1
			if (cnt[src] == 0)
			{
				// $B%G%9%F%#%M!<%7%g%s$r7hDj(B
				(*traffic[Trf])(src,dst);
				cout << setw(5) << clk;
				cout << " " << setw(5) << src;
				for (i = 0; i < Dsts; i++)
				{
					cout << " " << setw(5) << dst[i];
				}
				//				cout << endl;
				//		cout << " " << TYPE_DATA << endl;
				cout << " " << Data_Size << endl;
				cnt[src] = (int)(drand48() * Itvl_Pkt * 2);
			}
			else
			{
				cnt[src]--;
			}
#else
			if ( (clk%Itvl_Pkt) == 0 || clk == 0)
			{
				if (Trf == 9) //huyao 170117 all to all
				{
					for (int j = 0; j < Node_Num-1; j++)
					{
						(*traffic[Trf])(src,dst);
						cout << setw(5) << clk;
						cout << " " << setw(5) << src;
						for (i = 0; i < Dsts; i++)
						{
							cout << " " << setw(5) << dst[i];
						}
						//				cout << endl;
						//		cout << " " << TYPE_DATA << endl;
						cout << " " << Data_Size << endl;
					}
					
				}
				else
				{
					//$B%G%9%F%#%M!<%7%g%s7hDj(B
					(*traffic[Trf])(src,dst);
					cout << setw(5) << clk;
					cout << " " << setw(5) << src;
					for (i = 0; i < Dsts; i++)
					{
						cout << " " << setw(5) << dst[i];
					}
					//				cout << endl;
					//		cout << " " << TYPE_DATA << endl;
					cout << " " << Data_Size << endl;
				}
			}
#endif
		}
	}
}

/// nonuniform(hot spot) ///
void hotspot(int src, vector<int> &dst)
{
	for (int i = 0; i < Dsts; i++)
	{
		dst[i] = Hspots[(int)( drand48() * (Hnode))];
	}

}

/// nonuniform(perfect shuffle) huyao161228///
void shuffle(int src, vector<int> &dst)
{
	int	dst_node = 0;
	int 	int_length = sizeof(int)*CHAR_BIT;
	int 	bit_length = 1;

	// bit_length depends on Node_Num
	for (int x = 1; x < int_length; x++)
	{
		if ( (pow(2,double(x)) <= Node_Num-1) && (pow(2,double(x+1)) > Node_Num-1) )
			bit_length = x+1;
	}

	// rotate left 1 bit
	dst_node = src << 1;
	if (src >= pow(2,double(bit_length-1)))
	{ 
		dst_node &= int(pow(2,bit_length))-1;
		dst_node++;
	}

	// reverse if source = destination
	if (dst_node == src)
	{
		// reverse
		dst_node = ~dst_node;
		// clear 
		for (int i = int_length -1; i > bit_length-1; i--)
		{
			dst_node ^= (1 << i);
		}
	}

	for (int i = 0; i < Dsts; i++)
	{
		dst[i] = dst_node;
	}
}

/// nonuniform(butterfly) huyao161228///
void butterfly(int src, vector<int> &dst)
{
	int	dst_node = 0;
	int 	int_length = sizeof(int)*CHAR_BIT;
	int 	bit_length = 1;

	// bit_length depends on Node_Num
	for (int x = 1; x < int_length; x++)
	{
		if ( (pow(2,double(x)) <= Node_Num-1) && (pow(2,double(x+1)) > Node_Num-1) )
			bit_length = x+1;
	}

	// swap the most and least significant bits
	//11 or 00 -> 11 or 00 
	if ((src >= pow(2,double(bit_length-1)) && src%2 == 1) || (src < pow(2,double(bit_length-1)) && src%2 == 0))
	{ 
		dst_node = src;
	}
	//10 -> 01
	else if (src >= pow(2,double(bit_length-1)) && src%2 == 0)
	{
		dst_node = src-int(pow(2,bit_length-1))+1;
	}
	//01 -> 10
	else if (src < pow(2,double(bit_length-1)) && src%2 == 1)
	{
		dst_node = src+int(pow(2,bit_length-1))-1;
	}

	// reverse if source = destination
	if (dst_node == src)
	{
		// reverse
		dst_node = ~dst_node;
		// clear 
		for (int i = int_length -1; i > bit_length-1; i--)
		{
			dst_node ^= (1 << i);
		}
	}

	for (int i = 0; i < Dsts; i++)
	{
		dst[i] = dst_node;
	}
}

/// nonuniform(bit complement) huyao161228///
void complement(int src, vector<int> &dst)
{
	int	dst_node = 0;
	int 	int_length = sizeof(int)*CHAR_BIT;
	int 	bit_length = 1;

	// bit_length depends on Node_Num
	for (int x = 1; x < int_length; x++)
	{
		if ( (pow(2,double(x)) <= Node_Num-1) && (pow(2,double(x+1)) > Node_Num-1) )
			bit_length = x+1;
	}

	// reverse
	dst_node = ~src;
	// clear 
	for (int i = int_length -1; i > bit_length-1; i--)
	{
		dst_node ^= (1 << i);
	}

	for (int i = 0; i < Dsts; i++)
	{
		dst[i] = dst_node;
	}
}



/// nonuniform(bit reversal) ///
void reversal(int src, vector<int> &dst)
{
	int	dst_node = 0;
	int 	int_length = sizeof(int)*CHAR_BIT;
	int 	bit_length = 1;

	// $B%N!<%IHV9f$N%S%C%HD9$r%M%C%H%o!<%/%5%$%:$h$j7hDj(B
	for (int x = 1; x < int_length; x++)
	{
		if ( (pow(2,double(x)) <= Node_Num-1) && (pow(2,double(x+1)) > Node_Num-1) )
			bit_length = x+1;
	}

	// $B%S%C%H$r5U=g$K$9$k(B
	for (int i = bit_length - 1,j=0; i >= 0; i--,j++)
	{
		if (src & (1 << i))
			dst_node |= 1 << j;
	}

	// $B%=!<%9$H%G%9%F%#%M!<%7%g%s$,F10l$N>l9g(B
	if (dst_node == src)
	{
		//$B%=!<%9$rH?E>$7$?$b$N$r%G%9%F%#%M!<%7%g%s$K$9$k(B
		dst_node = ~dst_node;
		// $BM>J,$J%S%C%H$rD4@0(B
		for (int i = int_length -1; i > bit_length-1; i--)
		{
			dst_node ^= (1 << i);
		}
	}

	for (int i = 0; i < Dsts; i++)
	{
		dst[i] = dst_node;
	}
}

/// nonuniform(matrix transpose) ///
void matrix(int src, vector<int> &dst)
{
	int	b_coord[Dimm+1]; //$BJQ49A0$N:BI8(B
	int	x,i;
	int 	dst_node = 0;

	// $BJQ49A0$N:BI8$r5a$a$k(B
	for (i=1; i <= Dimm; i++)
	{
		x = src/POW(Npu,i-1);
		if (i != Dimm)
		{
			x %= Npu;
		}
		b_coord[i] = x;
	}

	// $B:BI8JQ49(B
	for (i=1; i <= Dimm; i++)
	{
		x = Npu - 1 - b_coord[Dimm-i+1];
		dst_node += (x % Npu)*POW(Npu,i-1);
	}

	// $BJQ49A0$HJQ498e$N:BI8$,F1$8>l9g$K9T$J$&D4@0(B
	if (src == dst_node)
	{
		dst_node = 0;
		for (i=1; i <= Dimm; i++)
		{
			x = Npu - 1 - b_coord[i];
			dst_node += (x % Npu)*POW(Npu,i-1);
		}
	}

	if (src == dst_node)
	{
		cout << "Coordinate was not changed!!" << endl;
		exit(1);
	}

	for (i = 0; i < Dsts; i++)
	{
		dst[i] = dst_node;
	}
}

/// uniform(random) ///
void uniform(int src, vector<int> &dst)
{
	int	x;
	int	i, j;

	for (i = 0; i < Dsts; i++)
	{
retry:
		if (Sgm == 0)
		{
			dst[i] = (int)(drand48() * Node_Num);
		}
		else
		{
			for (j=1; j <= Dimm; j++)
			{
				x = src/POW(Npu,j-1);
				if (j != Dimm)
				{
					x %= Npu;
				}
				x += (int)(nr()*Sgm);
				while (x < 0)
					x += Npu;
				dst[i] += (x % Npu)*POW(Npu,j-1);
			}
		}
		if (dst[i] == src)
		{
			goto retry;
		}
		for (j = 0; j < i; j++)
		{
			if (dst[i] == dst[j])
			{
				goto retry;
			}
		}
	}
}

double nr(void)
{
	static	int	flag = 0;
	static	double	v1, v2;
	static	double	s;

	if (flag == 0)
	{
		flag = 1;
		do
		{
			v1 = 2 * drand48() - 1;
			v2 = 2 * drand48() - 1;
			s = v1 * v1 + v2 * v2;
		}
		while (s > 1 || s == 0);
		s = sqrt(-2 * log(s) / s);
		return v1 * s;
	}
	else
	{
		flag = 0;
		return v2 * s;
	}
}

void mkhotspot(void)
{
	int count = 0;
	int tmp = 0;
	int i = 0;

	while (count < Hnode)
	{
		tmp = (int)(drand48() * Node_Num);
		for (i = 0; i < count; i++)
		{
			//$B=EJ#%A%'%C%/(B
			if ( Hspots[i] == tmp)
				break;
		}
		if (i == count)
		{
			Hspots[count++] = tmp;
		}
	}
	Hspots[Hnode] = Hspots[0];

	cout << "# HOTSPOT(" << Hnode << ") ";
	for (i = 0; i < Hnode; i++)
	{
		cout <<	Hspots[i] << " ";
	}
	cout << endl;
}

//huyao 161228
void tornado(int src, vector<int> &dst)
{

	int	dst_node = 0;
	dst_node = (src + Node_Num/2-1)%Node_Num;

	for (int i = 0; i < Dsts; i++)
	{
		dst[i] = dst_node;
	}

}

//huyao 170117
void alltoall(int src, vector<int> &dst)
{

	int	dst_node = 0;

	if (ata == src)
	{
		ata++;
	}

	dst_node = ata;
	ata++;
	ata %= Node_Num;

	for (int i = 0; i < Dsts; i++)
	{
		dst[i] = dst_node;
	}



}

void only_neighbor(int src, vector<int> &dst)
{
	list<int> neighbor;

	int tmp = (int)( drand48() * 10);
	//    cout << " tmp : " << tmp << endl;

	if (tmp < 9)
	{
		if ( src < Npu)
		{
			neighbor.push_back(src+Npu);
			if (src%Npu == 0)
			{
				neighbor.push_back(src+1);
			}
			else if (src%Npu == Npu-1)
			{
				neighbor.push_back(src-1);
			}
			else
			{
				neighbor.push_back(src-1);
				neighbor.push_back(src+1);
			}
		}
		else if ( src >= Npu*(Npu-1) )
		{
			neighbor.push_back(src-Npu);
			if (src%Npu == 0)
			{
				neighbor.push_back(src+1);
			}
			else if (src%Npu == Npu-1)
			{
				neighbor.push_back(src-1);
			}
			else
			{
				neighbor.push_back(src-1);
				neighbor.push_back(src+1);
			}
		}
		else
		{
			neighbor.push_back(src-Npu);
			neighbor.push_back(src+Npu);
			if (src%Npu == 0)
			{
				neighbor.push_back(src+1);
			}
			else if (src%Npu == Npu-1)
			{
				neighbor.push_back(src-1);
			}
			else
			{
				neighbor.push_back(src-1);
				neighbor.push_back(src+1);
			}
		}

		//    cout << src << " : neighbor num "<<  neighbor.size() << endl;

		for (int i=0; i < Dsts; i++)
		{
			list<int>::iterator p = neighbor.begin();
			int count = 0;
			int index = (int)( drand48() * neighbor.size());
			while (p != neighbor.end())
			{
				if (count++ == index)
				{
					dst[i] = *p;
					break;
				}
				++p;
			}
		}
	}
	else
	{
		//	cout << "RANDOM !!\n" ;
		uniform(src,dst);
	}

}



