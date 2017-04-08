#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>

#define PAS_NUMBER_OF_FRAMES 8  // number of frames in physical address space
#define VAS_NUMBER_OF_PAGES 16
#define TLB_NUMBER_OF_ENTRIES 4       // number of entries in TLB
#define FRAME_SIZE 256        // size of a frame
#define PAGE_SIZE 256  // size of a page

struct pageData {
    int page_number;
    int frame_number;
};

struct TLBData {
    int page_number;
    int frame_number;
};

int physicalMemory[PAS_NUMBER_OF_FRAMES][FRAME_SIZE]; // physical memory implemented as a 1D array (0...7) of 1D arrays (0...255)
pageData pageTable[VAS_NUMBER_OF_PAGES];
TLBData TLB[TLB_NUMBER_OF_ENTRIES];


#define BUFFER_SIZE 12

// input file and backing store
FILE *virtual_addresses_file;
FILE *backing_store;

char str_address[BUFFER_SIZE];
int virtual_address;
int num_page_faults=0;
int lru_frame=0;
signed char buf[256];

// function headers
void insertIntoPageTable(int virtual_address);
void loadBackingStore(int pageNumber);
void insertIntoTLB(int pageNumber, int frameNumber);

void insertIntoPageTable(int virtual_address){
    // obtain the page number and offset from the logical address
    int pageNumber = virtual_address / PAGE_SIZE;
    int offset = virtual_address % PAGE_SIZE;
    int frameNumber= -1;
    
    //search page table
        int i; 
        for(i = 0; i < VAS_NUMBER_OF_PAGES; i++){
            //printf("%d\n",pageTable[i].page_number);
            //if page referenced is in the page table
            if (pageTable[i].page_number==pageNumber){
                //get the frame number associated with it
                frameNumber = pageTable[i].frame_number;
                printf("Page %d is contained in frame %d\n",pageNumber,frameNumber );
            }
        }
    //the page referenced was not in the page table
        if (frameNumber== -1){
            //page fault
            num_page_faults++;
            printf("Virtual Address %d contained in page %d causes a page fault\n", virtual_address, pageNumber);
            //load the page stored in backing store into the first free frame
            loadBackingStore(pageNumber);
        }
    
    // value = physicalMemory[frameNumber][offset];  // frame number and offset used to get the signed value stored at that address
    //printf("frame number: %d\n", frameNumber);

}

void loadBackingStore(int pageNumber){
    // load page from the backing store and bring the frame into physical memory and the page table

    // load the bits into the physical memory 2D array using LRU
    fseek(backing_store, pageNumber*PAGE_SIZE, SEEK_SET);
    fread(buf, sizeof(signed char), PAGE_SIZE, backing_store);
    int i;
    for (i=0; i<PAGE_SIZE;i++){
        if (lru_frame < PAS_NUMBER_OF_FRAMES){
            physicalMemory[lru_frame][i] = buf[i];
        } else {
            //get least recently used frame to replace


        }
    }
    lru_frame++;

    //load info into the page table

    
    // and then load the frame number into the page table in the first available frame
    // pageTableNumbers[firstAvailablePageTableNumber] = pageNumber;
    // pageTableFrames[firstAvailablePageTableNumber] = firstAvailableFrame;
    
}

// main opens necessary files and calls on getPage for every entry in the addresses file
int main(int argc, char *argv[])
{
    //error if no input file entered
    if (argc != 2) {
        fprintf(stderr,"Please enter an input file of virtual addresses as well \n");
        return -1;
    } else {
        virtual_addresses_file = fopen(argv[1], "r");
        if (virtual_addresses_file == NULL) {
            fprintf(stderr, "Error opening %s\n",argv[1]);
            return -1;
        }
    }
    
    backing_store = fopen("BACKING_STORE.bin", "rb");
    
    if (backing_store == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin");
        return -1;
    }
    
    // read the virtual addresses file
    while ( fgets(str_address, BUFFER_SIZE, virtual_addresses_file) != NULL) {
        virtual_address = atoi(str_address);
      
        //determine page number from virtual address and put in page table
        insertIntoPageTable(virtual_address);
    }
    

    fclose(backing_store);
    fclose(virtual_addresses_file);
    
    return 0;
}

