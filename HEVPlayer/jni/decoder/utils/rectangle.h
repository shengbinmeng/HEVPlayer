#ifndef UTILS_RECTANGLE_H
#define UTILS_RECTANGLE_H

#ifndef STRIDE_ALIGN
#   define STRIDE_ALIGN 8
#endif



static lent_always_inline void lent_fill_rectangle( void *dst, int w, int h, int stride, uint32_t v )
{
	uint8_t *d = (uint8_t *)dst;
	uint16_t v2 = v * 0x101;
	uint32_t v4 = v * 0x1010101;
	uint64_t v8 = v4 + ((uint64_t)v4 << 32);
	const int s = stride;
	int i;

	if( w == 1 )
	{
		for( i = 0; i < h; i ++ ) 
			*( d+s*i ) = v;
	}
	else if( w == 2 )
	{
		for( i = 0; i < h; i ++ ) 
			M16( d+s*i ) = v2;
	}
	else if( w == 4 )
	{
		for( i = 0; i < h; i ++ ) 
			M32( d+s*i ) = v4;
	}
	else if( w == 8 )
	{
		for( i = 0; i < h; i ++ ) 
			M64( d+s*i ) = v8;
	}
	else if( w == 16 )
	{
		for( i = 0; i < h; i ++ ) 
		{
			M64( d+s*i ) = v8;
			M64( d+s*i+8 ) = v8;
		}
	}
	else
		assert( 0 );
}
#endif