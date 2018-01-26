#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "fat12.h"

int fid; /* global variable set by the open() function */

void empty_sector(uint8_t *buffer)
{
	int i;
	for(i = 0; i < DEFAULT_SECTOR_SIZE; buffer[i++]=0);
}

/*FD_WRITE function from the exercise description*/
int fd_write(int sector_number, uint8_t *buffer){
	int dest, len;
	dest = lseek(fid, sector_number * DEFAULT_SECTOR_SIZE, SEEK_SET);
	if (dest != sector_number * DEFAULT_SECTOR_SIZE){
		perror("fd_write");
	}
	len = write(fid, buffer, DEFAULT_SECTOR_SIZE);
	if (len != bps){
		perror("fd_write");
	}
	return len;
}

void set_boot_record(void *buffer)
{
	boot_record_t br;
	char oem[8] = "MSWIN4.1"
	
	br.bootjmp[0] = 0xeb;
	br.bootjmp[2] = 0x90;
	strcpy(br.oem_id, oem);
	br.sector_size = DEFAULT_SECTOR_SIZE;
	br.sectors_per_cluster = 1;
	br.reserved_sector_count = 1;
	br.number_of_fats = 2;
	br.number_of_dirents = 224;
	br.sector_count = 2880;
	br.media_type = 0xf0;
	br.fat_size_sectors = 9;
	br.sectors_per_track = 18;
	br.nheads = 2;
	br.sectors_hidden = 0;
	br.sector_count_large = 0;
	
	br.drive_number = 0;
	br.reserved1 = 0;
	br.boot_signature = 0;
	
	memcpy(buffer, &br, 38);
	return;
}

/*Sets the first 12 bits for value in the specified index at the buffer. Uses a similar logic to GetFatEntry in fat12.pdf*/
void set_fat_entry(uint8_t *buffer, int fat_index, uint16_t value)
{
	int word_index;
	
	word_index = (fat_index/2)*3; /*Strating index of a 3-byte segment*/
	if( fat_index % 2 == 0) {	
		/*Write value on 12 higher-order bits:*/
		buffer[word_index] = (value & 0x0ff0) >> 4	/*8 higher-order bits*/
		buffer[word_index+1] =  ((value & 0x000f) << 4) + (buffer[word_index+1] & 0x0f) /*4 lower-order bits*/
	} else {
		/*Write value on 12 lower-order bits*/
		buffer[word_index+1] =  ((value & 0x0f00) >> 8) + (buffer[word_index+1] & 0xf0) /*4 higher-order bits*/
		buffer[word_index+2] = value & 0x00ff	/*8 lower-order bits*/
	}
	return;
}
		
		
}

/*Fills buffer (assumed to be initalized with 0s) with the first two entries of a FAT table*/
/*This is done per the description in https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html*/
void create_fat_table(void *buffer)
{
	/*set FAT entry 0*/
	set_fat_entry(buffer, 0, 0xf0f); /*Media descriptor is 0xf0, remaining bits are set to 1*/
	set_fat_entry(buffer, 1, 0xfff); /*EOF marker*/
}


int main(int argc, char *argv[])
{
	uint8_t *sector;	/*512-byte buffer for writing*/
	int sector_idx;		/*Used for indexing*/

	if (argc != 2)
	{
		printf("Usage: %s <floppy_image>\n", argv[0]);
		exit(1);
	}

	if ( (fid = open (argv[1], O_RDWR|O_CREAT, 0644))  < 0 )
	{
		perror("Error: ");
		return -1;
	}

	
	/*Initialize the sector buffer*/
	sector = malloc(sizeof(char)*BYTES_PER_SECTOR);
	if( sector == NULL ) {
		printf("Insufficient memory");
		return -1;
	}
	
	/*1. Initialize and write boot sector*/
	empty_sector(fat_sector);
	set_boot_record(fat_sector);
	fd_write(0, fat_sector);
	
	/*2. Initialize FAT 1 and FAT2 tables*/
	/*Entires 0 & 1 for FAT1 and FAT2*/
	empty_sector(fat_sector);
	create_fat_table(fat_sector);
	fd_write(1, fat_sector);	/*FAT1*/
	fd_write(10, fat_sector);	/*FAT2*/
	
	/*Remaining FAT entries are empty (0x000)*/
	empty_sector(fat_sector);
	for(sector_idx=2; sector_idx < 10; sector_idx++) {
		fd_write(sector_idx, fat_sector);
		fd_write(sector_idx+9, fat_sector);
	}
	
	/* 3. Set direntires as free. Just fill it with zeros (as per sec.8 of the exercise) */
	for(sector_idx=19; sector_idx<33; fd_write(sector_idx++, fat_sector));
	
	
	// Step 4. Handle data block (e.g you can zero them or just leave 
	// untouched. What are the pros/cons?)
	/* No need to handle the data block. If the dirents and FAT tables are empty, the actual data in the data block is ignored*/
	
	close(fid);
	return 0;
}

