#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <queue>


#define PAS_NUMBER_OF_FRAMES 8  // number of frames in physical address space
#define VAS_NUMBER_OF_PAGES 16
#define TLB_NUMBER_OF_ENTRIES 4       // number of entries in TLB
#define FRAME_SIZE 256        // size of a frame
#define PAGE_SIZE 256  // size of a page
#define BUFFER_SIZE 12

struct pageData {
    int frame_number;
    int valid;
};
struct TLBData {
    int page_number;
    int frame_number;
    int allocated;
};

int physicalMemory[PAS_NUMBER_OF_FRAMES][FRAME_SIZE]; // physical memory implemented as a 1D array (0...7) of 1D arrays (0...255)
pageData pageTable[VAS_NUMBER_OF_PAGES];
TLBData TLB[TLB_NUMBER_OF_ENTRIES];
int freeFrameList[PAS_NUMBER_OF_FRAMES] = {0};
int freeFrameCount = 8;
std::queue<int> TLB_queue;
int popped;

// input file and backing store
FILE *virtual_addresses_file;
FILE *backing_store;

char str_address[BUFFER_SIZE];
int virtual_address;
int num_page_faults=0;
signed char buf[256];
int i,j;
int lru_counter_array[VAS_NUMBER_OF_PAGES];
int pageNumber;
int frameNumber;
double total_address_references = 0.0;
double TLBHit = 0.0;
int TLB_full=0;
int tlb_hits=0;


// function headers
void checkTLB(int virtual_address);
void checkPageTable(int virtual_address);
void insertIntoPageTable(int pageNumber, int frameNumber);
void loadBackingStore(int pageNumber);
void insertIntoTLB(int pageNumber, int frameNumber);
void replaceTLB();

void checkTLB(int virtual_address){
    //if in TLB, set TLBHit to 1
    pageNumber = virtual_address / PAGE_SIZE;
    for (i=0; i<TLB_NUMBER_OF_ENTRIES;i++){
        if (TLB[i].page_number == pageNumber){
            TLBHit = 1;
            tlb_hits++;
            printf("TLB Hit: page %d is contained in frame %d, found in TLB entry %d\n",pageNumber,TLB[i].frame_number,i );
            break;
        }
    }
    if (TLBHit == 0){
        printf("TLB Miss: page %d is not in the TLB \n", pageNumber);
        checkPageTable(virtual_address);
    }
}

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
        if (total_address_references>TLB_NUMBER_OF_ENTRIES){
            replaceTLB();
        }     
     }
}

void insertIntoPageTable(int pageNumber, int frameNumber){

    //page fault
    num_page_faults++;
    printf("PAGE FAULT: Virtual Address %d contained in page %d causes a page fault!\n", virtual_address, pageNumber);
    //load the page stored in backing store into the first free frame
    loadBackingStore(pageNumber);


    if (pageTable[pageNumber].valid==1){
        //get the frame number associated with it
        frameNumber = pageTable[pageNumber].frame_number;
        printf("LOADED: Page %d is loaded into frame %d\n",pageNumber,frameNumber );
        //load into TLB
        for (i=0; i<TLB_NUMBER_OF_ENTRIES;i++){
            if (TLB[i].allocated==0){
                printf("TLB LOAD: frame %d containing page %d is stored in TLB entry %d\n", frameNumber,pageNumber,i );
                TLB[i].allocated=1;
                TLB[i].page_number = pageNumber;
                TLB[i].frame_number = frameNumber;
                //add to queue to determine FIFO replacement strategy
                TLB_queue.push(pageNumber);

                break;
            } 
          

        } //end for

    if (total_address_references>TLB_NUMBER_OF_ENTRIES){
        replaceTLB();
    }

    } //end if page is in page table
     
            
}

void replaceTLB(){
     
    popped = TLB_queue.front();
    TLB_queue.pop();
    TLB_queue.push(pageNumber);
    for (i=0; i<TLB_NUMBER_OF_ENTRIES;i++){
        if (TLB[i].page_number == popped){
            TLB[i].page_number = pageNumber;
            TLB[i].frame_number = pageTable[pageNumber].frame_number;
            printf("TLB LOAD: frame %d containing page %d is stored in TLB entry %d\n", TLB[i].frame_number,pageNumber,i );

            break;

        }
    }
  
}

void loadBackingStore(int pageNumber){
    // load page from the backing store and bring the frame into physical memory and the page table

    // load the bits into the physical memory 2D array using LRU
    fseek(backing_store, pageNumber*PAGE_SIZE, SEEK_SET);
    fread(buf, sizeof(signed char), PAGE_SIZE, backing_store);
   
       
        //if physical memory is not full, put page in physical memory, update page table and TLB
        for (j=0;j<PAS_NUMBER_OF_FRAMES;j++){
            //0 for free
            if (freeFrameList[j]==0){
                for (i=0; i<PAGE_SIZE;i++){
                    physicalMemory[j][i] = buf[i];
                }
                //1 for allocated
                freeFrameList[j]=1;
                freeFrameCount--;
                pageTable[pageNumber].frame_number = j;
             
                pageTable[pageNumber].valid= 1;
                break;
            
        }
        if (freeFrameCount==0) {
            //if physical memory is full, aka the free frame list is empty
            //get least recently used frame to replace
          
           int frame_to_replace;
           int max_count = lru_counter_array[0];
           int frame_count;
           int oldPage;
           for (i=0; i<VAS_NUMBER_OF_PAGES;i++){
                frame_count = lru_counter_array[i];
                if (frame_count > max_count){
                    max_count = frame_count;
                    oldPage = i;
                }

           }

           frame_to_replace = pageTable[oldPage].frame_number;
           //now load in backing store
           for (i=0; i<PAGE_SIZE;i++){
                physicalMemory[frame_to_replace][i]= buf[i];
            }

            //load into the new page

             pageTable[pageNumber].frame_number = frame_to_replace;

             frameNumber = frame_to_replace;

             pageTable[oldPage].valid = 0;
             pageTable[pageNumber].valid = 1;

        } //end if replacement strategies
        
    }
    
}

// main opens necessary files and calls on getPage for every entry in the addresses file
int main(int argc, char *argv[])
{
   
   
    virtual_addresses_file = fopen("addresses.txt", "r");   
    backing_store = fopen("BACKING_STORE.bin", "rb");
    
    if (backing_store == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin");
        return -1;
    }

    //initialize pageTable's valid attribute for each page
    for (i=0; i< VAS_NUMBER_OF_PAGES; i++){
        pageTable[i].valid = 0;
    }
    //initialize TLB to say it's empty
     for (i=0; i< TLB_NUMBER_OF_ENTRIES; i++){
        TLB[i].allocated = 0;
    }
    
    printf("P = %d byte (page size)\n", PAGE_SIZE );
    printf("VAS = %d pages\n", VAS_NUMBER_OF_PAGES );
    printf("PAS = %d frames\n",PAS_NUMBER_OF_FRAMES );
    printf("TLB = %d entries\n\n",TLB_NUMBER_OF_ENTRIES );

    // read the virtual addresses file
    while ( fgets(str_address, BUFFER_SIZE, virtual_addresses_file) != NULL) {
        TLBHit=0;
        virtual_address = atoi(str_address);
        
        //check TLB first
        checkTLB(virtual_address);
        
        //determine page number from virtual address and check page table; else put page in page table + TLB

         for (i=0; i<VAS_NUMBER_OF_PAGES; i++){
            if (i!=pageNumber && pageTable[i].valid==1){
                //increment if the page was not referenced
                lru_counter_array[i]++;
            } else {
                lru_counter_array[pageNumber] = 0;

            }
        } //end for

        total_address_references++;
        printf("\n");
    } //end while
    
    fclose(backing_store);
    fclose(virtual_addresses_file);

    printf("Total address references: %.0lf\n", total_address_references);
    printf("TLB Hits: %d\n", tlb_hits );
    printf("TLB Hit Ratio: %f\n", tlb_hits/total_address_references);
    printf("Page Faults: %d\n\n", num_page_faults );

    printf("%s\n", "The contents of the page table after simulation: ");
    for (i=0; i<VAS_NUMBER_OF_PAGES;i++){
        if (pageTable[i].valid==1){
            printf("Page %d in frame %d\n", i, pageTable[i].frame_number);
        } else {
            printf("Page %d not in physical memory \n", i);
        }
    }
    printf("\n");

    printf("%s\n","The contents of each frame:" );
    for (j=0; j<PAS_NUMBER_OF_FRAMES; j++){
        for (i=0; i<VAS_NUMBER_OF_PAGES;i++){
            if (pageTable[i].frame_number==j && pageTable[i].valid==1){
                printf("Frame %d contains page %d \n",j, i );
            }
        }
    }
    printf("\n");
    
    return 0;
}

