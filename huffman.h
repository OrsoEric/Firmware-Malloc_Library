#ifndef HUFFMAN_H
	#define HUFFMAN_H

	/**********************************************************************************
	**	ENVIROMENT VARIABILE
	**********************************************************************************/

	//#define HUFFMAN_DEBUG

	/**********************************************************************************
	**	GLOBAL INCLUDE
	**********************************************************************************/

	#include <stdio.h>
	//#include <stdlib.h>
	#include <stdint.h>

	/**********************************************************************************
	**	DEFINE
	**********************************************************************************/

	#ifndef NULL
		#define NULL (void *)0
	#endif

	//Maximum allowed depth of the huffman encoder, the depth is the size of a bit group
	#define HUFFMAN_MAX_DEPTH	8

	/**********************************************************************************
	**	MACRO
	**********************************************************************************/

	//Wrapper for the malloc, allow to use alternate malloc function
	#define MY_MALLOC( size )	\
		malloc( size )

	//Wrapper for the free, allow to use alternate free function
	#define MY_FREE( adr )	\
		free( adr );	\
		adr = NULL

	//This macro can be used to handle the error code
	#define HUFFMAN_ERROR_HANDLER( code )

	#ifndef SET_BIT
		//set a single bit inside a variabile lefting the other bit untouched
		#define SET_BIT( var, shift_value ) \
			( (var) |= (0x01 << shift_value ) )
	#endif

	#ifndef CLEAR_BIT
		//clear a single bit inside a variabile lefting the other bit untouched
		#define CLEAR_BIT( var, shift_value ) \
			( (var) &= (~ (0x01 << shift_value ) ) )
	#endif

	/**********************************************************************************
	**	TYPEDEF
	**********************************************************************************/

	/**********************************************************************************
	**	PROTOTYPE: STRUCTURE
	**********************************************************************************/

	/**********************************************************************************
	**	PROTOTYPE: GLOBAL VARIABILE
	**********************************************************************************/

	/**********************************************************************************
	**	PROTOTYPE: FUNCTION
	**********************************************************************************/

	//Maximum number that can be expressed with a huffman code of given depth and group number (the function cap at MAX_UINT8)
	extern uint8_t huf_max8( uint8_t depth, uint8_t num_group );
	extern uint16_t huf_max16( uint8_t depth, uint8_t num_group );
	//return the number of group needed to rapresent an 8 bit number with depth
	extern uint8_t huf_size8( uint8_t num8, uint8_t depth );
	extern uint8_t huf_size16( uint16_t num16, uint8_t depth );

	//Skip a number without reading it, return the number of readed bit (useful to fast seek a number)
	extern uint8_t huf_skip( uint8_t *str, uint8_t depth, uint16_t start );

	//Encode an 8-bit input number and append it to a string (DOES NOT CHECK END STRING)
	extern uint8_t huf_enc8( uint8_t *str, uint8_t num8, uint8_t depth, uint16_t start );
	extern uint8_t huf_enc16( uint8_t *str, uint16_t num16, uint8_t depth, uint16_t start );

	//Decode a uint8_t number from a source str string, load the result in the dest variabile and return the number of readed bit
	extern uint8_t huf_dec8( uint8_t *str, uint8_t *dest, uint8_t depth, uint16_t start );
	extern uint8_t huf_dec16( uint8_t *str, uint16_t *dest, uint8_t depth, uint16_t start );

	//extern uint8_t loghuf_enc8( uint8_t *str, uint8_t num8, uint8_t start_depth, uint16_t start );

#else
	#warning "multiple inclusion of the header file huffman.h"
#endif
