/* filesys.c
 * 
 * provides interface to virtual disk
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "filesys.h"


diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
direntry_t * currentDir              = NULL ;
fatentry_t   currentDirIndex         = 0 ;

/* writedisk : writes virtual disk out to physical disk
 * 
 * in: file name of stored virtual disk
 */

void readblock(diskblock_t* block, int block_addres);

void writedisk ( const char * filename )
{
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;
   
}

//this function is used to initialize FAT and rootdir block from disk
void interpret_rootdir_and_fat() {
   //interpret FAT block
   //we know fat accupies blocks 1 and 2
   //normally this information would be provided in block 0
   //in this excercise i will use known fat indexes
   //if fat was to be made bigger, this function must be updated
   diskblock_t block; readblock(&block, 1);
   diskblock_t block2; readblock(&block2, 2);
   int j = 0;
   for (int i = 0; i < (BLOCKSIZE / sizeof(fatentry_t)); i++) {
      FAT[j] = block.fat[i];
      FAT[j + (BLOCKSIZE / sizeof(fatentry_t))] = block2.fat[i];
      j++;
   }
   
   //root dir at block 3
   diskblock_t rootdir; readblock(&rootdir, 3);
   currentDirIndex = 3;
   rootDirIndex = 3;
}

void readdisk ( const char * filename )
{
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
      fclose(dest) ;
   
   interpret_rootdir_and_fat();
}


/* the basic interface to the virtual disk
 * this moves memory around
 */

void writeblock ( diskblock_t * block, int block_address )
{
   //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
   memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
   //printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}

void readblock (diskblock_t* block, int block_address )
{
   memmove ( block->data, virtualDisk[block_address].data, BLOCKSIZE ) ;
}


/* read and write FAT
 * 
 * please note: a FAT entry is a short, this is a 16-bit word, or 2 bytes
 *              our blocksize for the virtual disk is 1024, therefore
 *              we can store 512 FAT entries in one block
 * 
 *              how many disk blocks do we need to store the complete FAT:
 *              - our virtual disk has MAXBLOCKS blocks, which is currently 1024
 *                each block is 1024 bytes long
 *              - our FAT has MAXBLOCKS entries, which is currently 1024
 *                each FAT entry is a fatentry_t, which is currently 2 bytes
 *              - we need (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))) blocks to store the
 *                FAT
 *              - each block can hold (BLOCKSIZE / sizeof(fatentry_t)) fat entries
 */
 
void copyFAT()
{
   //copy variable FAT to disk
   //iterate through FAT and write down every fatentry to fatblock
   //writeblock to disk
   diskblock_t block;
   int index = 0;
   for(int x = 1; x <= (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))); x++) { //two blocks
      for(int y = 0; y < (BLOCKSIZE / sizeof(fatentry_t)); y++){ //fat has 512 shorts
         block.fat[y] = FAT[index++];
      }
      writeblock(&block, x);
   }
}

/* implement format()
 */
void format ( )
{
   diskblock_t block ;
   direntry_t  rootDir ;
   int         pos             = 0 ;
   int         fatentry        = 0 ;
   int         fatblocksneeded =  (MAXBLOCKS / FATENTRYCOUNT ) ;
   
   //create a block and clear all data
   //C does not neccesairly create a variable without any data
   //So it's important to make sure that every byte is 0
   for (int i = 0; i < BLOCKSIZE; i++) block.data[i] = '\0' ;
   
   //write the string in the beggining of the first block
   strcpy( block.data,	"CS3026 Operating Systems Assessment" );	
   writeblock( &block, 0 );
   
   //Set all FAT as unused
   for (int i = 0; i < MAXBLOCKS; i++) {
      FAT[i] = UNUSED;
   }
   
   //initialize 1st block, fatblock and rootdirblock
   FAT[0] = ENDOFCHAIN;
   FAT[1] = 2;
   FAT[2] = ENDOFCHAIN;
   FAT[3] = ENDOFCHAIN;
   
   //save fat to disk
   copyFAT();
   
   //initialize rootdir block
   memset(&block.dir, 0x0, BLOCKSIZE);
   block.dir.isdir = 1;
   block.dir.nextEntry = 0;
   block.dir.parentDirBlock = ROOTBLOCKDIRENTRY;
   block.dir.parentDirEntry = ROOTBLOCKDIRENTRY;
   
   //save rootdir block
   writeblock(&block, 1 + fatblocksneeded);
   rootDirIndex = 1 + fatblocksneeded;
   
   currentDirIndex = rootDirIndex;
   
}


/* use this for testing
 */

void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

//this function returns index of next unused block
//if there is no such block, end program (memory full)
int next_unused_block()
{
   for (int i = 4; i < MAXBLOCKS; i++)
   {
      if (FAT[i] == UNUSED)
         return i;
   }
   printf("Disk is out of space. please change your program. exiting...\n");
   exit(0);
   return -1;
}

//this function returns the index of next unused direntry in dirblock's entrylist
int next_unused_direntry(dirblock_t dir)
{
   for (int i = 0; i < DIRENTRYCOUNT; i++) 
   {
      if (i >= dir.nextEntry) {
         return dir.nextEntry;
      } else if (dir.entrylist[i].unused == 1) {
         return i;
      }
   }
   
   return -1; //no more space
}

//this function returns boolean showing if a dirblock has any used entries
int is_dir_empty(dirblock_t d) 
{
   if (d.nextEntry == 0) return 1;
   for (int i = 0; i < d.nextEntry; i++) {
      if (d.entrylist[i].unused == 0) 
         return 0;
   }
   return 1;
}

//this function jumps through FAT and sets entries along the way to unused
void clear_fat_entries(int index) 
{
   while (FAT[index] != ENDOFCHAIN) {
      int i = FAT[index];
      FAT[index] = UNUSED;
      index = i;
   }
   FAT[index] = UNUSED;
}

//this function finds the file in dirblock
//if dirblock has no file with name filename, then -1 is returned
//if dirblock has file, index in entrylist is returned
int find_file(dirblock_t block, const char* filename) 
{
   for (int i = 0; i < block.nextEntry; i++) {
      if (strcmp(filename, block.entrylist[i].name) == 0 && block.entrylist[i].isdir == 0 && !block.entrylist[i].unused) 
         return i;
   }
   return -1;
}

//this function is the same as find_file, except it finds directories
int find_dir(dirblock_t block, char* name)
{
   for (int i = 0; i < block.nextEntry; i++) {
      if (strcmp(name, block.entrylist[i].name) == 0 && block.entrylist[i].isdir == 1 && !block.entrylist[i].unused) 
         return i;
   }
   return -1;
}

//-1 is error, else is index to block
//this function creates a directory inside another directory
int create_directory(dirblock_t * dir, int parentindex, char* name) {
   
   int block_index = next_unused_block();
   direntry_t direntry;
   direntry.firstblock = block_index;
   direntry.isdir = 1;
   direntry.unused = 0;
   strcpy(direntry.name, name);
   
   int nextentryindex = next_unused_direntry(*dir);
   if (nextentryindex < 0) {
      printf("ERROR: directory full\n");
      return -1;
   }
   dir->entrylist[nextentryindex] = direntry;

   diskblock_t block;
   memset(&block, 0x0, BLOCKSIZE);
   block.dir.parentDirBlock = parentindex;
   block.dir.parentDirEntry = dir->nextEntry;
   for (int i = 0; i < DIRENTRYCOUNT; i++) 
      block.dir.entrylist[i].unused = 1;
   writeblock(&block, block_index);
   FAT[block_index] = ENDOFCHAIN;
   copyFAT();
   
   dir->nextEntry++;
   
   return block_index;
}

//this function visits directories in a path and returns index to block of the last one
//-1 menas that some of the directories along the way does not exist
int parse_path(char* path)
{
   diskblock_t cur_dir;
   int current_index;

   
   if (path[0] == '/' || currentDir == NULL) { //absolute path
      current_index = rootDirIndex;
   } else { //relative
      current_index = currentDir->firstblock;
   }
   
   readblock(&cur_dir, current_index);
   
   //tokenize path by '/'
   char str[strlen(path)*sizeof(char)+1];
   strcpy(str, path);
   char * pch;
   char *saveptr;
   pch = __strtok_r(str,"/", &saveptr);
   while (pch != NULL) //each dir in path
   {
      if (strcmp(".", pch) == 0) { //same dir
      } else if (strcmp("..", pch) == 0) { //parent dir
         current_index = cur_dir.dir.parentDirBlock;
         if (current_index == ROOTBLOCKDIRENTRY) {
            printf("ERROR: already in root directory. Cannot move to parent directory\n");
            return -1;
         }
      } else {
         int direntry_index = find_dir(cur_dir.dir, pch);
         if (direntry_index >= 0) { //directory exists
            //find index of that directory
            direntry_t entry = cur_dir.dir.entrylist[direntry_index];
            int next_block_index = entry.firstblock;
            
            current_index = next_block_index;
         } else { //does not exist
            printf("ERROR: no such file or directory: %s\n", pch);
            return -1;
         }
      }
      
      //cur_dir is the block of found directory
      readblock(&cur_dir, current_index);
      pch = __strtok_r(NULL, "/", &saveptr); //next token
   }
   return current_index;
}

//this function returns how many "/" characters are in path string
int get_number_of_entries(const char* path) {
   int occ = 0;
   for(int i = 0; i < strlen(path); i++) {
      if (path[i] == '/') occ++;
   }
   return occ;
}

//this function returns the last token in path string
char* get_path_last(const char* path) { 
   if (get_number_of_entries(path) == 0) { //return itself
      char* toReturn = malloc(strlen(path)*sizeof(char)+1);
      strcpy(toReturn, path);
      return toReturn;
   }
   char str[strlen(path)*sizeof(char)+1];
   strcpy(str, path);
   char * pch;
   pch = strrchr(str,'/');
   pch += 1;
   char* toReturn = malloc(strlen(pch) * sizeof(char)+1);
   strcpy(toReturn, pch);
   return toReturn;
}

//this function returns path except the last token
char* get_path_except_last(const char* path) {
   if (get_number_of_entries(path) == 0) {
      if (strcmp(path, "..") == 0) {
         return "..";
      } else {
         return ".";
      }
   }
   char str[strlen(path)*sizeof(char)+1];
   strcpy(str, path);
   char * pch;
   pch = strrchr(str,'/');
   int index = pch-str+1;
   char* toReturn = malloc(index+1);
   
   memcpy(toReturn, path, index);
   toReturn[index] = '\0';
   return toReturn;
}

MyFILE * myfopen(const char * filename, const char * mode)
{
   //check modes
   if (!(strcmp(mode, "r") == 0 || strcmp(mode, "w") == 0 || strcmp(mode, "a") == 0)) {
      printf("ERROR: unknown mode");
      return 0;
   }
   
   if (filename[strlen(filename)-1] == '/') {
      printf("filename can't end with '/'");
      return 0;
   }
   
   //split *filename into path and actual filename
   
   int index;
   char* newpath = get_path_except_last(filename);
   char* fname = get_path_last(filename);
   
   index = parse_path(newpath);
   if (index < 0) { //trying to create directory in directory that does not exist
      printf("ERROR: file was not created\n");
      return 0;
   }
   
   diskblock_t dirblock;
   readblock(&dirblock, index);
   
   //check if a directory with same name exists
   if (find_dir(dirblock.dir, fname) > -1) {
      printf("ERROR: There already a directory with such a name\n");
      return 0;
   }
   
  int block_index;
  int filelength;
  
  
  //create pointer to empty file
  MyFILE *file = malloc(sizeof(MyFILE));
  file->pos = 0; //always begin at 0
  memcpy(file->mode, mode, sizeof(mode)); //filemode
  
  //find file's index in direntry list (if -1, create new file)
  int direntry_no = find_file(dirblock.dir, fname); 
  if (direntry_no == -1) { //create new file
      if (strcmp(mode, "r") == 0) // in readmode, if no file is found, return 0
         return 0;
      
      //add a direntry at index nextEntry
      direntry_no = next_unused_direntry(dirblock.dir);
      if (direntry_no < 0) {
         printf("ERROR: directory full\n");
         return 0;
      }
      dirblock.dir.nextEntry++; // new file created, increment nextEntry
      
      block_index = next_unused_block(); // new block
      filelength = 0;
      //create file's direntry
      direntry_t *dir_entry = &(dirblock.dir.entrylist[ direntry_no ]);
      dir_entry->isdir = 0;
      dir_entry->unused = 0;
      dir_entry->filelength = filelength;
      dir_entry->firstblock = block_index;
      strcpy(dir_entry->name, fname);
      
      //save changes in fat and dirblock
      writeblock(&dirblock, index);
      FAT[block_index] = ENDOFCHAIN;
      copyFAT();
  } else { //truncate the file in write mode
      //block index already exists
      direntry_t direntry = dirblock.dir.entrylist[ direntry_no ];
      block_index = direntry.firstblock;
      if (strcmp(mode, "w") == 0) { //truncate file in write mode
         filelength = 0;
         //clear FAT
         clear_fat_entries(block_index);
         FAT[block_index] = ENDOFCHAIN;
         copyFAT();
      } else if (strcmp(mode, "a") == 0) { //move to the end of file in append mod
         filelength = direntry.filelength;
         file->pos = filelength; //last char in given block
         while (FAT[block_index] != ENDOFCHAIN) { //iterate through FAT to find ENDOFCHAIN
            block_index = FAT[block_index]; // find last block
            file->pos -= BLOCKSIZE; // pos in given block
         }
      } else { //read filelength in readmode
         filelength = direntry.filelength;
      }
  }
  
  //depending on mode, get first or last block of file
  diskblock_t curblock;
  readblock(&curblock, block_index); //first block of file
  
  //initialize file values
  file->blockno = block_index;
  file->buffer = curblock;
  file->dirblock_no = index;
  file->direntry_no = direntry_no;
  file->filelength = filelength;
  
  return file;
}

void myfputc(int b, MyFILE *stream)
{
   //check mode
   if (strcmp(stream->mode, "r") == 0) {
      printf("Error: cant write in readmode\n");
      return; //can't edit in read mode;
   }
   
   //if pos is exceeding blocksize, no more bytes can be written in current buffer
   //save buffer to block, initiate new buffer, set pos as 0
   if (stream->pos >= BLOCKSIZE) //move to next block
   {
      //update FAT
      int next_block_index = next_unused_block();
      FAT[stream->blockno] = next_block_index;
      FAT[next_block_index] = ENDOFCHAIN;
      copyFAT();
      stream->pos = 0;
      
      //save before buffer change
      writeblock(&stream->buffer, stream->blockno);
      
      //new buffer
      diskblock_t next_block;
      readblock(&next_block, next_block_index);
      stream->buffer = next_block;
      stream->blockno = next_block_index;
   }
   //write byte to buffer
   stream->buffer.data[stream->pos] = b;
   stream->filelength++;
   
   //increment pos
   stream->pos++;
}

char myfgetc(MyFILE* stream)
{
   //check if mode is correct
   if (strcmp(stream->mode, "r") != 0) {
      printf("Error: can't read not in readmode\n");
      return  '\0'; //only read in read mode
   }
   
   //if pos is at the end of the file, return EOF
   if (FAT[stream->blockno] == ENDOFCHAIN && stream->pos >= stream->filelength % BLOCKSIZE) return '\0';
   
   //if pos is at the end of the block, we must move buffer to next block
   if (stream->pos >= BLOCKSIZE) {
      //get next block index from FAT
      int next_block_index = FAT[stream->blockno];
      
      //if there is no next block, return EOF
      if (next_block_index == ENDOFCHAIN) 
         return '\0'; //end of file
         
      //init next block to buffer
      diskblock_t nextblock;
      readblock(&nextblock, next_block_index);
      stream->buffer = nextblock;
      stream->blockno = next_block_index;
      stream->pos = 0;
   }
   
   char toReturn = stream->buffer.data[stream->pos];
   stream->pos++;
   
   return toReturn;
}

void myfclose(MyFILE *stream)
{
   //in readmode there are no changes to the file. the only operation required is freeing the memory
   if (strcmp(stream->mode, "r") != 0) {// not reading 
      //save buffer to block
      writeblock(&stream->buffer, stream->blockno);
      
      //update filelength
      diskblock_t dirblock;
      readblock(&dirblock, stream->dirblock_no);
      dirblock.dir.entrylist[stream->direntry_no].filelength = stream->filelength;
      writeblock(&dirblock, stream->dirblock_no);
   }
   
   //free the memory
   free(stream);
}


//-1 is error
int mymkdir(char* path) {
   
   if (path[strlen(path)-1] == '/') path[strlen(path)-1] = '\0';
   
   //get actual filename and actual path
   int index;
   char* newpath = get_path_except_last(path);
   char* dirname = get_path_last(path);
   
   index = parse_path(newpath);
   if (index < 0) { //trying to create directory in directory that does not exist
      printf("ERROR: path was not created\n");
      return -1;
   }
   
   
   if (index < 0) { //trying to create directory in directory that does not exist
      printf("ERROR: path was not created\n");
      return -1;
   }
   
   diskblock_t cur_dir;
   readblock(&cur_dir, index);
   
   for (int i = 0; i < cur_dir.dir.nextEntry; i++) {
      if (strcmp(dirname, cur_dir.dir.entrylist[i].name) == 0) {
         printf("ERROR: There already exists such a file or directory\n");
         return -1;
      }
   }
   
   //create directory
   if (create_directory(&cur_dir.dir, index, dirname) < 0) { //not enough space in directory
      printf("ERROR: path was not created\n");
      return -1;
   }
   
   writeblock(&cur_dir, index);
   
   return 0;
}

char** mylistdir(char* path)
{
   if (path[strlen(path)-1] == '/') path[strlen(path)-1] = '\0';
   //in case of error, returns empty array
   diskblock_t cur_dir;
   int cur_dir_index = parse_path(path); //find last dirblock in path
   
   if (cur_dir_index < 0) { //some error
      //return empty array
      char** toReturn = malloc(sizeof *toReturn);
      char* zero = ENDOFPATHARRAY;
      toReturn[0] = malloc(strlen(zero) * sizeof(char)); 
      strcpy(toReturn[0], zero); 
      printf("ERROR: Directories could not be listed\n");
      return toReturn;
   }
   
   readblock(&cur_dir, cur_dir_index);
   
   dirblock_t dir = cur_dir.dir;
   int size = dir.nextEntry;
   char** toReturn = malloc(sizeof *toReturn * size);
   
   int i = 0;
   int j = 0;
   while (i < dir.nextEntry) {
      if (dir.entrylist[i].unused == 0) {
         char* name;
         int size = strlen(dir.entrylist[i].name) * sizeof(char);
         
         toReturn[j] = malloc(size); 
         strcpy(toReturn[j], dir.entrylist[i].name); 
         j++;
      }
      i++;
   }
   char* zero = ENDOFPATHARRAY;
   toReturn[j] = malloc(strlen(zero) * sizeof(char)); 
   strcpy(toReturn[j], zero); 
   
   return toReturn;
}

//this function takes in array of strings (directories) and prints them out in usable way)
void printdirs(char** list)
{
   int i = 0;
   while (strcmp(list[i], ENDOFPATHARRAY) != 0) {
      printf("\t%s", list[i]);
      i++;
   }
   printf("\n");
}

void mychdir(char* path)
{
   if (strcmp(path, ".") == 0) { // we are already here
      return;
   } else if (strcmp(path, "..") == 0) { //move to parent
      diskblock_t cur_dir;
      readblock(&cur_dir, currentDirIndex);
      int index = cur_dir.dir.parentDirBlock;
      if (index == ROOTBLOCKDIRENTRY) {
         printf("ERROR: already in root directory. Cannot move to parent directory\nError: path was not changed\n");
         return;
      }
      readblock(&cur_dir, index); //parent 
      currentDirIndex = index;
      if (cur_dir.dir.parentDirBlock == ROOTBLOCKDIRENTRY) { //if moved to root, currentDir is null
         currentDir = NULL;
         return;
      }
      int e = cur_dir.dir.parentDirEntry;
      readblock(&cur_dir, cur_dir.dir.parentDirBlock);
         
      currentDir = malloc(sizeof(direntry_t));
      memcpy(currentDir, &cur_dir.dir.entrylist[e], sizeof(direntry_t));
      
      return;
   } else if (strcmp(path, "/") == 0) { //move to root
      currentDir = NULL;
      currentDirIndex = rootDirIndex;
      return;
   }
   
   //get actual filename and actual path
   int index;
   char* newpath = get_path_except_last(path);
   char* dirname = get_path_last(path);
   
   index = parse_path(newpath);
   if (index < 0) { //trying to create directory in directory that does not exist
      printf("ERROR: path was not changed\n");
      return;
   }
   
   diskblock_t d;
   readblock(&d, index);
   
   int entrylistindex = find_dir(d.dir, dirname);
   if (entrylistindex < 0) {
      printf("ERROR: no such file or directory: %s\n", dirname);
      return;
   }
   
   //update currentDir and currentDirIndex
   direntry_t current_direntry = d.dir.entrylist[entrylistindex];
   currentDirIndex = current_direntry.firstblock;
   currentDir = malloc(sizeof(direntry_t));
   memcpy(currentDir, &current_direntry, sizeof(direntry_t));
   
}

void myremove(char* path) 
{
   if (path[strlen(path)-1] == '/') {
      printf("filename can't end with '/'\n");
      return;
   }
   
   //split *filename into path and actual filename
   char* fname;
   int index;
   if (get_number_of_entries(path) == 0) {
      if (strcmp(path, "..") == 0 || strcmp(path, ".") == 0) {
         printf("ERROR, wrong filename\n");
         return;
      } else {
         fname = malloc(strlen(path));
         strcpy(fname, path);
         index = currentDirIndex;
      }
   } else {
      char* newpath = get_path_except_last(path);
      fname = get_path_last(path);
      
      index = parse_path(newpath);
      if (index < 0) { //trying to create directory in directory that does not exist
         printf("ERROR: file not found\n");
         return;
      }
   }
   diskblock_t dirblock;
   readblock(&dirblock, index);
   
   int i = find_file(dirblock.dir, fname);
   if (i < 0) {
      printf("ERROR: file not in given directory\n");
      return;
   }
   
   //deleting is setting unused to 1 and clearing FAT entries to UNUSED
   dirblock.dir.entrylist[i].unused = 1;
   clear_fat_entries(dirblock.dir.entrylist[i].firstblock);
   copyFAT();
   
   writeblock(&dirblock, index);
}


void myrmdir(char* path)
{
   if (strcmp(path, "/") == 0) {
      printf("ERROR: can't delete root directory");
      return;
   }
   if (path[strlen(path)-1] == '/' || strcmp(path, ".") == 0) {
      printf("ERROR: wrong dirname (you can't delete directory you are in)\n");
      return;
   }
   
   //split *filename into path and actual filename
   char* dirname;
   int index;
   if (get_number_of_entries(path) == 0) {
      if (strcmp(path, "..") == 0 || strcmp(path, ".") == 0) {
         printf("ERROR, wrong filename\n");
         return;
      } else {
         dirname = malloc(strlen(path));
         strcpy(dirname, path);
         index = currentDirIndex;
      }
   } else {
      char* newpath = get_path_except_last(path);
      dirname = get_path_last(path);
      
      index = parse_path(newpath);
      if (index < 0) { //trying to create directory in directory that does not exist
         printf("ERROR: file not found\n");
         return;
      }
   }
   
   
   diskblock_t dirblock;
   readblock(&dirblock, index);
   
   int i = find_dir(dirblock.dir, dirname);
   if (i < 0) {
      printf("ERROR: directory not in given directory\n");
      return;
   }
   
   int block_index_of_entry = dirblock.dir.entrylist[i].firstblock;
   
   if (currentDirIndex == block_index_of_entry) {
      printf("ERROR: you cannot delete your current directory");
      return;
   }
   
   diskblock_t dirblock2;
   readblock(&dirblock2, block_index_of_entry);
   
   if (is_dir_empty(dirblock2.dir)) { //check if directory is emptry
      dirblock.dir.entrylist[i].unused = 1;
      clear_fat_entries(block_index_of_entry);
      copyFAT();
      
      writeblock(&dirblock, index);
   } else {
      printf("ERROR: Directory will not be deleted - directory is not empty\n");
   }
}

void copyfile(MyFILE* s, MyFILE* dest)
{
   //iterate through all characters
   for (int i = 0; i < s->filelength; i++) 
      myfputc(myfgetc(s), dest);
}

void myfcp(const char* source, const char* destination)
{
   MyFILE* sourcefile = myfopen(source, "r");
   if (!sourcefile) {
      printf("ERROR: copy source file does not exist\n");
      return;
   }
   if (myfopen(destination, "r")) {
      printf("ERROR: copy destination file already exists\n");
      return;
   }
   MyFILE* destinationfile = myfopen(destination, "w");
   
   copyfile(sourcefile, destinationfile);
   
   myfclose(sourcefile); myfclose(destinationfile);
}

void myfmv(const char* source, const char* destination)
{
   MyFILE* sourcefile = myfopen(source, "r");
   if (!sourcefile) {
      printf("ERROR: move source file does not exist\n");
      return;
   }
   if (myfopen(destination, "r")) {
      printf("ERROR: move destination file already exists\n");
      return;
   }
   MyFILE* destinationfile = myfopen(destination, "w");
   
   copyfile(sourcefile, destinationfile);
   
   myfclose(sourcefile); myfclose(destinationfile);
   myremove((char*)source);
}
   