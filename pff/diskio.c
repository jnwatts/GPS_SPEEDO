/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2014      */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "sd_io.h"

SD_DEV sd;

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (void)
{
	if (SD_Init(&sd) != SD_OK)
		return RES_ERROR;
	
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Partial Sector                                                   */
/*-----------------------------------------------------------------------*/

DRESULT disk_readp (
	BYTE* buff,		/* Pointer to the destination object */
	DWORD sector,	/* Sector number (LBA) */
	UINT offset,	/* Offset in the sector */
	UINT count		/* Byte count (bit15:destination) */
)
{
	if (SD_Read(&sd, buff, sector, offset, count) != SD_OK)
		return RES_ERROR;

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Partial Sector                                                  */
/*-----------------------------------------------------------------------*/

DRESULT disk_writep (
	const BYTE* buff,		/* Pointer to the data to be written, NULL:Initiate/Finalize write operation */
	DWORD sc		/* Sector number (LBA) or Number of bytes to send */
)
{

	if (!buff) {
		if (sc) {

			// Initiate write process

		} else {

			// Finalize write process

		}
	} else {

		// Send data to the disk
		if (SD_Write(&sd, (void*)buff, sc) != SD_OK) {
			return RES_ERROR;
		}

	}

	return RES_OK;
}


