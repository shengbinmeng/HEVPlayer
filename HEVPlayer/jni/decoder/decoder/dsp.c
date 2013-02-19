//**********************************************
//dsp.c
//Unipipy @2011
//transform & mc
//**********************************************
#include "config.h"

#include "internal.h"



/////////////////////////////////////////////////////////////////////////////////////////////////
//idct
/////////////////////////////////////////////////////////////////////////////////////////////////
const int LENT_dctdst_mode_ver[36] = 
{
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 0
};

const int LENT_dctdst_mode_hor[36] = 
{
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 0
};
const dctcoef g_aiT4[4][4] =
{
	{ 64, 64, 64, 64},
	{ 83, 36,-36,-83},
	{ 64,-64,-64, 64},
	{ 36,-83, 83,-36}
};

const dctcoef g_aiT8[8][8] =
{
	{ 64, 64, 64, 64, 64, 64, 64, 64},
	{ 89, 75, 50, 18,-18,-50,-75,-89},
	{ 83, 36,-36,-83,-83,-36, 36, 83},
	{ 75,-18,-89,-50, 50, 89, 18,-75},
	{ 64,-64,-64, 64, 64,-64,-64, 64},
	{ 50,-89, 18, 75,-75,-18, 89,-50},
	{ 36,-83, 83,-36,-36, 83,-83, 36},
	{ 18,-50, 75,-89, 89,-75, 50,-18}
};

const dctcoef g_aiT16[16][16] =
{
	{ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
	{ 90, 87, 80, 70, 57, 43, 25,  9, -9,-25,-43,-57,-70,-80,-87,-90},
	{ 89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89},
	{ 87, 57,  9,-43,-80,-90,-70,-25, 25, 70, 90, 80, 43, -9,-57,-87},
	{ 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83},
	{ 80,  9,-70,-87,-25, 57, 90, 43,-43,-90,-57, 25, 87, 70, -9,-80},
	{ 75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75},
	{ 70,-43,-87,  9, 90, 25,-80,-57, 57, 80,-25,-90, -9, 87, 43,-70},
	{ 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64},
	{ 57,-80,-25, 90, -9,-87, 43, 70,-70,-43, 87,  9,-90, 25, 80,-57},
	{ 50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50},
	{ 43,-90, 57, 25,-87, 70,  9,-80, 80, -9,-70, 87,-25,-57, 90,-43},
	{ 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36},
	{ 25,-70, 90,-80, 43,  9,-57, 87,-87, 57, -9,-43, 80,-90, 70,-25},
	{ 18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18},
	{  9,-25, 43,-57, 70,-80, 87,-90, 90,-87, 80,-70, 57,-43, 25, -9}
};

const dctcoef g_aiT32[32][32] =
{
	{ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
	{ 90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13,  4, -4,-13,-22,-31,-38,-46,-54,-61,-67,-73,-78,-82,-85,-88,-90,-90},
	{ 90, 87, 80, 70, 57, 43, 25,  9, -9,-25,-43,-57,-70,-80,-87,-90,-90,-87,-80,-70,-57,-43,-25, -9,  9, 25, 43, 57, 70, 80, 87, 90},
	{ 90, 82, 67, 46, 22, -4,-31,-54,-73,-85,-90,-88,-78,-61,-38,-13, 13, 38, 61, 78, 88, 90, 85, 73, 54, 31,  4,-22,-46,-67,-82,-90},
	{ 89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89, 89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89},
	{ 88, 67, 31,-13,-54,-82,-90,-78,-46, -4, 38, 73, 90, 85, 61, 22,-22,-61,-85,-90,-73,-38,  4, 46, 78, 90, 82, 54, 13,-31,-67,-88},
	{ 87, 57,  9,-43,-80,-90,-70,-25, 25, 70, 90, 80, 43, -9,-57,-87,-87,-57, -9, 43, 80, 90, 70, 25,-25,-70,-90,-80,-43,  9, 57, 87},
	{ 85, 46,-13,-67,-90,-73,-22, 38, 82, 88, 54, -4,-61,-90,-78,-31, 31, 78, 90, 61,  4,-54,-88,-82,-38, 22, 73, 90, 67, 13,-46,-85},
	{ 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83},
	{ 82, 22,-54,-90,-61, 13, 78, 85, 31,-46,-90,-67,  4, 73, 88, 38,-38,-88,-73, -4, 67, 90, 46,-31,-85,-78,-13, 61, 90, 54,-22,-82},
	{ 80,  9,-70,-87,-25, 57, 90, 43,-43,-90,-57, 25, 87, 70, -9,-80,-80, -9, 70, 87, 25,-57,-90,-43, 43, 90, 57,-25,-87,-70,  9, 80},
	{ 78, -4,-82,-73, 13, 85, 67,-22,-88,-61, 31, 90, 54,-38,-90,-46, 46, 90, 38,-54,-90,-31, 61, 88, 22,-67,-85,-13, 73, 82,  4,-78},
	{ 75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75, 75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75},
	{ 73,-31,-90,-22, 78, 67,-38,-90,-13, 82, 61,-46,-88, -4, 85, 54,-54,-85,  4, 88, 46,-61,-82, 13, 90, 38,-67,-78, 22, 90, 31,-73},
	{ 70,-43,-87,  9, 90, 25,-80,-57, 57, 80,-25,-90, -9, 87, 43,-70,-70, 43, 87, -9,-90,-25, 80, 57,-57,-80, 25, 90,  9,-87,-43, 70},
	{ 67,-54,-78, 38, 85,-22,-90,  4, 90, 13,-88,-31, 82, 46,-73,-61, 61, 73,-46,-82, 31, 88,-13,-90, -4, 90, 22,-85,-38, 78, 54,-67},
	{ 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64},
	{ 61,-73,-46, 82, 31,-88,-13, 90, -4,-90, 22, 85,-38,-78, 54, 67,-67,-54, 78, 38,-85,-22, 90,  4,-90, 13, 88,-31,-82, 46, 73,-61},
	{ 57,-80,-25, 90, -9,-87, 43, 70,-70,-43, 87,  9,-90, 25, 80,-57,-57, 80, 25,-90,  9, 87,-43,-70, 70, 43,-87, -9, 90,-25,-80, 57},
	{ 54,-85, -4, 88,-46,-61, 82, 13,-90, 38, 67,-78,-22, 90,-31,-73, 73, 31,-90, 22, 78,-67,-38, 90,-13,-82, 61, 46,-88,  4, 85,-54},
	{ 50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50, 50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50},
	{ 46,-90, 38, 54,-90, 31, 61,-88, 22, 67,-85, 13, 73,-82,  4, 78,-78, -4, 82,-73,-13, 85,-67,-22, 88,-61,-31, 90,-54,-38, 90,-46},
	{ 43,-90, 57, 25,-87, 70,  9,-80, 80, -9,-70, 87,-25,-57, 90,-43,-43, 90,-57,-25, 87,-70, -9, 80,-80,  9, 70,-87, 25, 57,-90, 43},
	{ 38,-88, 73, -4,-67, 90,-46,-31, 85,-78, 13, 61,-90, 54, 22,-82, 82,-22,-54, 90,-61,-13, 78,-85, 31, 46,-90, 67,  4,-73, 88,-38},
	{ 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36},
	{ 31,-78, 90,-61,  4, 54,-88, 82,-38,-22, 73,-90, 67,-13,-46, 85,-85, 46, 13,-67, 90,-73, 22, 38,-82, 88,-54, -4, 61,-90, 78,-31},
	{ 25,-70, 90,-80, 43,  9,-57, 87,-87, 57, -9,-43, 80,-90, 70,-25,-25, 70,-90, 80,-43, -9, 57,-87, 87,-57,  9, 43,-80, 90,-70, 25},
	{ 22,-61, 85,-90, 73,-38, -4, 46,-78, 90,-82, 54,-13,-31, 67,-88, 88,-67, 31, 13,-54, 82,-90, 78,-46,  4, 38,-73, 90,-85, 61,-22},
	{ 18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18, 18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18},
	{ 13,-38, 61,-78, 88,-90, 85,-73, 54,-31,  4, 22,-46, 67,-82, 90,-90, 82,-67, 46,-22, -4, 31,-54, 73,-85, 90,-88, 78,-61, 38,-13},
	{  9,-25, 43,-57, 70,-80, 87,-90, 90,-87, 80,-70, 57,-43, 25, -9, -9, 25,-43, 57,-70, 80,-87, 90,-90, 87,-80, 70,-57, 43,-25,  9},
	{  4,-13, 22,-31, 38,-46, 54,-61, 67,-73, 78,-82, 85,-88, 90,-90, 90,-90, 88,-85, 82,-78, 73,-67, 61,-54, 46,-38, 31,-22, 13, -4}
};


static inline void pixel_sub_wxh( dctcoef *diff, int i_size,
								 pixel *pix1, int i_pix1, pixel *pix2, int i_pix2 )
{
	int y, x;
	for( y = 0; y < i_size; y++ )
	{
		for( x = 0; x < i_size; x++ )
			diff[x + y*i_size] = pix1[x] - pix2[x];
		pix1 += i_pix1;
		pix2 += i_pix2;
	}
}

#define idx(x,y) ((x)<<2|(y))
static void LENT_dst( dctcoef *dst, dctcoef *src, int shift )
{
	int i, c[4];
	int rnd_factor = 1<<(shift-1);
	for (i=0; i<4; i++)
	{
		// Intermediate Variables
		c[0] = src[idx(i,0)] + src[idx(i,3)];
		c[1] = src[idx(i,1)] + src[idx(i,3)];
		c[2] = src[idx(i,0)] - src[idx(i,1)];
		c[3] = 74* src[idx(i,2)];

		dst[idx(0,i)] =  ( 29 * c[0] + 55 * c[1]         + c[3]               + rnd_factor ) >> shift;
		dst[idx(1,i)] =  ( 74 * (src[idx(i,0)]+src[idx(i,1)] - src[idx(i,3)]) + rnd_factor ) >> shift;
		dst[idx(2,i)] =  ( 29 * c[2] + 55 * c[0]         - c[3]               + rnd_factor ) >> shift;
		dst[idx(3,i)] =  ( 55 * c[2] - 29 * c[1]         + c[3]               + rnd_factor ) >> shift;
	}
}

static void LENT_inv_dst( dctcoef *dst, dctcoef *src, int shift )
{
	int i, c[4];
	int rnd_factor = 1<<(shift-1);
	for (i=0; i<4; i++)
	{  
		// Intermediate Variables
		c[0] = src[idx(0,i)] + src[idx(2,i)];
		c[1] = src[idx(2,i)] + src[idx(3,i)];
		c[2] = src[idx(0,i)] - src[idx(3,i)];
		c[3] = 74* src[idx(1,i)];

		dst[idx(i,0)] =  ( 29 * c[0] + 55 * c[1]     + c[3]               + rnd_factor ) >> shift;
		dst[idx(i,1)] =  ( 55 * c[2] - 29 * c[1]     + c[3]               + rnd_factor ) >> shift;
		dst[idx(i,2)] =  ( 74 * (src[idx(0,i)] - src[idx(2,i)] + src[idx(3,i)]) + rnd_factor ) >> shift;
		dst[idx(i,3)] =  ( 55 * c[0] + 29 * c[2]     - c[3]               + rnd_factor ) >> shift;
	}
}

static void LENT_dct_4( dctcoef *dst, dctcoef *src, int shift )
{
	int j;  
	int E[2],O[2];
	int add = 1<<(shift-1);

	for (j=0; j<4; j++)
	{
		/* E and O */
		E[0] = src[idx(j,0)] + src[idx(j,3)];
		O[0] = src[idx(j,0)] - src[idx(j,3)];
		E[1] = src[idx(j,1)] + src[idx(j,2)];
		O[1] = src[idx(j,1)] - src[idx(j,2)];

		dst[idx(0,j)] = (g_aiT4[0][0]*E[0] + g_aiT4[0][1]*E[1] + add)>>shift;
		dst[idx(2,j)] = (g_aiT4[2][0]*E[0] + g_aiT4[2][1]*E[1] + add)>>shift;
		dst[idx(1,j)] = (g_aiT4[1][0]*O[0] + g_aiT4[1][1]*O[1] + add)>>shift;
		dst[idx(3,j)] = (g_aiT4[3][0]*O[0] + g_aiT4[3][1]*O[1] + add)>>shift;
	}
}

static void LENT_inv_dct_4( dctcoef *dst, dctcoef *src, int shift )
{
	int j;    
	int E[2],O[2];
	int add = 1<<(shift-1);

	for (j=0; j<4; j++)
	{
		/* Utilizing symmetry properties to the maximum to minimize the number of multiplications */    
		O[0] = g_aiT4[1][0]*src[idx(1,j)] + g_aiT4[3][0]*src[idx(3,j)];
		O[1] = g_aiT4[1][1]*src[idx(1,j)] + g_aiT4[3][1]*src[idx(3,j)];
		E[0] = g_aiT4[0][0]*src[idx(0,j)] + g_aiT4[2][0]*src[idx(2,j)];
		E[1] = g_aiT4[0][1]*src[idx(0,j)] + g_aiT4[2][1]*src[idx(2,j)];

		/* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */ 
		dst[idx(j,0)] = (E[0] + O[0] + add)>>shift;
		dst[idx(j,1)] = (E[1] + O[1] + add)>>shift;
		dst[idx(j,2)] = (E[1] - O[1] + add)>>shift;
		dst[idx(j,3)] = (E[0] - O[0] + add)>>shift;
	}
}
#undef idx

#define idx(x,y)  ((x)<<3|(y))
static void LENT_dct_8( dctcoef *dst, dctcoef *src, int shift )
{
	int j,k;  
	int E[4],O[4];
	int EE[2],EO[2];
	int add = 1<<(shift-1);

	for (j=0; j<8; j++)
	{    
		/* E and O*/
		for (k=0;k<4;k++)
		{
			E[k] = src[idx(j,k)] + src[idx(j,7-k)];
			O[k] = src[idx(j,k)] - src[idx(j,7-k)];
		}    
		/* EE and EO */
		EE[0] = E[0] + E[3];    
		EO[0] = E[0] - E[3];
		EE[1] = E[1] + E[2];
		EO[1] = E[1] - E[2];

		dst[idx(0,j)] = (g_aiT8[0][0]*EE[0] + g_aiT8[0][1]*EE[1] + add)>>shift;
		dst[idx(4,j)] = (g_aiT8[4][0]*EE[0] + g_aiT8[4][1]*EE[1] + add)>>shift; 
		dst[idx(2,j)] = (g_aiT8[2][0]*EO[0] + g_aiT8[2][1]*EO[1] + add)>>shift;
		dst[idx(6,j)] = (g_aiT8[6][0]*EO[0] + g_aiT8[6][1]*EO[1] + add)>>shift; 

		dst[idx(1,j)] = (g_aiT8[1][0]*O[0] + g_aiT8[1][1]*O[1] + g_aiT8[1][2]*O[2] + g_aiT8[1][3]*O[3] + add)>>shift;
		dst[idx(3,j)] = (g_aiT8[3][0]*O[0] + g_aiT8[3][1]*O[1] + g_aiT8[3][2]*O[2] + g_aiT8[3][3]*O[3] + add)>>shift;
		dst[idx(5,j)] = (g_aiT8[5][0]*O[0] + g_aiT8[5][1]*O[1] + g_aiT8[5][2]*O[2] + g_aiT8[5][3]*O[3] + add)>>shift;
		dst[idx(7,j)] = (g_aiT8[7][0]*O[0] + g_aiT8[7][1]*O[1] + g_aiT8[7][2]*O[2] + g_aiT8[7][3]*O[3] + add)>>shift;
	}
}

static void LENT_inv_dct_8( dctcoef *dst, dctcoef *src, int shift )
{
	int j,k;    
	int E[4],O[4];
	int EE[2],EO[2];
	int add = 1<<(shift-1);

	for (j=0; j<8; j++)
	{    
		/* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
		for (k=0;k<4;k++)
		{
			O[k] = g_aiT8[ 1][k]*src[idx(1,j)] + g_aiT8[ 3][k]*src[idx(3,j)] + g_aiT8[ 5][k]*src[idx(5,j)] + g_aiT8[ 7][k]*src[idx(7,j)];
		}

		EO[0] = g_aiT8[2][0]*src[idx(2,j)] + g_aiT8[6][0]*src[idx(6,j)];
		EO[1] = g_aiT8[2][1]*src[idx(2,j)] + g_aiT8[6][1]*src[idx(6,j)];
		EE[0] = g_aiT8[0][0]*src[idx(0,j)] + g_aiT8[4][0]*src[idx(4,j)];
		EE[1] = g_aiT8[0][1]*src[idx(0,j)] + g_aiT8[4][1]*src[idx(4,j)];

		/* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */ 
		E[0] = EE[0] + EO[0];
		E[3] = EE[0] - EO[0];
		E[1] = EE[1] + EO[1];
		E[2] = EE[1] - EO[1];
		for (k=0;k<4;k++)
		{
			dst[idx(j,k)] = (E[k] + O[k] + add)>>shift;
			dst[idx(j,k+4)] = (E[3-k] - O[3-k] + add)>>shift;
		}        
	}
}
#undef idx

#define idx(x,y)  ((x)<<4|(y))
static void LENT_inv_dct_16( dctcoef *dst, dctcoef *src, int shift )
{
	int j,k;  
	int E[8],O[8];
	int EE[4],EO[4];
	int EEE[2],EEO[2];
	int add = 1<<(shift-1);

	for (j=0; j<16; j++)
	{    
		/* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
		for (k=0;k<8;k++)
		{
			O[k] = g_aiT16[ 1][k]*src[idx( 1,j)] + g_aiT16[ 3][k]*src[idx( 3,j)] + g_aiT16[ 5][k]*src[idx( 5,j)] + g_aiT16[ 7][k]*src[idx( 7,j)] + 
				g_aiT16[ 9][k]*src[idx( 9,j)] + g_aiT16[11][k]*src[idx(11,j)] + g_aiT16[13][k]*src[idx(13,j)] + g_aiT16[15][k]*src[idx(15,j)];
		}
		for (k=0;k<4;k++)
		{
			EO[k] = g_aiT16[ 2][k]*src[idx( 2,j)] + g_aiT16[ 6][k]*src[idx( 6,j)] + g_aiT16[10][k]*src[idx(10,j)] + g_aiT16[14][k]*src[idx(14,j)];
		}
		EEO[0] = g_aiT16[4][0]*src[idx(4,j)] + g_aiT16[12][0]*src[idx(12,j)];
		EEE[0] = g_aiT16[0][0]*src[idx(0,j)] + g_aiT16[ 8][0]*src[idx( 8,j)];
		EEO[1] = g_aiT16[4][1]*src[idx(4,j)] + g_aiT16[12][1]*src[idx(12,j)];
		EEE[1] = g_aiT16[0][1]*src[idx(0,j)] + g_aiT16[ 8][1]*src[idx( 8,j)];

		/* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */ 
		for (k=0;k<2;k++)
		{
			EE[k] = EEE[k] + EEO[k];
			EE[k+2] = EEE[1-k] - EEO[1-k];
		}    
		for (k=0;k<4;k++)
		{
			E[k] = EE[k] + EO[k];
			E[k+4] = EE[3-k] - EO[3-k];
		}    
		for (k=0;k<8;k++)
		{
			dst[idx(j,k)] = (E[k] + O[k] + add)>>shift;
			dst[idx(j,k+8)] = (E[7-k] - O[7-k] + add)>>shift;
		}        
	}
}
#undef idx

#define idx(x,y)  ((x)<<5|(y))
static void LENT_inv_dct_32( dctcoef *dst, dctcoef *src, int shift )
{
	int j,k;  
	int E[16],O[16];
	int EE[8],EO[8];
	int EEE[4],EEO[4];
	int EEEE[2],EEEO[2];
	int add = 1<<(shift-1);

	for (j=0; j<32; j++)
	{    
		/* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
		for (k=0;k<16;k++)
		{
			O[k] = g_aiT32[ 1][k]*src[idx( 1,j)] + g_aiT32[ 3][k]*src[idx( 3,j)] + g_aiT32[ 5][k]*src[idx( 5,j)] + g_aiT32[ 7][k]*src[idx( 7,j)] + 
				g_aiT32[ 9][k]*src[idx( 9,j)] + g_aiT32[11][k]*src[idx(11,j)] + g_aiT32[13][k]*src[idx(13,j)] + g_aiT32[15][k]*src[idx(15,j)] + 
				g_aiT32[17][k]*src[idx(17,j)] + g_aiT32[19][k]*src[idx(19,j)] + g_aiT32[21][k]*src[idx(21,j)] + g_aiT32[23][k]*src[idx(23,j)] + 
				g_aiT32[25][k]*src[idx(25,j)] + g_aiT32[27][k]*src[idx(27,j)] + g_aiT32[29][k]*src[idx(29,j)] + g_aiT32[31][k]*src[idx(31,j)];
		}
		for (k=0;k<8;k++)
		{
			EO[k] = g_aiT32[ 2][k]*src[idx( 2,j)] + g_aiT32[ 6][k]*src[idx( 6,j)] + g_aiT32[10][k]*src[idx(10,j)] + g_aiT32[14][k]*src[idx(14,j)] + 
				g_aiT32[18][k]*src[idx(18,j)] + g_aiT32[22][k]*src[idx(22,j)] + g_aiT32[26][k]*src[idx(26,j)] + g_aiT32[30][k]*src[idx(30,j)];
		}
		for (k=0;k<4;k++)
		{
			EEO[k] = g_aiT32[4][k]*src[idx(4,j)] + g_aiT32[12][k]*src[idx(12,j)] + g_aiT32[20][k]*src[idx(20,j)] + g_aiT32[28][k]*src[idx(28,j)];
		}
		EEEO[0] = g_aiT32[8][0]*src[idx(8,j)] + g_aiT32[24][0]*src[idx(24,j)];
		EEEO[1] = g_aiT32[8][1]*src[idx(8,j)] + g_aiT32[24][1]*src[idx(24,j)];
		EEEE[0] = g_aiT32[0][0]*src[idx(0,j)] + g_aiT32[16][0]*src[idx(16,j)];    
		EEEE[1] = g_aiT32[0][1]*src[idx(0,j)] + g_aiT32[16][1]*src[idx(16,j)];

		/* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
		EEE[0] = EEEE[0] + EEEO[0];
		EEE[3] = EEEE[0] - EEEO[0];
		EEE[1] = EEEE[1] + EEEO[1];
		EEE[2] = EEEE[1] - EEEO[1];    
		for (k=0;k<4;k++)
		{
			EE[k] = EEE[k] + EEO[k];
			EE[k+4] = EEE[3-k] - EEO[3-k];
		}    
		for (k=0;k<8;k++)
		{
			E[k] = EE[k] + EO[k];
			E[k+8] = EE[7-k] - EO[7-k];
		}    
		for (k=0;k<16;k++)
		{
			dst[idx(j,k)] = (E[k] + O[k] + add)>>shift;
			dst[idx(j,k+16)] = (E[15-k] - O[15-k] + add)>>shift;
		}        
	}
}
#undef idx

void add4x4_idct_c( pixel *p_src, int i_src, dctcoef dct[16], int i_mode )
{
	dctcoef d[16];
	dctcoef tmp[16];
	int i, j;
	
	if( LENT_dctdst_mode_ver[i_mode] )
		LENT_inv_dst( tmp, dct, 7 );
	else
		LENT_inv_dct_4( tmp, dct, 7 );

	if( LENT_dctdst_mode_hor[i_mode] )
		LENT_inv_dst( d, tmp, 12 );
	else
		LENT_inv_dct_4( d, tmp, 12 );

	for( i = 0; i < 4; i++ )
	{
		for( j = 0; j < 4; j++ )
		{
			p_src[j] = lent_clip_uint8( p_src[j] + d[i*4+j] );
		}
		p_src += i_src;
	}
}
static void add8x8_idct_c( pixel *p_src, int i_src, dctcoef dct[64] )
{
	dctcoef d[64];
	dctcoef tmp[64];
	int i, j;

	LENT_inv_dct_8( tmp, dct, 7 );
	LENT_inv_dct_8( d, tmp, 12 );

	for( i = 0; i < 8; i++ )
	{
		for( j = 0; j < 8; j++ )
		{
			p_src[j] = lent_clip_uint8( p_src[j] + d[i*8+j] );
		}
		p_src += i_src;
	}
}

void add16x16_idct_c( pixel *p_src, int i_src, dctcoef dct[256] )
{
	dctcoef d[256];
	dctcoef tmp[256];
	int i, j;

	LENT_inv_dct_16( tmp, dct, 7 );
	LENT_inv_dct_16( d, tmp, 12 );

	for( i = 0; i < 16; i++ )
	{
		for( j = 0; j < 16; j++ )
		{
			p_src[j] = lent_clip_uint8( p_src[j] + d[i*16+j] );
		}
		p_src += i_src;
	}
}


void add32x32_idct_c( pixel *p_src, int i_src, dctcoef dct[1024] )
{
	dctcoef d[1024];
	dctcoef tmp[1024];
	int i, j;

	LENT_inv_dct_32( tmp, dct, 7 );
	LENT_inv_dct_32( d, tmp, 12 );

	for( i = 0; i < 32; i++ )
	{
		for( j = 0; j < 32; j++ )
		{
			p_src[j] = lent_clip_uint8( p_src[j] + d[i*32+j] );
		}
		p_src += i_src;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//MC相关
/////////////////////////////////////////////////////////////////////////////////////////////////
static int16_t luma_frac_coeff[4][8] =
{
	{  0, 0,   0, 64,  0,   0, 0,  0 },
	{ -1, 4, -10, 58, 17,  -5, 1,  0 },
	{ -1, 4, -11, 40, 40, -11, 4, -1 },
	{  0, 1,  -5, 17, 58, -10, 4, -1 }
};

static int16_t chroma_frac_coeff[8][4] = 
{
	{  0, 64,  0,  0 },
	{ -2, 58, 10, -2 },
	{ -4, 54, 16, -2 },
	{ -6, 46, 28, -4 },
	{ -4, 36, 36, -4 },
	{ -4, 28, 46, -6 },
	{ -2, 16, 54, -4 },
	{ -2, 10, 58, -2 }
};

static void draw_edge_c( pixel *p_dst, int drawheight, int picwidth, int stride, int sides, int shift)
{
	int i;
	pixel *p=p_dst-DRAW_EDGE_DELAY*stride;

	//draw horizontal band
	if ( 0 == shift ) {
		for(i=-DRAW_EDGE_DELAY;i<drawheight-DRAW_EDGE_DELAY;i++)
		{
			memset(p-(EDGE_WIDTH>>shift),	p[0],			(EDGE_WIDTH)*sizeof(*p));
			memset(p+picwidth,		p[picwidth-1],			(EDGE_WIDTH)*sizeof(*p));
			p+=stride;
		}
	} else if ( 1 == shift ) {
		for(i=-DRAW_EDGE_DELAY;i<drawheight-DRAW_EDGE_DELAY;i++)
		{
			memset(p-(EDGE_WIDTH>>shift),	p[0],			(EDGE_WIDTH>>1)*sizeof(*p));
			memset(p+picwidth,		p[picwidth-1],			(EDGE_WIDTH>>1)*sizeof(*p));
			p+=stride;
		}
	} else {
		for(i=-DRAW_EDGE_DELAY;i<drawheight-DRAW_EDGE_DELAY;i++)
		{
			memset(p-(EDGE_WIDTH>>shift),	p[0],			(EDGE_WIDTH>>shift)*sizeof(*p));
			memset(p+picwidth,		p[picwidth-1],			(EDGE_WIDTH>>shift)*sizeof(*p));
			p+=stride;
		}
	}
	if(sides&SIDE_BOTTOM)
	{
		for(i=drawheight-DRAW_EDGE_DELAY;i<drawheight;i++)
		{
			memset(p-(EDGE_WIDTH>>shift),	p[0],			(EDGE_WIDTH>>shift)*sizeof(*p));
			memset(p+picwidth,		p[picwidth-1],			(EDGE_WIDTH>>shift)*sizeof(*p));
			p+=stride;
		}
	}
	//draw top
	if(sides&SIDE_TOP)
	{
		p=p_dst-(EDGE_TOP>>shift)*stride-(EDGE_WIDTH>>shift);
		for(i=0;i<(EDGE_TOP>>shift);i++)
		{
			memcpy(p,p_dst-(EDGE_WIDTH>>shift),stride*sizeof(*p));
			p+=stride;
		}
	}

	//draw bottom
	if(sides&SIDE_BOTTOM)
	{
		p_dst+=drawheight*stride-stride;
		p=p_dst+stride-(EDGE_WIDTH>>shift);
		for(i=0;i<(EDGE_BOTTOM>>shift);i++)
		{
			memcpy(p,p_dst-(EDGE_WIDTH>>shift),stride*sizeof(*p));
			p+=stride;
		}
	}
}

static void mc_copy( pixel *dst, int i_dst, pixel *src, int i_src, int i_width, int i_height )
{
	int y;
	for( y = 0; y < i_height; y++, src += i_src, dst += i_dst )
		memcpy( dst, src, i_width * sizeof(pixel) );
}

#define TAPFILTER(src,f) \
	(src[x-3]*f[0]+src[x-2]*f[1]+src[x-1]*f[2]+src[x]*f[3]+ \
	src[x+1]*f[4]+src[x+2]*f[5]+src[x+3]*f[6]+src[x+4]*f[7]) 
static void hpel_filter( int16_t *dst1, int16_t *dst2, int16_t *dst3, pixel *src,
						int stride, int width, int height, int select_flag )
{
	int x, y;

	src -= stride*3;
	for( y=-3; y<height+4; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			dst1[x] = TAPFILTER(src,luma_frac_coeff[1]);//todo optimize
			dst2[x] = TAPFILTER(src,luma_frac_coeff[2]);
			dst3[x] = TAPFILTER(src,luma_frac_coeff[3]);
		}
		dst1 += HPEL_SIZE;
		dst2 += HPEL_SIZE;
		dst3 += HPEL_SIZE;
		src += stride;
	}
}
#undef TAPFILTER


#define TAPFILTER(src,f) \
	((int32_t)src[x-3*i_src]*f[0]+src[x-2*i_src]*f[1]+src[x-1*i_src]*f[2]+src[x]*f[3]+ \
	src[x+1*i_src]*f[4]+src[x+2*i_src]*f[5]+src[x+3*i_src]*f[6]+src[x+4*i_src]*f[7]) 
void mc_luma( pixel *dst, int i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
			 int mvx, int mvy, int i_width, int i_height )
{
	int fx = mvx&3;
	int fy = mvy&3;
	int i_offs = (mvy>>2)*i_src + (mvx>>2);
	int x, y;
	const int16_t *f = luma_frac_coeff[fy];

	if( fx == 0 )
	{//use src
		src += i_offs;
		if( fy == 0 )
			mc_copy( dst, i_dst, src, i_src, i_width, i_height );
		else
		{
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( src, f )+32)>>6);
				dst  += i_dst;
				src  += i_src;
			}
		}
	}
	else
	{//use hpx
		int16_t *hsrc = hpx[fx-1]+3*HPEL_SIZE /*+ i_offs*/;
		i_src=HPEL_SIZE;

		if( fy == 0 )
		{//加速
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((hsrc[x]  + 32) >> 6);
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
		else
		{
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( hsrc, f ) + ((1<<11))) >> 12);
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
	}
}
#undef TAPFILTER

#define TAPFILTER(src,f) \
	((int32_t)src[x-3*i_src]*f[0]+src[x-2*i_src]*f[1]+src[x-1*i_src]*f[2]+src[x]*f[3]+ \
	src[x+1*i_src]*f[4]+src[x+2*i_src]*f[5]+src[x+3*i_src]*f[6]+src[x+4*i_src]*f[7])
int16_t* get_ref( int16_t *dst, int *i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
				  int mvx, int mvy, int i_width, int i_height )
{
	int fx = mvx&3;
	int fy = mvy&3;
	int i_offs = (mvy>>2)*i_src + (mvx>>2);
	int x, y;
	const int16_t *f = luma_frac_coeff[fy];
	int16_t *rt = NULL;

	if( fx == 0 )
	{
		rt = dst;
		src += i_offs;

		if( fy == 0 )
		{
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x++ )
					dst[x] = (src[x]<<6);
				dst  += *i_dst;
				src  += i_src;
			}
		}
		else
		{
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x++ )
					dst[x] = TAPFILTER( src, f );
				dst  += *i_dst;
				src  += i_src;
			}
		}
	}
	else
	{
		int16_t *hsrc = hpx[fx-1] +3*HPEL_SIZE/*+ i_offs*/;
		i_src = HPEL_SIZE;

		if( fy == 0 )
		{
			*i_dst = i_src;
			rt = hsrc;
		}
		else
		{
			rt = dst;

			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x++ )
					dst[x] = (TAPFILTER( hsrc, f )>>6);
				dst  += *i_dst;
				hsrc += i_src;
			}
		}
	}

	return rt;
}
#undef TAPFILTER

void mc_avg( pixel *dst, int i_dst,
			int16_t *src1, int i_src1, int16_t *src2, int i_src2,
			int i_width, int i_height )
{
	int x, y;

	for( y = 0; y < i_height; y++ )
	{
		for( x = 0; x < i_width; x++ )
			dst[x] = lent_clip_uint8(((int32_t)src1[x] + src2[x] + ((1<<6))) >> 7);
		dst  += i_dst;
		src1 += i_src1;
		src2 += i_src2;
	}
}
void mc_weight_uni( pixel *dst, int i_dst,
			int16_t *src, int i_src,
			int i_width, int i_height,
			int _w, int _offset, int _shift, int pixBitDepth)
{
	int x, y;
	int w0      = _w;
	int offset  = _offset;
	int shiftNum = IF_INTERNAL_PREC - pixBitDepth;
	int shift   = _shift + shiftNum;
	int round   = shift?(1<<(shift-1)):0;

	for( y = 0; y < i_height; y++ )
	{
		for( x = 0; x < i_width; x++ )
			dst[x] = lent_clip_uint8(( (w0*src[x] + round) >> shift ) + offset );
		dst  += i_dst;
		src += i_src;
	}
}
void mc_weight_bi( pixel *dst, int i_dst,
			int16_t *src1, int i_src1, int16_t *src2, int i_src2,
			int i_width, int i_height,
			int _w0, int _w1, int _offset, int _shift, int pixBitDepth)
{
	int x, y;
	int w0      = _w0;
	int w1      = _w1;
	int offset  = _offset;
	int shiftNum = IF_INTERNAL_PREC - pixBitDepth;
	int shift   = _shift + shiftNum;
	int round   = shift?(1<<(shift-1)):0;

	offset=offset<<(shift-1);
	for( y = 0; y < i_height; y++ )
	{
		for( x = 0; x < i_width; x++ )
			dst[x] = lent_clip_uint8( ( (w0*src1[x] + w1*src2[x] + round + offset) >> shift ) );
		dst  += i_dst;
		src1 += i_src1;
		src2 += i_src2;
	}
}
void mc_chroma( pixel *dst, int i_dst, pixel *src, int i_src,
			   int mvx, int mvy, int i_width, int i_height )
{
	pixel *tmp;
	int x, y;
	int fx = mvx&0x7;
	int fy = mvy&0x7;
	const int16_t *f;

	src += (mvy >> 3) * i_src + (mvx >> 3);
	tmp = &src[i_src];

	if( fy == 0 )
	{
		f = chroma_frac_coeff[fx];

		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x++ )
				dst[x] = lent_clip_uint8(( f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2] + 32 ) >> 6);
			dst  += i_dst;
			src   = tmp;
			tmp += i_src;
		}
	}
	else if( fx == 0 )
	{
		f = chroma_frac_coeff[fy];
		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x++ )
				dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
			dst  += i_dst;
			src   = tmp;
			tmp += i_src;
		}
	}
	else
	{
		/*ALIGNED_16(*/ dctcoef buf_array[32*35] /*)*/;
		dctcoef *buf = buf_array;

		tmp = src;
		src -= i_src;
		f = chroma_frac_coeff[fx];
		for( y = -1; y < i_height+2; y++ )
		{
			for( x = 0; x < i_width; x++ )
				buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
			buf  += 32;
			src  += i_src;
		}
		
		buf = buf_array + 32;
		f = chroma_frac_coeff[fy];
		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x++ )
				dst[x] = lent_clip_uint8(( (int32_t)f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
			dst  += i_dst;
			buf  += 32;
		}
	}
}
void get_ref_chroma( int16_t *dst, int i_dst, pixel *src, int i_src,
						   int mvx, int mvy, int i_width, int i_height )
{
	pixel *tmp;
	int x, y;
	int fx = mvx&0x7;
	int fy = mvy&0x7;
	const int16_t *f;

	src += (mvy >> 3) * i_src + (mvx >> 3);
	tmp = &src[i_src];

	if( fy == 0 )
	{
		f = chroma_frac_coeff[fx];

		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x++ )
				dst[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
			dst  += i_dst;
			src   = tmp;
			tmp += i_src;
		}
	}
	else if( fx == 0 )
	{
		f = chroma_frac_coeff[fy];
		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x++ )
				dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
			dst  += i_dst;
			src   = tmp;
			tmp += i_src;
		}
	}
	else
	{
		/*ALIGNED_16(*/ dctcoef buf_array[32*35] /*)*/;
		dctcoef *buf = buf_array;

		tmp = src;
		src -= i_src;
		f = chroma_frac_coeff[fx];
		for( y = -1; y < i_height+2; y++ )
		{
			for( x = 0; x < i_width; x++ )
				buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
			buf  += 32;
			src  += i_src;
		}

		buf = buf_array + 32;
		f = chroma_frac_coeff[fy];
		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x++ )
				dst[x] = ((int32_t)f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
			dst  += i_dst;
			buf  += 32;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//反量化
/////////////////////////////////////////////////////////////////////////////////////////////////
//dequant tables
static const int i_dequant_scales[6] =
{
	40,45,51,57,64,72
};
#define DEQUANT( coef ) \
{	\
	/*if( (coef) > 0 ) */(coef) =   (bias + (coef) * mf) >> i_shift; \
	/*else             (coef) = -((bias - (coef) * mf) >> i_shift); */\
}

static void dequant_c( dctcoef *dct, int qp, int i_log2_size)
{
	int i;
	int i_shift = i_log2_size - 1;
	int i_size = (1<<(i_log2_size<<1));
	int bias = (1 << (i_shift - 1));
	int mf=i_dequant_scales[qp%6];

	mf <<= (qp/6);
	for( i = 0; i < i_size; i ++ )
		DEQUANT( dct[i] );
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//初始化对外接口
/////////////////////////////////////////////////////////////////////////////////////////////////


void dsp_init(DSPContext *dsp,unsigned int machine)
{
	void dsp_init_x86(DSPContext *dsp,unsigned int sse);

	dsp->add4x4_idct = add4x4_idct_c;
	dsp->add8x8_idct = add8x8_idct_c;
	dsp->add16x16_idct = add16x16_idct_c;
	dsp->add32x32_idct = add32x32_idct_c;

	dsp->draw_edge	=draw_edge_c;
	
	dsp->mc_luma = mc_luma;
	dsp->get_ref = get_ref;
	dsp->mc_chroma = mc_chroma;
	dsp->get_ref_chroma = get_ref_chroma;
	dsp->mc_avg = mc_avg;
	dsp->mc_weight_uni = mc_weight_uni;
	dsp->mc_weight_bi = mc_weight_bi;
	dsp->hpel_filter = hpel_filter;

	
	dsp->dequant = dequant_c;

#if ARCH_X86_32
	dsp_init_x86(dsp,machine);
#endif
}
