/*****************************************************************
**	R@P PROJECT
******************************************************************
**	AT_MALLOC_H
******************************************************************
**	Descrizione:
**	Libreria che implementa la gestione dinamica della memoria per
**	sistemi che non dispongono di malloc, l'idea è di allocare
**	un vettore e di gestirlo con funzioni come se ci fosse dietro un SO
**	la quantita' massima di memoria gestibile in questo modo è 64 KiB
**	la libreria è progettata in modo da poter essere utilizzata
**	all'interno di un firmware, dove l'SO non esiste
**  La tabella di allocazione è manteenuta all'interno della memoria,
**  la memoria viene allocata a partire dall'indice 0x0000 del vettore memory
**  la tabella di allocazione è invece allocata a partire dall'ultimo
**  byte della memoria.
**	V3:
**	>Backward compatibile with the other at_malloc library
**	>I will use the stdint definition
**	>The index will be huffman coded, to be 8 bit long when possibile
**	>Global flag added to signal error event
**	>Added function:
**		>at_malloc:		return a void pointer to a location of n byte
**		>at_free:		free a memory location perviusly allocated with the at_malloc
**		>at_free_all:	free all the memory, initialise the allocation table
**		>at_realloc:	reallocate a location perviusly allocated with the malloc
**						to n-byte, return the void * and copy all the old data
**						in the vector
**
******************************************************************
**	Autore:				Orso Eric
**	Versione:			3.0
**	Revisione:			3
**	Data Creazione:		06/06/10
**	Ultima Modifica:	23/06/10
*****************************************************************/

/*****************************************************************
**	History Version:
******************************************************************
**	>0.1 alfa: >iniziato a scrivere at_malloc e at_free
**  >1.0 beta: >completate e debuggate la at_malloc e la at_free
**	>2.0 beta: >cambiamento dell'algoritmo di allocazione per
**				ottimizzare l'utilizzo della memoria
**	>3.0 beta:
*****************************************************************/

/*****************************************************************
**	ALLOCATION TABLE STRATEGY:
******************************************************************
**	>Block get allocated from the first byte
**	>Allocation table dwell at the bottom of the vector
**	>Data in the allocation table are stored with huffman coding
**	aka. if the MSB is '1', the number continue on the following byte
**		ex: 	0	0x00		8bit
**				15	0x0f		8bit
**				127	0x7f		8bit
**				128 0x0180		16bit
**				255	0x01ff		16bit
**	>The allocation table store the block information
**
**	ex: memory occupation
**	0		3		5		7		9					END
**	|MEM	|MEM	|FRAG	|MEM	|FREE... |A-TABLE	|
**
**	MALLOC:
**		>need to kwnow fragment position and size and free memory size
**	FREE:
**		>need to know block position and size
**
**	STRATEGY:	SAVE ONLY BLOCK INFORMATION
**		>Need to compute fragment data during malloc
**		>Allow for block address control in free
**	STRATEGY:	SAVE ONLY FRAGMET INFORMATION (IMPOSSIBILE TO FREE!)
**		>Unable to check free address data
**		>Need to compute fragment merging
**		>How do i do the free? i need to know the block size or end
**	STRATEGY:	SAVE BOTH BLOCK AND FRAGMENT INFORMATION
**		>BIG allocation table
**		>more data to order in ordered a-table
**		>allow for fastest malloc and free
**	STRATEGY:	ORDERED ALLOCATION TABLE
**		>Allow for relative address storage
**		>Allow for simpler fragment merging
**		>Allocation table need to be ordered after every malloc (shift data left)
**		and free (shift data rigth), cost depend on the number of element to be shifted
**	STRATEGY:	OUT OF ORDER ALLOCATION TABLE
**		>Need absolute index
**		>Using triky relative index would gain nothing from huffman
**		since the relative displacement is not mostly '0'
**		>Save computational power since there is no need to reorder the A-Table
**		>Search need more time since i have to run the whole A-Table all the time
**		>Need more space in the ATable since the addresses will take mor bit
**		>Saving only block information will make extracting fragment information a nightmare
**		>Describing the free memory need a special fragment block or a big address information
**
**	STRATEGY:	FILL FREE MEMORY FIRST:
**		>Faster until i reach the memory limit
**		>May have bigger risk of not finding a fragment big enough
**
**	STRATEGY:	FILL FRAGMENT WHEN POSSIBILE
**		>Harder to compute, even when the memory is half empty
**		>Less risk of not finding a fragment big enough
**
**	SMALLEST A-TABLE SIZE:
**		ORDERED A-TABLE, BLOCK DATA, RELATIVE INDEX and HUFFMAN CODING
**		>need reordering after every malloc and free
**
**	HIGHEST MALLOC AND FREE SPEED:
**		OUT-OF-ORDER A-TABLE, BLOCK & FRAGMENT DATA, ABSOLUTE INDEX, NO ENCODING
**		>i have the highest overhead
**		>i can maximize the malloc and free speed
**
**	--------------------------------------------------------------
**		STRATEGY: ABSOLUTE OFFSET (Fragment Start, Fragment Stop)
**	--------------------------------------------------------------
**	PROS:
**		Fastest computation
**
**	--------------------------------------------------------------
**		STRATEGY: RELATIVE OFFSET (Block Start, Block Size/Stop)
**	--------------------------------------------------------------
**	PROS:
**		Save A-Table size, the saving is extreme in case of contiguous block
**	CONS:
**		Require more calculation, need huffman coding to fully exploit the saving
**
**		ex: allocation table
**	Mem che inizia 0Byte dopo l'inizio della memoria, di dimensione 3B
**	Mem che inizia 0B dopo il blocco precedente
*****************************************************************/

/*****************************************************************
**	Bug List:
******************************************************************
**	>
*****************************************************************/

/*****************************************************************
**	ToDo:
******************************************************************
**
*****************************************************************/

#ifndef AT_MALLOC_H
	#define AT_MALLOC_H

	/*****************************************************************
	**	ENVROIMENT VARIABILE
	*****************************************************************/

    //#define AT_MALLOC_DEBUG
    #define ATABLE_DEBUG

	/*****************************************************************
	**	INCLUDE
	*****************************************************************/

    #ifdef AT_MALLOC_DEBUG
        #include <stdio.h>
    #endif

    //Standard integer definition
    #include <stdint.h>

	/*****************************************************************
	**	USER DEFINE
	*****************************************************************/

		///***************************************************************
		///	MEMORY SIZE
		///***************************************************************

	#define AT_MALLOC_MEMORY_SIZE		256

	//Size of the memory to be handled by the at_malloc
	#ifndef AT_MALLOC_MEMORY_SIZE
		#define AT_MALLOC_MEMORY_SIZE		128
		#warning "AT_MALLOC_MEMORY_SIZE set to 128 byte by default, see header for detail"
	#endif

		///***************************************************************
		///	AT_MALLOC CONFIGURATION
		///***************************************************************

	//Enable the huffman encoding of the allocation table
	//	slow down the malloc, but greatly decrease allocation table size
	#define AT_MALLOC_HUFFMAN

	//If the huffman encoding is enabled, define the depth of the encoding
	//	allow to tune the a-table size consumption to the application
	#define AT_MALLOC_HDEPTH 		2

	//Size of the circular buffer used for the ATABLE edit procedure
	#define AT_MALLOC_BUFSIZE		8

	/*****************************************************************
	**	ENUM
	*****************************************************************/

	//Possibile state of the at malloc global status flag
	enum
	{
		AT_MALLOC_OK			= 0,
		AT_MALLOC_UNINITIALISED = 1,
		AT_MALLOC_MEMORY_FULL	= 2,
		AT_MALLOC_ERROR 		= 3,
		AT_MALLOC_PARAM_ERROR	= 4,
		AT_MALLOC_BUFFER_ERROR	= 5
	};

	//Meaning of the bit in the status byte of the malloc FSM
	enum
	{
		AT_MALLOC_FLAG_CONTINUE = 0,	//If rised i can continue executing the FSM
		AT_MALLOC_FLAG_REUSE	= 1,	//If rised, i will reuse an old fragment
		AT_MALLOC_FLAG_RESIZE	= 2,	//If rised, mean that the num_group information in the a-table need to be resized
		AT_MALLOC_FLAG_BNOTF	= 3,	//If rised i am working on a fragment, if cleared i am working on a block
		AT_MALLOC_FLAG_JUMP		= 4, 	//If rised, i am authorising a jump to the next block/fragment
		AT_MALLOC_FLAG_MUPDATE	= 5		//If rised, authorize the update of memindex
	};

	//Stat enumeration
	enum
	{
		AT_MALLOC_USEDMEM 		= 0,	//Total user allocated byte (allocation table not included)
		AT_MALLOC_FREEMEM 		= 1,	//Total free memory, fragmented byte included
		AT_MALLOC_FRAGMEM 		= 2,	//Total fragmented memory
		AT_MALLOC_MAXFRAG 		= 3,	//size of the maximum fragment
		AT_MALLOC_NUMBLOCK 		= 4,	//Number of user allocated block
		AT_MALLOC_ATSIZE		= 5,	//Size of the allocation table
		AT_MALLOC_NUMSTAT 		= 6		//Number of statistic

	};

	/*****************************************************************
	**	DEFINE
	*****************************************************************/

    //Null address
	#ifndef NULL
		#define	NULL				(void *)0
	#endif

	//Number of parameter at the start of the allocation table
	#define AT_MALLOC_ATABLE_PARAM	4
	//number of huffman block to be skipped until the start of the block information
	#define AT_MALLOC_ATABLE_SKIP	1

		///***************************************************************
		///	A-TABLE DEFINITION
		///***************************************************************


	/*****************************************************************
	**	DEBUG MACRO
	*****************************************************************/

	#ifdef DEBUG
		//Print a debug string
		#define DEBUG_STR( str )	\
			printf("debug string: %s\n", (str) )

		//Trace the function flow
		#define TRACE( function_name )	\
			printf("trace: %s\n", function_name )
	#else
		//Print a debug string
		#define DEBUG_STR( str )

		//Trace the function flow
		#define TRACE( function_name)
	#endif

	/*****************************************************************
	**	MACRO
	*****************************************************************/

	//Handle error
	#define AT_MALLOC_ERROR_HANDLER( error_code )	\
		at_malloc_status = (error_code)

	//Divide by power of 2 and round up
	//ret = x/2^y
	#define CEILING_DIVIDE( x, y )	\
		( ((x)&((1<<(y))-1)) ? (((x) / (1<<(y))) +1) : ((x) / (1<<(y))) )

	//return the bit size of the number that start in position num16
	#define HUF_SIZE16( num16 )	\
		( AT_MALLOC_HDEPTH * huf_size16( (num16), AT_MALLOC_HDEPTH ) )

	//Encode a 16-bit number in the memory at bit bit_index
	#define HUF_ENC16( num16, bit_index )	\
		huf_enc16( at_malloc_memory, (num16), AT_MALLOC_HDEPTH, (bit_index) )

	//Decode a 16 bit number that start exactly at bit bit_index
	#define HUF_DEC16( num16, bit_index )	\
		huf_dec16( at_malloc_memory, (num16), AT_MALLOC_HDEPTH, (bit_index) )

	//set a single bit inside a variabile lefting the other bit untouched
	#ifndef SET_BIT
		#define SET_BIT( var, shift_value ) \
			( (var) |= (0x01 << shift_value ) )
	#endif

	//clear a single bit inside a variabile lefting the other bit untouched
	#ifndef CLEAR_BIT
		#define CLEAR_BIT( var, shift_value ) \
			( (var) &= (~ (0x01 << shift_value ) ) )
	#endif

	//Is a bit in a variabile '0'
	#ifndef IS_BIT_ZERO
		#define IS_BIT_ZERO( var, shift_value ) \
			(((var) & (1 << (shift_value))) == 0)
	#endif

	//Is a bit in a variabile '1'
	#ifndef IS_BIT_ONE
		#define IS_BIT_ONE( var, shift_value ) \
			(((var) & (1 << (shift_value))) != 0)
	#endif

		///***************************************************************
		/// BUFFER USE
		///***************************************************************

	//increase the bot
	//return buf[ bot -1 ]
	#define BUF_READ( buf, bot, top, size )	\
		( ( (((bot)+1)>=(size)) ? (bot=0): (++(bot)) ), buf[ ( ((bot)==0)?((size)-1):((bot)-1) )] )

	//Peek the number to be readed in the buffer
	#define BUF_PEEK( buf, bot )	\
		(buf[ (bot) ])

	//store data
	//increase top
	#define BUF_WRITE( buf, bot, top, size, data )	\
		(buf[ (top) ]=(data)), ( (((top)+1)>=(size))?(top=0):(++(top)))

	//element in the buffer
	#define BUF_ELEM( bot, top, size )	\
		( ((top)>=(bot)) ? ((top)-(bot)) : ((size)-(bot)+(top)) )

	/*****************************************************************
	**	STRUCTURE PROTOTYPE
	*****************************************************************/

	/*****************************************************************
	**	FUNCTION PROTOTYPE
	*****************************************************************/

	//Allocate a piece of memory of size {size}, return NULL and rise error flag if fail
	extern void *at_malloc( uint16_t size );

	//Same as malloc, but initialise the memory to zero
	extern void *at_calloc( uint16_t size );

	//Resize a piece of memory perviusly allocated with malloc, copying the content (DUMB version implemented)
	extern void *at_realloc( void *address, uint16_t old_size, uint16_t new_size );

	///TODO
	//Create a clone of an allocated piece of memory
	extern void *at_clonalloc( void *address, uint16_t size );

	///TODO
	//Return the size of the allocated memory block, 0 if the block does not exist
	extern uint16_t at_sizalloc( void *address );

	//Free a piece of memory perviusly allocated with malloc or calloc or realloc
	extern void at_free( void *address );

	//Initialise the allocation table, effectivly freeing all the memory,
	extern void at_free_all( void );

	///TO BE COMPLETED
	//Return a vector with the desired information (data index)
	//	0: Total allocated memory
	//	1: Total free memory (fragment included)
	//	2: Total fragmented memory
	//	3: Max fragment size
	//	4: Total allocated block
	//	5: ATable size
	extern uint16_t *at_malloc_stat( uint8_t cfg );

	/*****************************************************************
	**	GLOBAL VARIABILE PROTOTYPE
	*****************************************************************/

	//Memory vector
	extern uint8_t at_malloc_memory[ AT_MALLOC_MEMORY_SIZE ];
	//At malloc status variabile, see enum for detail
	extern uint8_t at_malloc_status;

#else
	#warning "inclusione multipla di at_malloc.h"
#endif
