/*****************************************************************
**	R@P PROJECT
******************************************************************
**	Descrizione:
**
**	MEMORY ORGANISATION:
**	--------------------------------------------------------------
**	--	FIRST BYTE
**	--------------------------------------------------------------
**	FIRST VARIABILE ENCODED IN THE A-TABLE
**	X>user memory		: number of byte allocated {BYTE}
**							This data is useless
**	>num block			: number of allocated block {#}
**							Easy to foresee: malloc will increase it by 1 and free will decrease it by 1. FAIL mean no changhe
**	X>num fragment		: number of fragment with at least 1B {#}
**							This data is useless
**	X>max fragment size	: allow to know if there is a fragment big enough to be recycled {BYTE}
**							I have to calculate all the data anyway so i don't need the max fragment
**	X>free user memory	: size of the memory block between the last block and the end of the allocation table {BYTE}
**							In normal condition is a fairly big number and can be difficult to obtain, i will not use it
**	X>a-table size		: allow to know where the atable end {BIT)
**							Difficult to calculate, the size will depend on the number itself
**	--------------------------------------------------------------
**	A-TABLE
**	>Block 0: start //Distance from the start of the user memory to the start of the first block
**	>Block 0: size	//size of the block
**	>Block 1: start	//Distance from the end of the pervious block to the start of the next block
**	>Block 1: end
**	...
**	--------------------------------------------------------------
**	...
**	{BLOCK1}
**	{FRAGMENT1}
**	{BLOCK0}
**	{FRAGMENT0}
**	--------------------------------------------------------------
**	--	LAST BYTE
**	--------------------------------------------------------------
**
**	EXAMPLE: MALLOC
**	Nb = Num_Block
**	Fx = Fragment {x} size
**	Bx = Block {x} size
**
**	---------------------------------------------------------------
**	STATUS
**	>|Nb=0||...FREEMEM...|<
**	---------------------------------------------------------------
**	>Malloc 10 (First Block allocation)
**	---------------------------------------------------------------
**	STATUS
**	>|Nb=1||F0=0|B0=10|...FREEMEM...|BLOCK0=10|<
**	---------------------------------------------------------------
**	>Malloc 20 (Block allocation in free memory)
**	---------------------------------------------------------------
**	STATUS
**	>|Nb=1||F0=0|B0=10|F1=0|B1=20|...FREEMEM...|BLOCK1=20|BLOCK0=10|<
**	---------------------------------------------------------------
**	>Free B0 (Block Free)
**	---------------------------------------------------------------
**	STATUS
**	>|Nb=1||F0=10|B0=20|...FREEMEM...|BLOCK0=20|FREEMEM=10|<
**	---------------------------------------------------------------
**	>Free B0 (Last block free)
**	---------------------------------------------------------------
**	STATUS
**	>|Nb=0||...FREEMEM...|<
**	---------------------------------------------------------------
**
**
**
*****************************************************************/

/****************************************************************************
**	HYSTORY VERSION
*****************************************************************************
**	r1v0.1 ALPHA
**	>A-Table initialisation OK
**	>at_free_all written and tested
****************************************************************************/

/****************************************************************************
**	BUG LIST
*****************************************************************************
**	R1V0.1 - BUG1 (SOLVED)
**	There is an error in the calculation of the return address which caused a
**	faulty return address in function at_malloc.
**	It was coused by a wrong initialisation of "mem_index" in the atable update
**	FSM
*****************************************************************************
**
****************************************************************************/


/*****************************************************************
**	INCLUDE
*****************************************************************/

#include "at_malloc.h"
#include "huffman.h"

/*****************************************************************
**	GLOBAL VARIABILE
*****************************************************************/

//Memory vector
uint8_t at_malloc_memory[ AT_MALLOC_MEMORY_SIZE ];
//At malloc status variabile, see header for possibile value
uint8_t at_malloc_status = AT_MALLOC_UNINITIALISED;

/*****************************************************************
**	FUNCTION
*****************************************************************/

/****************************************************************************
**	AT_FREE_ALL
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**	Initialise the allocation table, effectivly deleting all the memory
**	perviously allocated
****************************************************************************/

void at_free_all( void )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	//Index of the actual bit
	uint16_t bit_index;
	//
	uint16_t tmp16;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//	Initialise the main parameter of the allocation table
	//	>user memory		: number of byte allocated {BYTE}
	//	>num block			: number of allocated block {#}
	//	>num fragment		: number of fragment with at least 1B {#}
	//	>max fragment size	: allow to know if there is a fragment big enough to be recycled {BYTE}
	//	>free user memory	: size of the memory block between the last block and the end of the allocation table {BYTE}
	//	>a-table size		: allow to know where the atable end {BIT)
	//
	//	NO>USER_MEMORY 		= 0 {byte} //useless
	//	>NUM_BLOCK			= 0 {#}
	//	>NUM_FRAGMENT		= 0 {#}
	//	>MAX_FRAGMENT_SIZE	= 0 {byte}
	//	NO>FREE_USER_MEMORY	= AT_MALLOC_MEMORY_SIZE - CELING( A-TABLE_SIZE /8 ) {byte}
	//	NO>A-TABLE_SIZE		= A-TABLE_SIZE {bit}

	//start from the first bit
	bit_index = 0;

	//NUM BLOCK
	tmp16 = 0;
	bit_index += HUF_ENC16( tmp16, bit_index );
	//Skip the void block
	bit_index += AT_MALLOC_ATABLE_SKIP *AT_MALLOC_HDEPTH;

	#ifdef AT_MALLOC_DEBUG
		printf("bit_index: %d\n", bit_index);
	#endif

	///***************************************************************
	/// THE MEMORY IS NOW INITIALISED
	///***************************************************************

	//set the malloc status variabile
	at_malloc_status = AT_MALLOC_OK;

	///***************************************************************
	/// RETURN
	///***************************************************************

	return;
} //end function: at_free_all

/****************************************************************************
**	AT_MALLOC
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**
****************************************************************************/

void *at_malloc( uint16_t size )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	//FSM Circular buffer, used to update the atable
	uint16_t buf[ AT_MALLOC_BUFSIZE ];
	uint8_t buf_top = 0;
	uint8_t buf_bot = 0;

	//Number of allocated memory block
	uint16_t num_block;
	//ATable READ bit index
	uint16_t rbit_index;
	//ATable WRITE bit index
	uint16_t wbit_index;
	//Absolute memory index (0 point to the last byte)
	uint16_t mem_index;
	//Index of the block
	uint16_t block_index;
	//temp counter
	uint16_t t;
	//temp variabile
	uint16_t tmp16;
	//state machine status variabile (do not need to be static)
	//to see the bit meaning see the header enum
	uint8_t status = 0;

	void *ptr;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	//I cannot allocate 0-size vector
	if (size == 0)
	{
		AT_MALLOC_ERROR_HANDLER( AT_MALLOC_PARAM_ERROR );

		return NULL;
	}

	///***************************************************************
	/// INITIALISE MEMORY
	///***************************************************************
	//	If the status flag is set to {AT_MALLOC_UNINITIALISED} call
	//	the at_free_all() to initialise the allocation table

	//if: the memory is uninitialised
	if (at_malloc_status == AT_MALLOC_UNINITIALISED)
	{
		//Initialise the allocation table
		at_free_all();
	}

	//Block only in case of CRITICAL status error (and recover them
	if (at_malloc_status != AT_MALLOC_ERROR)
	{
		at_malloc_status = AT_MALLOC_OK;
	}

	//if the memory is in error status
	if (at_malloc_status != AT_MALLOC_OK )
	{
		//
		AT_MALLOC_ERROR_HANDLER( at_malloc_status );
		//
		return NULL;
	}

	#ifdef AT_MALLOC_DEBUG
		printf( "-----------------------------AT_MALLOC-------------------------\n" );
	#endif

	///***************************************************************
	/// READ THE MAIN VARIABILES OF THE ALLOCATION TABLE
	///***************************************************************

	rbit_index = 0;
	//Number of allocated memory block
	rbit_index += HUF_DEC16( &num_block, rbit_index );
	//Add the skip block at end
	rbit_index += AT_MALLOC_ATABLE_SKIP *AT_MALLOC_HDEPTH;

	///***************************************************************
	/// FIND FRAGMENT
	///***************************************************************
	//	I am doing it in a different FSM to keep the overall program simpler
	//	>I want to malloc {size} byte, i search throught the atable
	//	until i find a fragment big enough or the last block.
	//	>I calculate the absolute index and the atable index while i'm at it
	//	>If i'm allocating the first block i skip the while

	//If: i'm allocating the first block
	if (num_block == 0)
	{
		//Rise the continue flag if there is not at least one block already allocated (i will skip the fragment search)
		CLEAR_BIT( status, AT_MALLOC_FLAG_CONTINUE );
		//I will instance a new block
		CLEAR_BIT( status, AT_MALLOC_FLAG_REUSE );
	}
	//If:
	else
	{
		//Rise the continue flag if there is at least one block already allocated
		SET_BIT( status, AT_MALLOC_FLAG_CONTINUE );
		//I start with fragment
		CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
	}

	//Set the absolute memory index to the last byte
	mem_index = 0;
	//Start from the first block
	t = 0;
	//I will instance a new block by default
	CLEAR_BIT( status, AT_MALLOC_FLAG_REUSE );
	//while: i have to continue searching for fragment
	while ( IS_BIT_ONE( status, AT_MALLOC_FLAG_CONTINUE ) )
	{
		///FETCH DATA FROM ATABLE
		//Save rbit index: if i found the right fragment i have to return it's bit position
		wbit_index = rbit_index;
		//Fetch
		rbit_index += HUF_DEC16( &tmp16, rbit_index );
		//If: i fetched a fragment
		if ( IS_BIT_ZERO( status, AT_MALLOC_FLAG_BNOTF ) )
		{
			//if: Is the fragment Big enough?
			if (tmp16 >= size)
			{
				#ifdef AT_MALLOC_DEBUG
					printf("FRAGMENT REUSE\n");
				#endif
				//I will reuse the fragment
				CLEAR_BIT( status, AT_MALLOC_FLAG_CONTINUE );
				//I will reuse a fragment
				SET_BIT( status, AT_MALLOC_FLAG_REUSE );
				//If the fragment is big enough i will recover it's bit position
				rbit_index = wbit_index;
			}
			else
			{
				//Update memory index
				mem_index += tmp16;
			}
		}
		//If: i fetched a block
		else
		{
			//Update memory index
			mem_index += tmp16;
		}

		///NEXT OPERATION CALCULATION
		//If: i fetched fragment
		if ( IS_BIT_ZERO( status, AT_MALLOC_FLAG_BNOTF ) || IS_BIT_ZERO( status, AT_MALLOC_FLAG_CONTINUE ) )
		{
			//Fetch Block next
			SET_BIT( status, AT_MALLOC_FLAG_BNOTF );
		}
		//If: i fetched block
		else
		{
			//Jump to the next block
			t++;

			if (t >= num_block)
			{
				CLEAR_BIT( status, AT_MALLOC_FLAG_CONTINUE );
			}
			else
			{
				//Fetch fragment next
				CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
			}
		}
	}	//end while:
	//Store the insertion block index
	block_index = t;

	//printf("INSERT POINT\n");
	//printf("bit_index: %d\n", rbit_index);
	//printf("memory_index: %d\n", mem_index);
	//printf("status: %d\n", status);
	//printf("block index: %d\n", block_index);
	//printf("num_block: %d\n", num_block);

	///***************************************************************
	///	FREE MEMORY CHECK
	///***************************************************************
	//	Is there enough memory to complete the operation?
	//		FRAGMENT REUSE
	//	The check was already done in the fragment finder
	//		NEW BLOCK
	//	I have to check if num_block require an additional grup
	//	I have to check if the fragment and size bit dimension together with
	//	the rest of the ATABLE is enough to contain everything

	//if: do the num_block require an additional bit group?
	if ( HUF_SIZE16( num_block +1 ) > HUF_SIZE16( num_block ) )
	{
		//I save the information in the status
		SET_BIT( status, AT_MALLOC_FLAG_RESIZE );
	}

	//if: i'm creating a new block i will check the free contiguos memory
	if ( IS_BIT_ZERO( status, AT_MALLOC_FLAG_REUSE ) )
	{
		//
		tmp16 = 0;
		//Account for the increase in the num_block
		if (status >= 8)
		{
			tmp16 += AT_MALLOC_HDEPTH;
			//printf("num_block need to be resized\n");
		}
		//Account for the ATABLE size
		tmp16 = tmp16 + rbit_index;
		//Account for the new fragment size HUF_SIZE16( 0 ) = 1 group
		tmp16 = tmp16 + AT_MALLOC_HDEPTH;
		//Account for the block size
		tmp16 = tmp16 + HUF_SIZE16( size );
		//Convert from bit to byte and round up
		tmp16 = CEILING_DIVIDE( tmp16, 3 );
		//Add the used memory (is the mem_index)
		tmp16 = tmp16 + mem_index;
		//Calculate the free memory
		tmp16 = AT_MALLOC_MEMORY_SIZE -tmp16;

		//If the contiguos memory is not enough
		if (size > tmp16)
		{
			AT_MALLOC_ERROR_HANDLER( AT_MALLOC_MEMORY_FULL );

			return NULL;
		}
	}	//end if: i'm creating a new block i will check the free contiguos memory

	///***************************************************************
	///	NUM BLOCK RESIZE HANDLER
	///***************************************************************
	//	If the num block atable variabile need to be resized, i will
	//	do it and caclulate the read and write index accordingly
	//	Here i have:
	//	>status
	//		>AT_MALLOC_FLAG_RESIZE: if rised, mean that the num_block inside the a-table need to be resized
	//	>block_index: hold the index of the block to be created / modified

	//If num_block need to be resized
	if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_RESIZE ) )
	{
		//set the bit_index to the start
		rbit_index = 0;
		wbit_index = 0;
		//Read the num_block from the atable
		rbit_index += HUF_DEC16( &num_block, rbit_index );
		//Set the read bit index to the start of the block description
		rbit_index += AT_MALLOC_ATABLE_SKIP *AT_MALLOC_HDEPTH;
		//Increase the block count
		num_block++;
		//Write the num_group
		wbit_index += HUF_ENC16( num_block, wbit_index );
		//Set the write bit index to the start of the block description
		wbit_index += AT_MALLOC_ATABLE_SKIP *AT_MALLOC_HDEPTH;
		//Start from the first block descriptor
		t = 0;
		//Reset the memory index
		mem_index = 0;
		#ifdef AT_MALLOC_DEBUG
		printf("NUMBLOCK (resized): %d, windex: %d, rindex: %d\n", num_block, wbit_index, rbit_index);
		#endif
	}
	else
	{
		//Increase the block count
		num_block++;
		//Write the new num_block
		HUF_ENC16( num_block, 0 );
		//I can start from the insertion point
		//rbit_index = rbit_index;
		wbit_index = rbit_index;
		//Start from the insertion block descriptor
		t = block_index;
		#ifdef AT_MALLOC_DEBUG
		printf("NUMBLOCK: %d windex: %d, rindex: %d\n", num_block, wbit_index, rbit_index);
		#endif
	}

	///***************************************************************
	///	ATABLE UPDATER FSM
	///***************************************************************
	//	This FSM handle the update of all block and fragment in the ATABLE
	//	Here i have:
	//	>status
	//		>AT_MALLOC_FLAG_RESIZE:	if rised, mean that the num_block inside the a-table was resized (rbit_index and wbit_index now point to the start of tthe atable)
	//		>AT_MALLOC_FLAG_REUSE: 	if rised mean that there is a fragment big enough to be resized
	//	>block_index:	hold the index of the block to be created/modified
	//	>rbit_index:	read index of the atable
	//	>wbit_index:	write index of the atable

	///WHILE VARIABILE PRESET
	//Reset the memory index (CAUSE OF THE BUG1)
	//mem_index = 0;
	//The read will start from fragment
	CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
	//The advancement is unauthorized
	CLEAR_BIT( status, AT_MALLOC_FLAG_JUMP );
	//rise the continue flag
	SET_BIT( status, AT_MALLOC_FLAG_CONTINUE );
	//Authorize the update of mem_index
	SET_BIT( status, AT_MALLOC_FLAG_MUPDATE );
	///WHILE MALLOC FSM
	//ATABLE update while, i will move all the necessary block
	//while: i have to continue
	while ( IS_BIT_ONE( status, AT_MALLOC_FLAG_CONTINUE ) )
	{
		///READ HANDLER
		//If: the buffer is full
		if ( BUF_ELEM( buf_bot, buf_top, AT_MALLOC_BUFSIZE ) >= (AT_MALLOC_BUFSIZE -1) )
		{
			#ifdef AT_MALLOC_DEBUG
			printf("HOLY CRAP! WAIT!!\n");
			#endif
			//do nothing: if i can't write back anymore, the write handler section will give up
		}	//End If: the buffer is full
		//If: i'm done reading
		else if (t == num_block)
		{
			#ifdef AT_MALLOC_DEBUG
			printf("WAIT\n");
			#endif
			//do nothing: wait for the write back handlet to complete it's job
		}	//end if:
		//If: i'm writing a new block and i am pointing to that block?
		else if ( IS_BIT_ZERO( status, AT_MALLOC_FLAG_REUSE ) && (t == block_index) )
		{
			//If: i have to read a block: save the new fragment in the buffer
			if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_BNOTF ) )
			{
				//size of the block
				tmp16 = size;
				#ifdef AT_MALLOC_DEBUG
				printf("NEWBLOCK block_index: %d size: %d\n", t, tmp16);
				#endif
				//Load the block size in the buffer
				BUF_WRITE( buf, buf_bot, buf_top, AT_MALLOC_BUFSIZE, tmp16 );
				//Is the update of mem_index authorized?
				if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_MUPDATE ) )
				{
					//Update the memory index
					mem_index += tmp16;
				}
				//Authorize the advancement
				SET_BIT( status, AT_MALLOC_FLAG_JUMP );
				//de authorize the update of mem_index
				CLEAR_BIT( status, AT_MALLOC_FLAG_MUPDATE );
			}
			//If: i have to read a fragment: save the new fragment in the buffer
			else
			{
				//Size of the fragment
				tmp16 = 0;
				#ifdef AT_MALLOC_DEBUG
				printf("NEWFRAGMENT block_index: %d size: %d\n", t, tmp16);
				#endif
				//Load the fragment size in the buffer
				BUF_WRITE( buf, buf_bot, buf_top, AT_MALLOC_BUFSIZE, tmp16 );
				//Is the update of mem_index authorized?
				if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_MUPDATE ) )
				{
					//Update the memory index
					mem_index += tmp16;
				}
				//Authorize the advancement
				SET_BIT( status, AT_MALLOC_FLAG_JUMP );
			}
		}	//End If: i'm writing a new block at the end of the atable using the free contiguos memory
		//If: i'm reusing a fragment and i reached the insertion point
		else if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_REUSE ) && (t == block_index) )
		{
			//the rbit index is pointing to the fragment to be reused:
			//If: i have to read a block: save the new fragment in the buffer
			if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_BNOTF ) )
			{
				//The new block size is the block size
				tmp16 = size;
				#ifdef AT_MALLOC_DEBUG
				printf("INNERBLOCK block: %d size: %d\n", t, tmp16);
				#endif
				//Load the block size in the buffer
				BUF_WRITE( buf, buf_bot, buf_top, AT_MALLOC_BUFSIZE, tmp16 );
				//Is the update of mem_index authorized?
				if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_MUPDATE ) )
				{
					//Update the memory index
					mem_index += tmp16;
				}
				//Authorize the advancement
				SET_BIT( status, AT_MALLOC_FLAG_JUMP );
				//de authorize the update of mem_index
				CLEAR_BIT( status, AT_MALLOC_FLAG_MUPDATE );
			}
			//If: i have to read a fragment: save the new fragment in the buffer
			else
			{
				//II read the size without updating the rbit_index
				HUF_DEC16( &tmp16, rbit_index );
				#ifdef AT_MALLOC_DEBUG
				printf("OLD FRAGMENT: %d, rbit_index: %d\n", tmp16, rbit_index);
				#endif
				//The new fragment size is the old fragment size minus the block size
				tmp16 = tmp16 - size;
				#ifdef AT_MALLOC_DEBUG
				printf("INNERFRAGMENT block: %d size: %d\n", t, tmp16);
				#endif
				//Load the fragment size in the buffer
				BUF_WRITE( buf, buf_bot, buf_top, AT_MALLOC_BUFSIZE, tmp16 );
				//Is the update of mem_index authorized?
				if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_MUPDATE ) )
				{
					//Update the memory index
					mem_index += tmp16;
				}
				//Authorize the advancement
				SET_BIT( status, AT_MALLOC_FLAG_JUMP );
			}
		} //End If: i'm reusing a fragment and i reached the insertion point
		//If: I am writing the first fragment after i inserted the new block inside the memory
		else if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_REUSE ) && IS_BIT_ZERO( status, AT_MALLOC_FLAG_BNOTF ) && (t == (block_index +1)) )
		{
			//Update the rbit_index (i don't' need to read)
			//HUF_SIZE16 BUG3
			//rbit_index += HUF_SIZE16( rbit_index );
			rbit_index += HUF_DEC16( &tmp16, rbit_index );
			#ifdef AT_MALLOC_DEBUG
			printf("GHOST FRAGMENT: block_index: %d rbit_index: %d\n", t, rbit_index);
			#endif
			//The fragment was used up, i have to replace it with 0
			BUF_WRITE( buf, buf_bot, buf_top, AT_MALLOC_BUFSIZE, 0 );
			//surely i don't have to update mem_index (is 0 and i passed the allocated block
			//mem_index += 0;
			//Authorize the advancement
			SET_BIT( status, AT_MALLOC_FLAG_JUMP );
		} //End If: I am writing the first fragment after i inserted the new block inside the memory
		//If: none of the above, i just fetch the number
		else
		{
			//Fetch the number from the atable
			rbit_index += HUF_DEC16( &tmp16, rbit_index );
			#ifdef AT_MALLOC_DEBUG
			printf("READ block_index: %d size: %d pos: %d\n", t, tmp16, rbit_index);
			#endif
			//Load the readed data in the buffer
			BUF_WRITE( buf, buf_bot, buf_top, AT_MALLOC_BUFSIZE, tmp16 );
			//Is the update of mem_index authorized?
			if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_MUPDATE ) )
			{
				//Update the memory index
				mem_index += tmp16;
			}
			//Authorize the advancement
			SET_BIT( status, AT_MALLOC_FLAG_JUMP );
		}

		//I have to do a read operation until the read index is lower than
		//the write index + the size of the number to be written

		///WRITE BACK HANDLER
		//If: there is at least a byte to be written
		if ( BUF_ELEM( buf_bot, buf_top, AT_MALLOC_BUFSIZE ) > 0 )
		{
			//peek the number to be written
			tmp16 = BUF_PEEK( buf, buf_bot );
			//If: there is room for the byte to be written OR i readed everything
			if ( ( wbit_index + HUF_SIZE16( tmp16 ) <= rbit_index ) || ( t == num_block ) )
			{
				//Remove the num from buffer
				tmp16 = BUF_READ( buf, buf_bot, buf_top, AT_MALLOC_BUFSIZE );
				#ifdef AT_MALLOC_DEBUG
				printf("WRITEOP wbit_index: %d, rbit_index %d num %d\n", wbit_index, rbit_index, tmp16 );
				#endif
				//Write the num in the ATABLE
				wbit_index += HUF_ENC16( tmp16, wbit_index );
			}	//End If: there is room for the byte to be written  OR i readed everything
			//If: there isn't room to write, but the buffer is full (mean i can't read again)
			//happen when i'm reading a very big number followed by many short number
			else if ( BUF_ELEM( buf_bot, buf_top, AT_MALLOC_BUFSIZE ) >= (AT_MALLOC_BUFSIZE -1) )
			{
				//SHIT!!! I'M DONE FOR!!!
				AT_MALLOC_ERROR_HANDLER( AT_MALLOC_BUFFER_ERROR );

				return NULL;
			}	//End If: there isn't room, but the buffer is full (mean i can't drain it)
		}	//End if: there is at least a byte to be written

		///UPDATE TEMP BLOCK INDEX AND STATUS
		//	>If i readed, advance status flag and block index
		//	>If the buffer is empty and i readed everything, i'm done

		//If: the advancement to the next group was authorized
		if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_JUMP ) )
		{
			//clear the advancement flag
			CLEAR_BIT( status, AT_MALLOC_FLAG_JUMP );

			//If: readed a block AND i'm not done reading
			if ( ( IS_BIT_ONE( status, AT_MALLOC_FLAG_BNOTF ) ) && ( t < num_block ) )
			{
				//Advance block
				t++;
				//Read fragment next
				CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
				#ifdef AT_MALLOC_DEBUG
				printf("JUMP: next block, fragment information: %d\n", t);
				#endif
			}
			//If: i readed a fragment
			else
			{
				//Read block next
				SET_BIT( status, AT_MALLOC_FLAG_BNOTF );
				#ifdef AT_MALLOC_DEBUG
				printf("JUMP: block information: %d\n", t);
				#endif
			}
		}	//End If: the advancement to the next group was authorized

		///END WHILE HANDLING
		//Decide when the operation is completed

		//if i reached the end of the reading, and i'm done writing
		if ( (t == num_block) && ( BUF_ELEM( buf_bot, buf_top, AT_MALLOC_BUFSIZE ) == 0 ) )
		{
			//Clear the continue flag
			CLEAR_BIT( status, AT_MALLOC_FLAG_CONTINUE );
		}
	}	//end while: i have to continue

	///***************************************************************
	/// CALCULATE RETURN ADDRESS
	///***************************************************************
	//	Here i have:
	//	>mem_index:	hold the position of the first byte of the block to be allocated
	#ifdef AT_MALLOC_DEBUG
	printf("mem index: %d\n", mem_index);
	#endif

	//Calculate the index referenced to the memory vector
	mem_index = (AT_MALLOC_MEMORY_SIZE -1) - (mem_index -1);

	ptr = (void *)&at_malloc_memory[ mem_index ];

	#ifdef AT_MALLOC_DEBUG
		printf("return address: %x\n", (int)ptr);
	#endif

	//return the pointer to the memory
	return ptr;
} //end function: at_malloc

/****************************************************************************
**	AT_CALLOC
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**	Simple malloc wrapper that initialise the newly allocated memory
****************************************************************************/

void *at_calloc( uint16_t size )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	//return address
	void *ptr;
	uint8_t *ptr8;
	//counter
	uint16_t t;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	///***************************************************************
	/// ALGORITHM
	///***************************************************************

	ptr = at_malloc( size );

	if (ptr == NULL)
	{
		return NULL;
	}

	ptr8 = (uint8_t *)ptr;

	for (t = 0;t < size;t++)
	{
		ptr8[ t ] = 0;
	}

	///***************************************************************
	/// RETURN
	///***************************************************************

	return ptr;
} //end function: at_calloc

/****************************************************************************
**	AT_REALLOC
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**	Resize a piece of memory perviusly allocated with malloc, copying the content
**	The implementation is dumb, i use the malloc and free function
**	Next implementation will take advantage of resizing an existing block
**	if possibile (always for shrinking)
**
****************************************************************************/

void *at_realloc( void *address, uint16_t old_size, uint16_t new_size )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint8_t *ptr;
	uint8_t *ptr1;

	uint16_t t;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if ( address == NULL )
	{
		return NULL;
	}

	if ( (old_size == 0) || (new_size == 0) )
	{
		return NULL;
	}

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//	>allocate new block
	//	>move data
	//	>free old block
	//	>return new block

	//Allocate a new block of size (new_size)
	ptr = (uint8_t *)at_malloc( new_size * sizeof( uint8_t ) );
	//Malloc fail detection
	if (ptr == NULL)
	{
		return NULL;
	}
	//initialise counter
	t = 0;
	ptr1 = (uint8_t *)address;
	//while: no block is finished
	while ( (t < new_size) && (t < old_size) )
	{
		//Move data
		ptr[ t ] = ptr1[ t ];
		//Next byte
		t++;
	}	//end while: no block is finished
	//Fill the rest with zeros
	//while: new block is not finished
	while (t < new_size)
	{
		//Move data
		ptr[ t ] = 0;
		//Next byte
		t++;
	}	//end while: no block is finished

	//free the old vector
	at_free( address );

	///***************************************************************
	/// RETURN
	///***************************************************************

	return (void *)ptr;
} //end function:


/****************************************************************************
**	AT_SIZEALLOC
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**	Return the size of the allocated memory block, 0 if the block does not exist
****************************************************************************/
/*
uint16_t at_sizalloc( void *address )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint16_t index;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if (address == NULL)
	{
		return 0;
	}

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//	Find the block inside the allocation table and return the block size
	//	>Calculate the absolute displacement of the block
	//	>Scroll the atable until said address is found
	//	>Return the block size

	index =(uint8_t *)&at_malloc_memory[ 0 ] - (uint8_t *)&address[ 0 ];

	printf("%d\n", index);

	///***************************************************************
	/// RETURN
	///***************************************************************

	return index;
} //end function:
*/


/****************************************************************************
**	AT_FREE
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**	This function can free a block allocated with the malloc
**	>
****************************************************************************/

void at_free( void *address )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	//number of block in the memory
	uint16_t num_block;
	//bit read index
	uint16_t rbit_index;
	//bit write index
	uint16_t wbit_index;
	//absolute address of the input memory
	uint16_t address_index;
	//index of the block to be freed
	uint16_t block_index;
	//status byte (FSM use) (does not need to be static)
	uint8_t status			= 0;

	///***************************************************************
	/// TEMP VARIABILE
	///***************************************************************
	//Those variabile will only used for temp porpouse

	uint16_t tmp16, tmp16_1;
	uint16_t t;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	//Block only in case of CRITICAL status error (and recover them
	if (at_malloc_status != AT_MALLOC_ERROR)
	{
		at_malloc_status = AT_MALLOC_OK;
	}

	//Is the memory OK?
	if (at_malloc_status != AT_MALLOC_OK)
	{
		//i will not change the stutus, but i may need to call the handler anyway
		AT_MALLOC_ERROR_HANDLER( at_malloc_status );

		return;
	}

	//Is the pointer valid?
	if (address == NULL)
	{
		AT_MALLOC_ERROR_HANDLER( AT_MALLOC_PARAM_ERROR );

		return;
	}

	//Is the pointer within the memory?
	if ( ((uint8_t *)address < &at_malloc_memory[ 0 ]) || ((uint8_t *)address >= &at_malloc_memory[ AT_MALLOC_MEMORY_SIZE ]) )
	{
		AT_MALLOC_ERROR_HANDLER( AT_MALLOC_PARAM_ERROR );

		return;
	}

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//	>read NUM_BLOCK
	//	>calculate the absolute index of the block
	//	>search in the atable for the block, it exist?
	//	Y:
	//		>num_block need to be resized?
	//		Y:
	//			>resize and move everything from the first block
	//		>if numblock == 1 - free_all()
	//		>is there a block after the one i want to free?
	//		Y:
	//			>sum the block size and the next fragment to the block fragment
	//
	//----------------------------------------------------------------
	//	ES. 1 block: free all
	//		BEFORE: (delete B0)
	//	NB = 1|F0 = 2|B0= 4|
	//		AFTER:
	//	NB = 0
	//----------------------------------------------------------------
	//	ES. 2 block: delete the first and merge fragment and move everything
	//		BEFORE: (delete B0)
	//	NB = 3|F0 = 2|B0 = 4|F1 = 8|B1 = 16|F2 = 32|B2 = 64|
	//		AFTER:
	//	NB = 2|F0 = 2+4+8 = 14|B0 = 16|F1 = 32|B1 = 64|
	//----------------------------------------------------------------
	//	ES. 3 block: delete the last block, i don't have to update any fragment, it will add to the contiguos memory
	//		BEFORE: (delete B2)
	//	NB = 3|F0 = 2|B0 = 4|F1 = 8|B1 = 16|F2 = 32|B2 = 64|
	//		AFTER:
	//	NB = 3|F0 = 2|B0 = 4|F1 = 8|B1 = 16|
	//----------------------------------------------------------------

	#ifdef AT_MALLOC_DEBUG
		printf( "-----------------------------AT_FREE-------------------------\n" );
	#endif

	///***************************************************************
	/// READ NUM_BLOCK
	///***************************************************************
	//	Read num_block from the atable
	//	If numblock-1 would need to be resized, rise the resize flag
	//	At the end i will have:
	//	>num_block: hold the number of allocated block
	//	>rbit_index: point to the fragment information of the first block

	rbit_index = 0;
	wbit_index = 0;

	//read from the ATABLE
	rbit_index = HUF_DEC16( &tmp16, rbit_index );
	//save
	num_block = tmp16;
	//If: i don't have any block, i can quit
	if (num_block == 0)
	{
		AT_MALLOC_ERROR_HANDLER( AT_MALLOC_PARAM_ERROR );

		return;
	}
	//If: num block would need to be resized
	if (rbit_index > HUF_SIZE16( num_block -1 ))
	{
		//rise the resize flag
		SET_BIT( status, AT_MALLOC_FLAG_RESIZE );
	}
	//Write back the new block information
	wbit_index += HUF_ENC16( num_block -1, wbit_index );
	//update num block
	num_block = num_block -1;
	//jump the skip block
	rbit_index += AT_MALLOC_ATABLE_SKIP * AT_MALLOC_HDEPTH;
	wbit_index += AT_MALLOC_ATABLE_SKIP * AT_MALLOC_HDEPTH;

	#ifdef AT_MALLOC_DEBUG
		printf("num_block: %d\n", num_block);
	#endif

	///***************************************************************
	/// CALCULATE ABSOLUTE ADDRESS
	///***************************************************************
	//	calculate the index of the input address referenced to the memory
	//----------------------------------------------------------------
	//	At the end i will have:
	//	>address_index: hold the byte index of the input address referenced to the malloc memory

	//calculate the distance in byte between the start of the memory and the start of the block
	address_index = (uint8_t *)&(at_malloc_memory[ AT_MALLOC_MEMORY_SIZE -1 ]) - (uint8_t *)address +1;

	#ifdef AT_MALLOC_DEBUG
		printf("address_index: %d\n", address_index);
	#endif

	///***************************************************************
	/// SEARCH FOR THE BLOCK TO BE FREED IN THE ATABLE
	///***************************************************************
	//	calculate the address absolute index
	//	>WHILE i haven't reached the end of the atable
	//		>does a block have the wished absolute address?
	//		Y: save the block index and point the rbit_index to the fragment
	//		N: ERROR
	//	At the end i will have:
	//	>block_index: contain the index of the block to be freed
	//	>rbit_index: point to the fragment information of block index

	//clear to avoid annoyng warning
	block_index = 0;
	//rise the continue flag
	SET_BIT( status, AT_MALLOC_FLAG_CONTINUE );
	//Start from the first block
	t = 0;
	//Start from absolute address 0
	tmp16_1 = 0;
	//Read fragment information
	CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
	//While: the continue flag is rised
	while ( IS_BIT_ONE( status, AT_MALLOC_FLAG_CONTINUE ) )
	{
		///READ HANDLER
		//read from the a-table
		rbit_index += HUF_DEC16( &tmp16, rbit_index );

		//if: I'm reading a block?
		if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_BNOTF ) )
		{
			//Subtract the size with the calculated block address
			tmp16_1 += tmp16;
			//I found the block?
			if (address_index == tmp16_1)
			{
				//i'm done searching
				CLEAR_BIT( status, AT_MALLOC_FLAG_CONTINUE );
				//save the block i found
				block_index = t;
				#ifdef AT_MALLOC_DEBUG
					printf("block found!\n");
				#endif
			}
		}
		//if: I'm reading a fragment?
		else
		{
			//Subtract the size with the calculated block address
			tmp16_1 += tmp16;
		}

		///WRITE HANDLER
		//Even if i resized i don't take this chanche to write back, if
		//i have to modify a fragment to update the atable, is difficult to handle

		///JUMP HANDLER
		//I reached the last block?
		if ( (t == num_block +1) || (IS_BIT_ZERO( status, AT_MALLOC_FLAG_CONTINUE ) ) )
		{
			//do nothing
			#ifdef AT_MALLOC_DEBUG
				printf("JUMP: WAIT\n");
			#endif
		}
		//If: i readed a block information
		else if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_BNOTF ) )
		{
			//If: i don't have to resize and this block didn't hold the information i needed
			if ( IS_BIT_ZERO( status, AT_MALLOC_FLAG_RESIZE ) )
			{
				//use wbit_index to save the fragment information position
				wbit_index = rbit_index;
				//printf("save write point: %d\n", wbit_index);
			}
			//read fragment information next
			CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
			//Increase group index
			t++;
			#ifdef AT_MALLOC_DEBUG
				printf("JUMP: NEXT BLOCK, FRAGMENT INFORMATION\n");
			#endif
		}
		//If: i readed a fragment information
		else
		{
			//read block information next
			SET_BIT( status, AT_MALLOC_FLAG_BNOTF );
			#ifdef AT_MALLOC_DEBUG
				printf("JUMP: BLOCK INFORMATION\n");
			#endif
		}

		///END WHILE HANDLER
		//If: i searched the last valid block
		if (t == num_block +1)
		{
			//break the cycle
			CLEAR_BIT( status, AT_MALLOC_FLAG_CONTINUE );
			#ifdef AT_MALLOC_DEBUG
				printf("JUMP: QUIT\n");
			#endif
		}
	}	//End While: the continue flag is rised

	///***************************************************************
	///	ATABLE UPDATER
	///***************************************************************
	//	If:
	//	1> i have only 1 block
	//		at_free_all
	//	2> i have to free the last block
	//		if i have to resize, write back anything but the last block
	//		if i don't have to resize, do nothing
	//	3> i have to resize?
	//		write back anything until i reach the block index -1
	//	ALGORITHM:
	//	>num_block == 0?
	//	Y:
	//		free all
	//	>I have resized num_block?
	//	Y:
	//		start from at the beginning of the atable
	//	N:
	//		start from the fragment information of the block to be freed
	//	>While: continue == '1'
	//		>Read information
	//		>information belong to the block to be freed?
	//		N:
	//			write it back and sum the accumulated data
	//		Y:
	//			accumulate it
	//----------------------------------------------------------------
	//	Here i have:
	//	t				: if == num_block +1 the block is not allocated
	//	block_index		: index of the block to be freed
	//	rbit_index		: useless
	//	wbit_index		: if FLAG_RESIZE is one, point to the first number in the atable
	//					  if FLAG_RESIZE is zero, point to the fragment of the block_index
	//	address_index	: absolute address of the first byte of the block to be freed
	//	FLAG_RESIZE		: if rised, the num_block information was resized

	#ifdef AT_MALLOC_DEBUG
		printf("t: %d\n", t);
		printf("block_index: %d\n", block_index);
		printf("wbit_index: %d\n", wbit_index);
		printf("address_index: %d\n", address_index);
		printf("status: %d\n", status);
	#endif

	//If: i did not find the block
	if (t >= num_block +1 )
	{
		//printf("param_error\n");
		//i tried to free a non valid block: n00b!
		AT_MALLOC_ERROR_HANDLER( AT_MALLOC_PARAM_ERROR );

		return;
	}

	//If: i have only 1 block
	if (num_block == 0)
	{
		//printf("free all the memory\n");
		//nothing to do
		return;
	}

	//If: I have resized num block?
	if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_RESIZE) )
	{
		#ifdef AT_MALLOC_DEBUG
		printf("num_block was resized\n");
		#endif
		//I will start from the beginning (1 more block after the resized num_block)
		rbit_index = wbit_index + 1 * AT_MALLOC_HDEPTH;
		//Start from the first block
		t = 0;
		//Read fragment information
		CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
		//rise the continue flag
		SET_BIT( status, AT_MALLOC_FLAG_CONTINUE );
	}
	//If: I have not resized num block?
	else
	{
		//set the read index as the saved fragment information
		rbit_index = wbit_index;
		//Start from the block i want to free
		t = block_index;
		//Read fragment information
		CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
		//rise the continue flag
		SET_BIT( status, AT_MALLOC_FLAG_CONTINUE );
	}

	#ifdef AT_MALLOC_DEBUG
	printf("WRITE BACK HANDLER\n");
	printf("rbit_index: %d\n", rbit_index);
	printf("wbit_index: %d\n", wbit_index);
	printf("t: %d\n", t);
	#endif

	tmp16_1 = 0;

	//While: the continue flag is rised
	while ( IS_BIT_ONE( status, AT_MALLOC_FLAG_CONTINUE ) )
	{
		///READ HANDLER
		//read from the a-table
		rbit_index += HUF_DEC16( &tmp16, rbit_index );
		#ifdef AT_MALLOC_DEBUG
		printf("READ: data: %d, bit: %d\n", tmp16, rbit_index);
		#endif
		//If: i am not reading the block to be freed
		if ( t == block_index )
		{
			//save the displacement
			tmp16_1 += tmp16;
			#ifdef AT_MALLOC_DEBUG
			printf("ACCUMULATOR: new value: %d\n", tmp16_1);
			#endif
		}

		///WRITE HANDLER
		//If: i am not reading the block to be freed
		if (t != block_index)
		{
			//sum the accumulated data to the number to be written back
			tmp16 += tmp16_1;
			//reset the accumulator
			tmp16_1 = 0;

			wbit_index += HUF_ENC16( tmp16, wbit_index );
			#ifdef AT_MALLOC_DEBUG
			printf("WRITE: data: %d bit :%d\n", tmp16, wbit_index);
			#endif
		}

		///JUMP HANDLER
		//I reached the last block?
		if ( (t == num_block +1) || (IS_BIT_ZERO( status, AT_MALLOC_FLAG_CONTINUE ) ) )
		{
			//do nothing
			#ifdef AT_MALLOC_DEBUG
				printf("JUMP: WAIT\n");
			#endif
		}
		//If: i readed a block information
		else if ( IS_BIT_ONE( status, AT_MALLOC_FLAG_BNOTF ) )
		{
			//read fragment information next
			CLEAR_BIT( status, AT_MALLOC_FLAG_BNOTF );
			//Increase group index
			t++;
			#ifdef AT_MALLOC_DEBUG
				printf("JUMP: NEXT BLOCK, FRAGMENT INFORMATION\n");
			#endif
		}
		//If: i readed a fragment information
		else
		{
			//read block information next
			SET_BIT( status, AT_MALLOC_FLAG_BNOTF );
			#ifdef AT_MALLOC_DEBUG
				printf("JUMP: BLOCK INFORMATION\n");
			#endif
		}

		///END WHILE HANDLER
		//If: i searched the last valid block
		if (t == num_block +1)
		{
			//break the cycle
			CLEAR_BIT( status, AT_MALLOC_FLAG_CONTINUE );
			#ifdef AT_MALLOC_DEBUG
				printf("JUMP: QUIT\n");
			#endif
		}
	}	//End While: the continue flag is rised

	///***************************************************************
	/// RETURN
	///***************************************************************

	return;
} //end function:

/****************************************************************************
**	AT_MALLOC_STAT
*****************************************************************************
**	PARAMETER:
**	RETURN:
**	DESCRIPTION:
**	If cfg is a data index (see below): a vector of size 1 will be returned with the wished data
**	If cfg is 6+: the full vector will be returned.
**
**	Return a vector with the desired information (data index)
**	0: Total allocated memory
**	1: Total free memory (fragment included)
**	2: Total fragmented memory
**	3: Max fragment size
**	4: Number of allocated block
**	5: ATable size
****************************************************************************/

uint16_t *at_malloc_stat( uint8_t cfg )
{
	///***************************************************************
	/// STATIC VARIABILE
	///***************************************************************

	///***************************************************************
	/// LOCAL VARIABILE
	///***************************************************************

	uint16_t *stat = NULL;

	//bit address inside the atable
	uint16_t bit_index;

	uint16_t t;

	uint16_t tmp16;

	///***************************************************************
	/// PARAMETER CHECK
	///***************************************************************

	if (cfg > AT_MALLOC_NUMSTAT)
	{
		return NULL;
	}

	///***************************************************************
	/// ALGORITHM
	///***************************************************************
	//	>Allocate return vector
	//	>Read the malloc statistic
	//	>return the return vector

	///Allocate the return vector
	stat = (uint16_t *)at_malloc( AT_MALLOC_NUMSTAT * sizeof( uint16_t ) );
	//malloc fail check
	if (stat == NULL)
	{
		return NULL;
	}
	///Initialise stat vector
	for (t = 0;t < AT_MALLOC_NUMSTAT;t++)
	{
		stat[ 0 ] = 0;
	}
	///Read the ATABLE PARAM
	//Set cursor to the start of the atable
	bit_index = 0;
	//Read the number of block
	bit_index += HUF_DEC16( &tmp16, bit_index );
	//num_block = tmp16;
	stat[ AT_MALLOC_NUMBLOCK ] = tmp16;
	//Skip the void Block
	bit_index += AT_MALLOC_ATABLE_SKIP *AT_MALLOC_HDEPTH;
	///Read the block data
	//initialise while
	t = 0;
	//while: Until all block are readed
	while ( t < stat[ AT_MALLOC_NUMBLOCK ] )
	{
		//Decode fragment information
		bit_index += HUF_DEC16( &tmp16, bit_index );
		#ifdef ATABLE_DEBUG
		printf("F%3d- %3d BYTE\n",t , tmp16);
		#endif
		//Is this fragment the biggest fragment?
		if ( tmp16 > stat[ AT_MALLOC_MAXFRAG ] )
		{
			//Update the max fragment size
			stat[ AT_MALLOC_MAXFRAG ] = tmp16;
		}
		//Add the fragment to the total fragmented memory
		stat[ AT_MALLOC_FRAGMEM ] += tmp16;
		//Decode block information
		bit_index += HUF_DEC16( &tmp16, bit_index );
		#ifdef ATABLE_DEBUG
		printf("B%3d- %3d BYTE\n",t , tmp16);
		#endif
		//Add the block to the total used memory
		stat[ AT_MALLOC_USEDMEM ] += tmp16;
		//jump to the next block
		t++;
	}	//end while: Until all block are readed
	//Calculate the atable size
	stat[ AT_MALLOC_ATSIZE ] = bit_index /8 +1;
	//Calculate the total free memory
	stat[ AT_MALLOC_FREEMEM ] = AT_MALLOC_MEMORY_SIZE - stat[ AT_MALLOC_USEDMEM ] - stat[ AT_MALLOC_ATSIZE ];

	#ifdef ATABLE_DEBUG
		printf("0: %5d Total allocated memory\n", stat[ 0 ]);
		printf("1: %5d Total free memory - fragment included\n", stat[ 1 ]);
		printf("2: %5d Total fragmented memory\n", stat[ 2 ]);
		printf("3: %5d Max fragment size\n", stat[ 3 ]);
		printf("4: %5d Number of allocated block\n", stat[ 4 ]);
		printf("5: %5d ATable size\n", stat[ 5 ]);
	#endif

	//If i want to return only 1 value
	if (cfg != AT_MALLOC_NUMSTAT)
	{
		//save the useful stat
		tmp16 = stat[ cfg ];
		//free the stats vector
		at_free( stat );
		stat = NULL;
		//allocate memory for a single byte
		stat = at_malloc( sizeof( uint16_t ) );
		//Save the useful stat
		stat[ 0 ] = tmp16;
	}

	///***************************************************************
	/// RETURN
	///***************************************************************

	return stat;
} //end function: at_malloc_stat
