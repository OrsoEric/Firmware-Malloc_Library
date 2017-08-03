/****************************************************************************
**	R@P PROJECT
*****************************************************************************
**
*****************************************************************************
**	Author: 			Orso Eric
**	Creation Date:		24/08/11
**	Last Edit Date:
**	Revision:			2
**	Version:			1.0 BETA
****************************************************************************/

/****************************************************************************
**	HYSTORY VERSION
*****************************************************************************
**	r1v0.1 ALPHA
**	>huf_enc8 OK
**	>huf_max8 OK
**	>huf_dec8 OK
**	r2v1.0 BETA
**	>huf_size8 OK
**	>now the huf enc8 use the huf size8 to calculate the group number
****************************************************************************/

/****************************************************************************
**	DESCRIPTION
*****************************************************************************
**	This library encode a number using the huffman coding.
**	The coding is a bit group encoding to allow for better storing efficency
**	(TODO) The harmonic huffman coding allow for dynamic group size
**
**	Es. HUFFMAN
**	----------------------
**	Huf depth = 1
**	0	|	0b 0
**	1	|	0b 01
**	2	|	0b 011
**	3	|	0b 0111
**	4	|	0b 01111
**	----------------------
**	Huf depth = 2
**	0	|	0b 00
**	1	|	0b 01
**	2	|	0b 00 10
**	5	|	0b 01 11
**	6	|	0b 00 10 10
**	13	|	0b 01 11 11
**	14	|	0b 00 10 10 10
**	29	|	0b 01 11 11 11
**	----------------------
**	Huf depth = 3
**	0	|	0b 000
**	3	|	0b 011
**	4	|	0b 000 100
**	19	|	0b 011 111
**	20	|	0b 000 100 100
**	83	|	0b 011 111 111
**
**	Es. LOGARITHMIC HUFFMAN
**	----------------------
**	LogHuf start_depth = 1, inc_depth = 1
**	increase by 1 every block
**	0	|	0b 0
**	1	|	0b 00 1
**	2	|	0b 01 1
**	3	|	0b 000 10 1
**	10	|	0b 011 11 1				(G2 + G1 + B7)
**	11	|	0b 0000 100 10 1 		(G8 + G2 + G1 + B0)
**	74	|	0b 0111 111 11 1		(G8 + G2 + G1 + B63)
**	75	|	0b 00000 1000 100 10 1	(G64 + G8 + G2 + G1 + B0)
**
**	Es. LOGARITHMIC HUFFMAN
**	----------------------
**	LogHuf start_depth = 2, inc_depth = 2
**	increase by 2 every block
**	0	|	0b 00
**	1	|	0b 01
**	2	|	0b 0000 10
**	17	|	0b 0111 11
**	18	|	0b 000000 1000 10
**	529	|	0b 011111 1111 11
**
****************************************************************************/

/****************************************************************************
**	KNOWN BUG
*****************************************************************************
**	r1v0.1Alpha - BUG1
**	>FORMULAE: the bit grup MSB increase value
**	ex: 6 -> 00 10 10 -> Z2B2 G1B1 G0B0 [G1=2^2*(depth-1), G0=2^(depth-1)] + [B2B1B0]
**	so, at depth = 1 the block MSB value is always 2^k*0 = 1
**	-------------------------------------------------------------------------
**	r2v1.0 - BUG2
**	When write 1h2 then 2h2, instead will write 3h2.
**	It was caused by a major error in the bit clear and set algorithm, i ended
**	when the number was 0, but doing so, i could let some bit to '1'
**	Now the check is done until the bit are ended
**	-------------------------------------------------------------------------
**	r2v1.0 - BUG3 - HUF_SIZE16
**	with the string 0 - 4 - 0 -2 - 0 - 2 HDEPTH=2 the function return the wrong
**	size for the first 2
**
****************************************************************************/

/****************************************************************************
**	INCLUDE
****************************************************************************/

#include "huffman.h"

/****************************************************************************
**	GLOBAL VARIABILE
****************************************************************************/

/****************************************************************************
**	FUNCTION
****************************************************************************/

/****************************************************************************
**	HUF_MAX8
*****************************************************************************
**	Maximum number that can be expressed with a huffman code of given depth
**	and group number
**	the function cap at MAX_UINT8
****************************************************************************/

uint8_t huf_max8( uint8_t depth, uint8_t num_group )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint8_t t;

	uint8_t tmp8;

	uint8_t max8;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if ( (num_group == 0) || (depth < 1) || (depth > HUFFMAN_MAX_DEPTH) )
	{
		return 0;
	}

	///***************************************************************
	/// ALGORITHM
	///***************************************************************

	//if: depth == 1
	if (depth == 1)
	{
		max8 = num_group -1;
	}	//end if: depth == 1
	//if: depth > 1
	else
	{
		//clear the max
		max8 = 0;
		//for: every group
		for (t = 1;t <= num_group;t++)
		{
			//if: the weight is not too high
			if ( (t*(depth-1)) < 8)
			{
				//Weight of the group MSB
				tmp8 = (1 << (t*(depth-1)));
			}	//end if:
			//if: the weight is too high
			else
			{
				//cap the maximum
				max8 = 255;

				return max8;
			}

			//I will subtract the 1 in the formula in the first group
			if (t == 1)
			{
				tmp8--;
			}

			//if: the sum would not cap
			if ( max8 < (255 - tmp8) )
			{
				//return the sum
				max8 = max8 + tmp8;
			}
			//if: the sum would cap
			else
			{
				//return cap
				max8 = 255;
			}
			//debug
			//printf(" depth: %d, group: %d, weigth: %d, max8: %d\n", depth, t, tmp8, max8);
		}	//end for: every group
	}	//end if: depth > 1

	///***************************************************************
	/// RETURN
	///***************************************************************

	return max8;
} //end function: huf_max8

/****************************************************************************
**	HUF_MAX16
*****************************************************************************
**	Maximum number that can be expressed with a huffman code of given depth
**	and group number
**	the function cap at MAX_UINT16
****************************************************************************/

uint16_t huf_max16( uint8_t depth, uint8_t num_group )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint8_t t;

	uint16_t tmp16;

	uint16_t max16;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if ( (num_group == 0) || (depth < 1) || (depth > HUFFMAN_MAX_DEPTH) )
	{
		return 0;
	}

	///***************************************************************
	/// ALGORITHM
	///***************************************************************

	//if: depth == 1
	if (depth == 1)
	{
		max16 = num_group -1;
	}	//end if: depth == 1
	//if: depth > 1
	else
	{
		//clear the max
		max16 = 0;
		//for: every group
		for (t = 1;t <= num_group;t++)
		{
			//if: the weight is not too high
			if ( (t*(depth-1)) < 16)
			{
				//Weight of the group MSB
				tmp16 = (1 << (t*(depth-1)));
			}	//end if:
			//if: the weight is too high
			else
			{
				//cap the maximum
				max16 = 65535;

				return max16;
			}

			//I will subtract the 1 in the formula in the first group
			if (t == 1)
			{
				tmp16--;
			}

			//if: the sum would not cap
			if ( max16 < (65535 - tmp16) )
			{
				//return the sum
				max16 = max16 + tmp16;
			}
			//if: the sum would cap
			else
			{
				//return cap
				max16 = 65535;
			}
			//debug
			//printf(" depth: %d, group: %d, weigth: %d, max8: %d\n", depth, t, tmp8, max8);
		}	//end for: every group
	}	//end if: depth > 1

	///***************************************************************
	/// RETURN
	///***************************************************************

	return max16;
} //end function: huf_max16

/****************************************************************************
**	HUF SIZE8
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**		return the number of group needed to rapresent an 8 bit number with depth
****************************************************************************/

uint8_t huf_size8( uint8_t num8, uint8_t depth )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	// number of bit group
	uint8_t num_group;

	uint8_t max8;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//	Calculate the number of bit group needed to rapresent the number
	//	D = Depth of the huffman encoding
	//	N = number of group
	//	B = number of binary bit
	//----------------------------------------------------------------
	// Nmax in function of Depth and Number of bit group
	//	D == 1								//Depth 1 mean that no binary bit are present
	//		Nmax 	= N -1					//Account for the MSB of the bit group
	//	D >= 2
	//		B = (D -1) * N					//Number of binary bit present
	//		Nmax 	= (N -1) * 2^(D -1) +	//Account for the MSB of the bit group
	//				+ (2 ^ B) -1			//account for the value of the binary bit
	//----------------------------------------------------------------
	//	Number of group in function of depth and number
	//	D == 1
	//		N = Nmax + 1
	//	D >= 2
	//

	if (depth == 1)
	{
		//I use the inverse formula to calculate the group number
		num_group = num8 + 1;
	}
	else
	{
		//initialise to the minimum group number
		num_group = 0;
		//Instead of reversing that trascendent formula, i try value until i get the good one :)
		do
		{
			//increase the number of group
			num_group++;
			//Calculate the maximum number that can be rapresented in num_group
			max8 = huf_max8( depth, num_group );
		}
		//increase the group number until i can express a number greater or equal then the input number
		while (max8 < num8);

		#ifdef HUFFMAN_DEBUG
			printf("max: %d\n", max8);
		#endif
	}

	///***************************************************************
	/// RETURN
	///***************************************************************

	return num_group;
} //end function: huf_size8

/****************************************************************************
**	HUF SIZE16
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**		return the number of group needed to rapresent a 16 bits number with depth
****************************************************************************/

uint8_t huf_size16( uint16_t num16, uint8_t depth )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	// number of bit group
	uint8_t num_group;

	uint16_t max16;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//	Calculate the number of bit group needed to rapresent the number
	//	D = Depth of the huffman encoding
	//	N = number of group
	//	B = number of binary bit
	//----------------------------------------------------------------
	// Nmax in function of Depth and Number of bit group
	//	D == 1								//Depth 1 mean that no binary bit are present
	//		Nmax 	= N -1					//Account for the MSB of the bit group
	//	D >= 2
	//		B = (D -1) * N					//Number of binary bit present
	//		Nmax 	= (N -1) * 2^(D -1) +	//Account for the MSB of the bit group
	//				+ (2 ^ B) -1			//account for the value of the binary bit
	//----------------------------------------------------------------
	//	Number of group in function of depth and number
	//	D == 1
	//		N = Nmax + 1
	//	D >= 2
	//

	if (depth == 1)
	{
		//I use the inverse formula to calculate the group number
		num_group = num16 + 1;
	}
	else
	{
		//initialise to the minimum group number
		num_group = 0;
		//Instead of reversing that trascendent formula, i try value until i get the good one :)
		do
		{
			//increase the number of group
			num_group++;
			//Calculate the maximum number that can be rapresented in num_group
			max16 = huf_max16( depth, num_group );
		}
		//increase the group number until i can express a number greater or equal then the input number
		while (max16 < num16);

		#ifdef HUFFMAN_DEBUG
			printf("max: %d\n", max16);
		#endif
	}

	///***************************************************************
	/// RETURN
	///***************************************************************

	return num_group;
} //end function: huf_size16

/****************************************************************************
**	HUF SKIP
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**	Skip a number without reading it, return the number of readed bit (useful to fast seek a number)
****************************************************************************/

uint8_t huf_skip( uint8_t *str, uint8_t depth, uint16_t start )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint8_t start_bit;
	//bit index
	uint8_t bit_index;
	//index of the next group MSB
	uint8_t next_bit_index;
	//temp pointer
	uint8_t *ptr8;

	uint8_t continue_flag;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if ( (str == NULL) || (depth < 1) || (depth >= HUFFMAN_MAX_DEPTH) )
	{
		return 255;
	}

	///***************************************************************
	/// DECODE START
	///***************************************************************
	//	Decode the bit start information in a new pointer to the first
	//	byte and a bit index

	//pointer to the first useful byte
	ptr8 = &str[ start / 8 ];
	//index of the first useful bit
	start_bit = start & 7;

	#ifdef HUFFMAN_DEBUG
		//first useful byte
		printf("start byte: %d\n", start / 8);
		//first useful bit
		printf("start bit: %d\n", start_bit);
	#endif

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//  >The bit is a group MSB?
	//	N:
	//		decode it as a ninary bit
	//	Y:
	//		is 0?
	//		Y:
	//			decode complete
	//		N:
	//			decode it as a group MSB

	//initialise
	bit_index = 0;
	//initialise the next MSB group it position
	next_bit_index = depth -1;
	//rise the continue flag
	continue_flag = 0x01;
	//
	while ( continue_flag )
	{
		//Is this a group MSB bit?
		//es. 001 100 111
		//    ^   ^   ^
		if (bit_index == next_bit_index)
		{
			//calculate the next group MSB bit
			next_bit_index += depth;
			//Is the group MSB '1'?
			//es. 001 100 111
			//        ^   ^
			if ( ptr8[ ((start_bit + bit_index) / 8) ] & (1 << ((start_bit + bit_index) & 7) ) )
			{
				//do nothing
			}
			//The group MSB is '0'
			//es. 001 100 111
			//    ^
			else
			{
				//The number is completed
				continue_flag = 0;
			}
		}
		//This is a binary bit
		//es. 001 100 111
		//     ^^  ^^  ^^
		else
		{
			//do nothing
		}

		//jump to the next bit
		bit_index++;
	}	//end while:

	///***************************************************************
	/// RETURN
	///***************************************************************

	//return the number of processed bit
	return bit_index;
} //end function: huf_skip

/****************************************************************************
**	HUF_ENC8
*****************************************************************************
**	Append an encoded number to a source string, start bit force the parser to
**	jump bit (speed up the encoding) return the number of bit added to the string
**
**	str:		pointer to the start of the string
**	str_end:	pointer to the last byte of the string, allow for overflow handling
**				if NULL, will skip the string end check
**	num:		number to be added to the string
**	depth:		depth of the huffman encoding
**	start_bit:	number of bit to be skipped from the start of the string
**
**	return:
**	255: error
**	anything else is the number of added bit
**	This function encode an 8 bit number as a huffman bit stream
**	This structure hold a huffman bit stream,
**	Ex
**	D = Depth
**	MSB...LSB
**
**	NUM		| D = 1					| D = 2				| D = 3
**	0b0		| 0b 0					| 0b 00				| 0b 000
**	0b1		| 0b 0 1				| 0b 01				| 0b 001
**	0b10	| 0b 0 1 1				| 0b 00 10			| 0b 010
**	0b11	| 0b 0 1 1 1			| 0b 00 11			| 0b 011
**	0b100	| 0b 0 1 1 1 1			| 0b 01 10	 		| 0b 000 100
**	0x101	| 0b 0 1 1 1 1 1		| 0b 01 11			| 0b 000 101
**	0x110	| 0b 0 1 1 1 1 1 1		| 0b 00 10 10		| 0b 000 110
**	0x111	| 0b 0 1 1 1 1 1 1 1	| 0b 00 10 11		| 0b 000 111
**	0x1000	| 0b 0 1 1 1 1 1 1 1 1	| 0b 00 11 10		| 0b 001 100
**	0x1001	| 0b 0 1 1 1 1 1 1 1 1 1| 0b 00 11 11		| 0b 001 100
**
**	n = index of the last group
**	G(x) = '1' //MSB of group x (not the last group
**	Z(x) = '0' //MSB of the last group
**
**	H1 = Z(n) G(n-1) ... G(1) G(0)
**
**	H2 = Z(n)B(n) G(n-1)B(n-1) ... G(1)B(1) G(0)B(0)
**
**	H3 = Z(n)B(2n+1)B(2n+0) G(n-1)B(2(n-1)+1)B(2(n-1)+0) ... G(1)B(3)B(2) G(0)B(1)B(0)
**
**	ex: D = 3
**	19 = 4 + 15 -> 011 111
**	20 = 16 + 4 + 0 -> 000 100 100
**
**	//Maximum number in function of bit group and depth of the encoding
**	D = 1
**	Nmax( N, D ) = N * 2^(D-1)
**	D >= 2 (BUG FORMULAE!!)
**	Nmax( N, D ) = N * 2^(D-1) + 2^[(D-1)*N + (D-2) ] -1
**	D >= 2
**	2^(D-1) + 2^(2*(D-1)) + 2^(3*(D-1))
**	Nmax( N, D ) = N * 2^(D-1) + 2^[(D-1)*N + (D-2) ] -1
**
****************************************************************************/

uint8_t huf_enc8( uint8_t *str, uint8_t num8, uint8_t depth, uint16_t start )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint8_t start_bit;
	//Number of bit group
	uint8_t num_group;
	//Number of meaningful bit in the stream
	uint8_t num_bit;
	//Number of byte in the string
	uint8_t str_length;

	uint8_t tmp8, tmp8_1, t, mask;
	//Temp pointer
	uint8_t *ptr8 = NULL;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if (str == NULL)
	{
		HUFFMAN_ERROR_HANDLER( 0xff );

		return 0xff;
	}

	if ( (depth < 1) || (depth > HUFFMAN_MAX_DEPTH) )
	{
		HUFFMAN_ERROR_HANDLER( 0xff );

		return 0xff;
	}

	if ( num8 >= 255 )
	{
		HUFFMAN_ERROR_HANDLER( 0xff );

		return 0xff;
	}

	#ifdef HUFFMAN_DEBUG
		//Calculated number of bit group
		printf( "num8 = %d\n", num8);
	#endif

	///***************************************************************
	///	BIT GROUP NUMBER CALCULATION
	///***************************************************************

	//obtain the number of group needed to rapresent the number
	num_group = huf_size8( num8, depth );

	#ifdef HUFFMAN_DEBUG
		//Calculated number of bit group
		printf( "num_group = %d\n", num_group);
	#endif

	///***************************************************************
	/// CALCULATE THE STRING LENGTH
	///***************************************************************
	//	The string contain the bit stream
	//	str_length = round_up( (start_bit + num_group * depth) /8 )

	//Calculate the number of meaningful bit
	num_bit = num_group * depth;
	//Calculate the total number of bit
	tmp8 = start + num_bit;
	//if the division would leave a rest
	if (tmp8 & 0x07)
	{
		//divide and round up
		str_length = (tmp8 >> 3) + 1;
	}
	//if the division would leave no rest (perfect fit)
	else
	{
		//divide only
		str_length = (tmp8 >> 3);
	}

	#ifdef HUFFMAN_DEBUG
		//Calculate the number of meaningful bit
		printf( "num_bit = %d\n", num_bit);
		//Calculated number of bit group
		printf( "str_length = %d\n", str_length);
	#endif

	///***************************************************************
	///	CALCULATE POINTER AND OFFSET
	///***************************************************************
	//	Resolbe the start parameter and calculate the pointer to the
	//	byte and the bit index

	//link the pointer to the first byte to be used
	ptr8 = &str[ start / 8 ];
	//calculate the bit offset of the byte
	start_bit = start & 7;

	#ifdef HUFFMAN_DEBUG
		//Start byte
		printf( "start byte = %d\n", start / 8);
		//Start bit
		printf( "start bit = %d\n", start_bit);
	#endif

	///***************************************************************
	///	HUFFMAN ENCODER
	///***************************************************************
	//	Now i have to write the huffman code inside the string
	//	Algorithm:
	//	>Subtract the number by the MSB of the block
	//	>Set the MSB of the block in the bit stream
	//	>move the bit from the input number to the bit stream
	//

	if (depth == 1)
	{
		num8 = 0;
	}
	else
	{
		//Subtract the number by the number of group weigthed by the depth
		for (t = 1;t < num_group;t++)
		{
			num8 = num8 - (1 << (t*(depth-1)));
		}
	}

	#ifdef HUFFMAN_DEBUG
		//num8
		printf( "num8 = %d\n", num8);
	#endif

	//Now i can copy the bit content to the bit stream at the appropriate position
	//start from the first bit of the num
	mask = 0;
	//start from the first bit of the bit stream
	tmp8 = 0;
	//index of the next block MSB (i have to jump it)
	tmp8_1 = depth -1;
	//While: i haven't reached the end
	while ( tmp8 < (num_bit -1) )
	{
		//If I'm in a block MSB
		if (tmp8 == tmp8_1)
		{
			//Set the block MSB (i don't need to, i already did it)
			SET_BIT( ptr8[ ((start_bit + tmp8) >> 3) ], ((start_bit + tmp8) & 0x07) );

			#ifdef HUFFMAN_DEBUG
				printf( "SET GROUP MSB bit: %d byte: %d\n", ((start_bit + tmp8) & 0x07), ((start_bit + tmp8) >> 3) );
			#endif
			//jump to the next block MSB
			tmp8_1 = tmp8_1 + depth;
			//jump to the next string bit
			tmp8++;
		}
		//If i'm not in a block MSB
		else
		{
			//If the bit is '1'
			if ( num8 & (0x01 << mask) )
			{
				//Set the bit in the bit stream
				SET_BIT( ptr8[ ((start_bit + tmp8) >> 3) ], ((start_bit + tmp8) & 0x07) );

				#ifdef HUFFMAN_DEBUG
					printf( "SET BINARY BIT bit: %d byte: %d\n", ((start_bit + tmp8) & 0x07), ((start_bit + tmp8) >> 3) );
				#endif
			}
			//If the bit is '0'
			else
			{
				//Set the bit in the bit stream
				CLEAR_BIT( ptr8[ ((start_bit + tmp8) >> 3) ], ((start_bit + tmp8) & 0x07) );

				#ifdef HUFFMAN_DEBUG
					printf( "CLEAR BINARY BIT bit: %d byte: %d\n", ((start_bit + tmp8) & 0x07), ((start_bit + tmp8) >> 3) );
				#endif
			}

			//jump to the next num bit
			mask++;
			//jump to the next string bit
			tmp8++;
		}
	}	//end while:i haven't reached the end

	//Clear final group MSB
	CLEAR_BIT( ptr8[ ((start_bit + tmp8) >> 3) ], ((start_bit + tmp8) & 0x07) );

	#ifdef HUFFMAN_DEBUG
		printf( "CLEAR GROUP MSB bit: %d byte: %d\n", ((start_bit + tmp8) & 0x07), ((start_bit + tmp8) >> 3) );
	#endif

	///***************************************************************
	/// RETURN
	///***************************************************************

	return num_bit;
} //end function: huf_enc8

/****************************************************************************
**	HUF_ENC16
*****************************************************************************
**	Append an encoded number to a source string, start bit force the parser to
**	jump bit (speed up the encoding) return the number of bit added to the string
****************************************************************************/

uint8_t huf_enc16( uint8_t *str, uint16_t num16, uint8_t depth, uint16_t start )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint8_t start_bit;
	//Number of bit group
	uint8_t num_group;
	//Number of meaningful bit in the stream
	uint8_t num_bit;
	//Number of byte in the string
	uint8_t str_length;

	uint8_t tmp8, tmp8_1, t, mask;
	//Temp pointer
	uint8_t *ptr8 = NULL;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if (str == NULL)
	{
		HUFFMAN_ERROR_HANDLER( 0xff );

		return 0xff;
	}

	if ( (depth < 1) || (depth > HUFFMAN_MAX_DEPTH) )
	{
		HUFFMAN_ERROR_HANDLER( 0xff );

		return 0xff;
	}

	if ( num16 >= 65535)
	{
		HUFFMAN_ERROR_HANDLER( 0xff );

		return 0xff;
	}

	#ifdef HUFFMAN_DEBUG
		//Calculated number of bit group
		printf( "num16 = %d\n", num16);
	#endif

	///***************************************************************
	///	BIT GROUP NUMBER CALCULATION
	///***************************************************************

	//obtain the number of group needed to rapresent the number
	num_group = huf_size16( num16, depth );

	#ifdef HUFFMAN_DEBUG
		//Calculated number of bit group
		printf( "num_group = %d\n", num_group);
	#endif

	///***************************************************************
	/// CALCULATE THE STRING LENGTH
	///***************************************************************
	//	The string contain the bit stream
	//	str_length = round_up( (start_bit + num_group * depth) /8 )

	//Calculate the number of meaningful bit
	num_bit = num_group * depth;
	//Calculate the total number of bit
	tmp8 = start + num_bit;
	//if the division would leave a rest
	if (tmp8 & 0x07)
	{
		//divide and round up
		str_length = (tmp8 >> 3) + 1;
	}
	//if the division would leave no rest (perfect fit)
	else
	{
		//divide only
		str_length = (tmp8 >> 3);
	}

	#ifdef HUFFMAN_DEBUG
		//Calculate the number of meaningful bit
		printf( "num_bit = %d\n", num_bit);
		//Calculated number of bit group
		printf( "str_length = %d\n", str_length);
	#endif

	///***************************************************************
	///	CALCULATE POINTER AND OFFSET
	///***************************************************************
	//	Resolbe the start parameter and calculate the pointer to the
	//	byte and the bit index

	//link the pointer to the first byte to be used
	ptr8 = &str[ start / 8 ];
	//calculate the bit offset of the byte
	start_bit = start & 7;

	#ifdef HUFFMAN_DEBUG
		//Start byte
		printf( "start byte = %d\n", start / 8);
		//Start bit
		printf( "start bit = %d\n", start_bit);
	#endif

	///***************************************************************
	///	HUFFMAN ENCODER
	///***************************************************************
	//	Now i have to write the huffman code inside the string
	//	Algorithm:
	//	>Subtract the number by the MSB of the block
	//	>Set the MSB of the block in the bit stream
	//	>move the bit from the input number to the bit stream
	//

	if (depth == 1)
	{
		num16 = 0;
	}
	else
	{
		//Subtract the number by the number of group weigthed by the depth
		for (t = 1;t < num_group;t++)
		{
			num16 = num16 - (1 << (t*(depth-1)));
		}
	}

	#ifdef HUFFMAN_DEBUG
		//num16
		printf( "num16 = %d\n", num16);
	#endif

	//Now i can copy the bit content to the bit stream at the appropriate position
	//start from the first bit of the num
	mask = 0;
	//start from the first bit of the bit stream
	tmp8 = 0;
	//index of the next block MSB (i have to jump it)
	tmp8_1 = depth -1;
	//While: i haven't reached the end
	while ( tmp8 < (num_bit -1) )
	{
		//If I'm in a block MSB
		if (tmp8 == tmp8_1)
		{
			//Set the block MSB (i don't need to, i already did it)
			SET_BIT( ptr8[ ((start_bit + tmp8) >> 3) ], ((start_bit + tmp8) & 0x07) );

			#ifdef HUFFMAN_DEBUG
				printf( "SET GROUP MSB bit: %d byte: %d\n", ((start_bit + tmp8) & 0x07), ((start_bit + tmp8) >> 3) );
			#endif
			//jump to the next block MSB
			tmp8_1 = tmp8_1 + depth;
			//jump to the next string bit
			tmp8++;
		}
		//If i'm not in a block MSB
		else
		{
			//If the bit is '1'
			if ( num16 & (0x01 << mask) )
			{
				//Set the bit in the bit stream
				SET_BIT( ptr8[ ((start_bit + tmp8) >> 3) ], ((start_bit + tmp8) & 0x07) );

				#ifdef HUFFMAN_DEBUG
					printf( "SET BINARY BIT bit: %d byte: %d\n", ((start_bit + tmp8) & 0x07), ((start_bit + tmp8) >> 3) );
				#endif
			}
			//If the bit is '0'
			else
			{
				//Set the bit in the bit stream
				CLEAR_BIT( ptr8[ ((start_bit + tmp8) >> 3) ], ((start_bit + tmp8) & 0x07) );

				#ifdef HUFFMAN_DEBUG
					printf( "CLEAR BINARY BIT bit: %d byte: %d\n", ((start_bit + tmp8) & 0x07), ((start_bit + tmp8) >> 3) );
				#endif
			}

			//jump to the next num bit
			mask++;
			//jump to the next string bit
			tmp8++;
		}
	}	//end while:i haven't reached the end

	//Clear final group MSB
	CLEAR_BIT( ptr8[ ((start_bit + tmp8) >> 3) ], ((start_bit + tmp8) & 0x07) );

	#ifdef HUFFMAN_DEBUG
		printf( "CLEAR GROUP MSB bit: %d byte: %d\n", ((start_bit + tmp8) & 0x07), ((start_bit + tmp8) >> 3) );
	#endif

	///***************************************************************
	/// RETURN
	///***************************************************************

	return num_bit;
} //end function: huf_enc16

/****************************************************************************
**	HUF_DEC8
*****************************************************************************
**	Decode a uint8_t number from a str string, load the result in the dest
**	variabile and return the number of readed bit
**
**	Return code:
**	255 mean that an error occurred
**	every other code mean that is ok
**
**	Dest:
**	255 mean that an overflow occurred ( the variabile was bigger than uint8)
**	every other code mean is ok
****************************************************************************/

uint8_t huf_dec8( uint8_t *str, uint8_t *dest, uint8_t depth, uint16_t start )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint8_t start_bit;
	//bit index
	uint8_t bit_index;
	//index of the next group MSB
	uint8_t next_bit_index;
	//Index of the group
	uint8_t group_index;
	//temp pointer
	uint8_t *ptr8;

	uint8_t num8;

	uint8_t continue_flag;
	uint8_t overflow_flag;

	uint8_t tmp8;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if ( (str == NULL) || (dest == NULL) || (depth < 1) || (depth >= HUFFMAN_MAX_DEPTH) )
	{
		return 255;
	}

	///***************************************************************
	/// DECODE START
	///***************************************************************
	//	Decode the bit start information in a new pointer to the first
	//	byte and a bit index

	//pointer to the first useful byte
	ptr8 = &str[ start / 8 ];
	//index of the first useful bit
	start_bit = start & 7;

	#ifdef HUFFMAN_DEBUG
		//first useful byte
		printf("start byte: %d\n", start / 8);
		//first useful bit
		printf("start bit: %d\n", start_bit);
	#endif

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//  >The bit is a group MSB?
	//	N:
	//		decode it as a ninary bit
	//	Y:
	//		is 0?
	//		Y:
	//			decode complete
	//		N:
	//			decode it as a group MSB

	//clear the number
	num8 = 0;
	//initialise
	bit_index = 0;
	group_index = 0;
	//initialise the next MSB group it position
	next_bit_index = depth -1;
	//rise the continue flag
	continue_flag = 0x01;
	//clear the overflow flag
	overflow_flag = 0x00;
	//
	while ( continue_flag )
	{
		//Is this a group MSB bit?
		//es. 001 100 111
		//    ^   ^   ^
		if (bit_index == next_bit_index)
		{
			//calculate the next group MSB bit
			next_bit_index += depth;
			//Is the group MSB '1'?
			//es. 001 100 111
			//        ^   ^
			if ( ptr8[ ((start_bit + bit_index) / 8) ] & (1 << ((start_bit + bit_index) & 7) ) )
			{
				//increase the number by the binary weigth of the group MSB
				tmp8 = (1 << ((group_index +1)*(depth -1)) );
				//Would not generate overflow?
				if ( (overflow_flag == 0x00) && (num8 < 255 - tmp8) )
				{
					//sum the number
					num8 += tmp8;
				}
				else
				{
					num8 = 255;

					overflow_flag = 0x01;
				}

			}
			//The group MSB is '0'
			//es. 001 100 111
			//    ^
			else
			{
				//The number is completed
				continue_flag = 0;
			}

			//Update the number of group
			group_index++;
		}
		//This is a binary bit
		//es. 001 100 111
		//     ^^  ^^  ^^
		else
		{
			//is the binary bit '1'?
			//es. 001 100 111
			//      ^      ^^
			if ( ptr8[ ((start_bit + bit_index) / 8) ] & (1 << ((start_bit + bit_index) & 7) ) )
			{
				//increase the number by the binary weigth of the bit
				tmp8 = (1 << ( bit_index -group_index) );
				//Would not generate overflow?
				if ( (overflow_flag == 0x00) && (num8 < 255 - tmp8) )
				{
					//sum the number
					num8 += tmp8;
				}
				else
				{
					num8 = 255;

					overflow_flag = 0x01;
				}
			}
			//es. 001 100 111
			//     ^   ^^
			else
			{
				//do nothing
			}
		}

		#ifdef HUFFMAN_DEBUG
			//decoded number
			printf("num8: %d\n", num8);
		#endif

		//jump to the next bit
		bit_index++;
	}	//end while:

	///***************************************************************
	/// SAVE DECODED NUM
	///***************************************************************

	dest[ 0 ] = num8;

	///***************************************************************
	/// RETURN
	///***************************************************************

	//return the number of processed bit
	return bit_index;
} //end function: huf_dec8

/****************************************************************************
**	HUF_DEC16
*****************************************************************************
**	Decode a uint16_t number from a str string, load the result in the dest
**	variabile and return the number of readed bit
**
**	Return code:
**	255 mean that an error occurred
**	every other code mean that is ok
**
**	Dest:
**	65535 mean that an overflow occurred ( the variabile was bigger than uint16)
**	every other code mean is ok
****************************************************************************/

uint8_t huf_dec16( uint8_t *str, uint16_t *dest, uint8_t depth, uint16_t start )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint8_t start_bit;
	//bit index
	uint8_t bit_index;
	//index of the next group MSB
	uint8_t next_bit_index;
	//Index of the group
	uint8_t group_index;
	//temp pointer
	uint8_t *ptr8;

	uint16_t num16;

	uint8_t continue_flag;
	uint8_t overflow_flag;

	uint16_t tmp16;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if ( (str == NULL) || (dest == NULL) || (depth < 1) || (depth >= HUFFMAN_MAX_DEPTH) )
	{
		return 255;
	}

	///***************************************************************
	/// DECODE START
	///***************************************************************
	//	Decode the bit start information in a new pointer to the first
	//	byte and a bit index

	//pointer to the first useful byte
	ptr8 = &str[ start / 8 ];
	//index of the first useful bit
	start_bit = start & 7;

	#ifdef HUFFMAN_DEBUG
		//first useful byte
		printf("start byte: %d\n", start / 8);
		//first useful bit
		printf("start bit: %d\n", start_bit);
	#endif

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//  >The bit is a group MSB?
	//	N:
	//		decode it as a ninary bit
	//	Y:
	//		is 0?
	//		Y:
	//			decode complete
	//		N:
	//			decode it as a group MSB

	//clear the number
	num16 = 0;
	//initialise
	bit_index = 0;
	group_index = 0;
	//initialise the next MSB group it position
	next_bit_index = depth -1;
	//rise the continue flag
	continue_flag = 0x01;
	//clear the overflow flag
	overflow_flag = 0x00;
	//
	while ( continue_flag )
	{
		//Is this a group MSB bit?
		//es. 001 100 111
		//    ^   ^   ^
		if (bit_index == next_bit_index)
		{
			//calculate the next group MSB bit
			next_bit_index += depth;
			//Is the group MSB '1'?
			//es. 001 100 111
			//        ^   ^
			if ( ptr8[ ((start_bit + bit_index) / 8) ] & (1 << ((start_bit + bit_index) & 7) ) )
			{
				//increase the number by the binary weigth of the group MSB
				tmp16 = (1 << ((group_index +1)*(depth -1)) );
				//Would not generate overflow?
				if ( (overflow_flag == 0x00) && (num16 < 65535 - tmp16) )
				{
					//sum the number
					num16 += tmp16;
				}
				else
				{
					num16 = 65535;

					overflow_flag = 0x01;
				}

			}
			//The group MSB is '0'
			//es. 001 100 111
			//    ^
			else
			{
				//The number is completed
				continue_flag = 0;
			}

			//Update the number of group
			group_index++;
		}
		//This is a binary bit
		//es. 001 100 111
		//     ^^  ^^  ^^
		else
		{
			//is the binary bit '1'?
			//es. 001 100 111
			//      ^      ^^
			if ( ptr8[ ((start_bit + bit_index) / 8) ] & (1 << ((start_bit + bit_index) & 7) ) )
			{
				//increase the number by the binary weigth of the bit
				tmp16 = (1 << ( bit_index -group_index) );
				//Would not generate overflow?
				if ( (overflow_flag == 0x00) && (num16 < 65535 - tmp16) )
				{
					//sum the number
					num16 += tmp16;
				}
				else
				{
					num16 = 65535;

					overflow_flag = 0x01;
				}
			}
			//es. 001 100 111
			//     ^   ^^
			else
			{
				//do nothing
			}
		}

		#ifdef HUFFMAN_DEBUG
			//decoded number
			printf("num16: %d\n", num16);
		#endif

		//jump to the next bit
		bit_index++;
	}	//end while:

	///***************************************************************
	/// SAVE DECODED NUM
	///***************************************************************

	dest[ 0 ] = num16;

	///***************************************************************
	/// RETURN
	///***************************************************************

	//return the number of processed bit
	return bit_index;
} //end function: huf_dec16
