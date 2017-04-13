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
#define BUFFER_SIZE 12

struct pageData {
    int frame_number;
    //int page_number;
    int valid;
};
struct TLBData {
    int page_number;
    int frame_number;
};

int physicalMemory[PAS_NUMBER_OF_FRAMES][FRAME_SIZE]; // physical memory implemented as a 1D array (0...7) of 1D arrays (0...255)
pageData pageTable[VAS_NUMBER_OF_PAGES];
TLBData TLB[TLB_NUMBER_OF_ENTRIES];
int freeFrameList[PAS_NUMBER_OF_FRAMES] = {0};
int freeFrameCount = 8;

// input file and backing store
FILE *virtual_addresses_file;
FILE *backing_store;

char str_address[BUFFER_SIZE];
int virtual_address;
int num_page_faults=0;
//int lru_frame=0;
signed char buf[256];
int i,j;
int lru_counter_array[VAS_NUMBER_OF_PAGES];
int pageNumber;
int frameNumber;
int total_address_references = 0;


// function headers
void checkPageTable(int virtual_address);
void insertIntoPageTable(int pageNumber, int frameNumber);
void loadBackingStore(int pageNumber);
void insertIntoTLB(int pageNumber, int frameNumber);

void checkPageTable(int virtual_address){
    // obtain the page number and offset from the logical address
    pageNumber = virtual_address / PAGE_SIZE;
    int offset = virtual_address % PAGE_SIZE;
    frameNumber= -1;
 
    
    //search page table


     if (pageTable[pageNumber].valid==0){
        insertIntoPageTable(pageNumber, frameNumber);
     } else if (pageTable[pageNumber].valid==1){
        //it's in the page table and TLB!
        frameNumber = pageTable[pageNumber].frame_number;
        printf("FOUND: page %d is contained in frame %d\n", pageNumber, frameNumber);
        
     }
     
}

void insertIntoPageTable(int pageNumber, int frameNumber){

            //page fault
            num_page_faults++;
            printf("PAGE FAULT: Virtual Address %d contained in page %d causes a page fault\n", virtual_address, pageNumber);
            //load the page stored in backing store into the first free frame
            loadBackingStore(pageNumber);
            if (pageTable[pageNumber].valid==1){
            //get the frame number associated with it
            frameNumber = pageTable[pageNumber].frame_number;
            printf("LOADED: Page %d is loaded into frame %d\n",pageNumber,frameNumber );
            }
            
}

void loadBackingStore(int pageNumber){
    // load page from the backing store and bring the frame into physical memory and the page table

    // load the bits into the physical memory 2D array using LRU
    fseek(backing_store, pageNumber*PAGE_SIZE, SEEK_SET);
    fread(buf, sizeof(signed char), PAGE_SIZE, backing_store);
   
        //if (lru_frame < PAS_NUMBER_OF_FRAMES){
        //if physical memory is not full, put page in physical memory, update page table and TLB
        for (j=0;j<PAS_NUMBER_OF_FRAMES;j++){
            //0 for free
            if (freeFrameList[j]==0){
                for (i=0; i<PAGE_SIZE;i++){
                    physicalMemory[j][i] = buf[i];
                }
                //1 for allocated
                freeFrameList[j]=1;
               // printf("allocated %d\n", j);
                freeFrameCount--;
              //  pageTable[availablePage].frame_number = j;
                pageTable[pageNumber].frame_number = j;
              //  pageTable[availablePage].page_number = pageNumber;
             
                pageTable[pageNumber].valid= 1;
                break;
            
        }
        if (freeFrameCount==0) {
            //if physical memory is full, aka the free frame list is empty
            //get least recently used frame to replace
           // printf("all full! %d\n");
           int frame_to_replace;
           int max_count = lru_counter_array[0];
           int frame_count;
           int oldPage;
           for (i=0; i<VAS_NUMBER_OF_PAGES;i++){
                // printf("page %d is count %d\n",i,lru_counter_array[i] );
                // printf("page %d's frame is frame %d\n",i,pageTable[i].frame_number );
                frame_count = lru_counter_array[i];
                if (frame_count > max_count){
                    max_count = frame_count;
                    oldPage = i;
                }

           }

           frame_to_replace = pageTable[oldPage].frame_number;
           // printf("frame to replace %d\n",frame_to_replace );
           //now load in backing store
           for (i=0; i<PAGE_SIZE;i++){
                physicalMemory[frame_to_replace][i]= buf[i];
            }

            //load into the new page

             pageTable[pageNumber].frame_number = frame_to_replace;

             frameNumber = frame_to_replace;

             pageTable[oldPage].valid = 0;
             pageTable[pageNumber].valid = 1;

            

        }
        
    }
    
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

    //initialize pageTable's valid attribute for each page
    for (i=0; i< VAS_NUMBER_OF_PAGES; i++){
        pageTable[i].valid = 0;
    }
    
    // read the virtual addresses file
    while ( fgets(str_address, BUFFER_SIZE, virtual_addresses_file) != NULL) {
        virtual_address = atoi(str_address);
      
        //determine page number from virtual address and check page table; else put page in page table + TLB
        checkPageTable(virtual_address);

         for (i=0; i<VAS_NUMBER_OF_PAGES; i++){
            if (i!=pageNumber and pageTable[i].valid==1){
                //increment if the page was not referenced
                lru_counter_array[i]++;
               // printf("page %d is count %d\n", i, lru_counter_array[i] );
            } else {
                lru_counter_array[pageNumber] = 0;
               // printf("page %d is count %d\n", i, lru_counter_array[i]);

            }
        } //end for
        total_address_references++;
    } //end while
    
    fclose(backing_store);
    fclose(virtual_addresses_file);

    printf("Total address references: %d\n", total_address_references);
    printf("Page Faults: %d\n", num_page_faults );
    
    return 0;
}

