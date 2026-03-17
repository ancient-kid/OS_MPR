#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 3
#define TLB_SIZE 2
#define ADDRESS_COUNT 100

typedef struct {
    int page;
    int frame;
} TLBEntry;

typedef struct {
    int frame;
    int valid;
} PageTableEntry;

/* Global Structures */

PageTableEntry pageTable[NUM_PAGES];
TLBEntry tlb[TLB_SIZE];
int physicalMemory[NUM_FRAMES][PAGE_SIZE];

int frameTable[NUM_FRAMES];
int lruCounter[NUM_FRAMES];

int nextFreeFrame = 0;
int tlbIndex = 0;

int pageFaults = 0;
int tlbHits = 0;
int totalAddresses = 0;

int algorithmChoice = 1;


/* Utility */

void initializeSystem()
{
    for(int i=0;i<NUM_PAGES;i++)
    {
        pageTable[i].valid = 0;
        pageTable[i].frame = -1;
    }

    for(int i=0;i<NUM_FRAMES;i++)
    {
        frameTable[i] = -1;
        lruCounter[i] = 0;
    }

    for(int i=0;i<TLB_SIZE;i++)
    {
        tlb[i].page = -1;
        tlb[i].frame = -1;
    }

    pageFaults = 0;
    tlbHits = 0;
    totalAddresses = 0;
}


/* TLB Functions */

int searchTLB(int page)
{
    for(int i=0;i<TLB_SIZE;i++)
    {
        if(tlb[i].page == page)
        {
            tlbHits++;
            return tlb[i].frame;
        }
    }
    return -1;
}

void addTLB(int page,int frame)
{
    tlb[tlbIndex].page = page;
    tlb[tlbIndex].frame = frame;
    tlbIndex = (tlbIndex+1) % TLB_SIZE;
}


/* Replacement Algorithms */

int fifoReplace()
{
    static int pointer = 0;

    int victim = pointer;
    pointer = (pointer + 1) % NUM_FRAMES;

    return victim;
}

int lruReplace()
{
    int min = lruCounter[0];
    int victim = 0;

    for(int i=1;i<NUM_FRAMES;i++)
    {
        if(lruCounter[i] < min)
        {
            min = lruCounter[i];
            victim = i;
        }
    }

    return victim;
}


/* Disk Simulation */

void loadPageFromDisk(int page,int frame)
{
    for(int i=0;i<PAGE_SIZE;i++)
        physicalMemory[frame][i] = page;
}


/* Page Fault Handler */

int handlePageFault(int page)
{
    pageFaults++;

    int frame;

    if(nextFreeFrame < NUM_FRAMES)
    {
        frame = nextFreeFrame;
        nextFreeFrame++;
    }
    else
    {
        if(algorithmChoice == 1)
            frame = fifoReplace();
        else
            frame = lruReplace();

        int oldPage = frameTable[frame];
        pageTable[oldPage].valid = 0;
    }

    loadPageFromDisk(page,frame);

    pageTable[page].frame = frame;
    pageTable[page].valid = 1;

    frameTable[frame] = page;

    return frame;
}


/* Address Translation */

void translateAddress(int virtualAddress)
{
    totalAddresses++;

    int page = (virtualAddress >> 8) & 0xFF;
    int offset = virtualAddress & 0xFF;

    printf("\nVirtual Address: %d\n",virtualAddress);
    printf("Page: %d Offset: %d\n",page,offset);

    int frame = searchTLB(page);

    if(frame != -1)
    {
        printf("TLB HIT\n");
    }
    else
    {
        printf("TLB MISS\n");

        if(pageTable[page].valid)
        {
            frame = pageTable[page].frame;
            printf("Page Table HIT\n");
        }
        else
        {
            printf("PAGE FAULT\n");
            frame = handlePageFault(page);
        }

        addTLB(page,frame);
    }

    int physicalAddress = frame * PAGE_SIZE + offset;
    int value = physicalMemory[frame][offset];

    lruCounter[frame] = totalAddresses;

    printf("Frame: %d\n",frame);
    printf("Physical Address: %d\n",physicalAddress);
    printf("Value: %d\n",value);
}


/* Visualization */

void showPageTable()
{
    printf("\nPage Table\n");

    for(int i=0;i<NUM_PAGES;i++)
    {
        if(pageTable[i].valid)
            printf("Page %d -> Frame %d\n",i,pageTable[i].frame);
    }
}

void showFrameTable()
{
    printf("\nFrame Table\n");

    for(int i=0;i<NUM_FRAMES;i++)
    {
        if(frameTable[i] != -1)
            printf("Frame %d -> Page %d\n",i,frameTable[i]);
    }
}

void showTLB()
{
    printf("\nTLB\n");

    for(int i=0;i<TLB_SIZE;i++)
    {
        if(tlb[i].page != -1)
            printf("Page %d -> Frame %d\n",tlb[i].page,tlb[i].frame);
    }
}


/* Statistics */

void showStats()
{
    printf("\n========= Statistics =========\n");

    printf("Total Addresses: %d\n",totalAddresses);
    printf("Page Faults: %d\n",pageFaults);
    printf("Page Fault Rate: %.2f\n",(float)pageFaults/totalAddresses);

    printf("TLB Hits: %d\n",tlbHits);
    printf("TLB Hit Rate: %.2f\n",(float)tlbHits/totalAddresses);
}


/* Address Stream */

void runSimulation()
{
    int address;

    printf("\nEnter -1 to stop\n");

    while(1)
    {
        printf("\nEnter Virtual Address: ");
        scanf("%d",&address);

        if(address == -1)
            break;

        translateAddress(address);
    }
}


/* Menu */

void menu()
{
    int choice;

    while(1)
    {
        printf("\n=================================\n");
        printf(" Virtual Memory Simulation\n");
        printf("=================================\n");
        printf("1. Run Address Translation\n");
        printf("2. Show Page Table\n");
        printf("3. Show Frame Table\n");
        printf("4. Show TLB\n");
        printf("5. Show Statistics\n");
        printf("6. Select Replacement Algorithm\n");
        printf("7. Reset System\n");
        printf("8. Exit\n");

        printf("Choice: ");
        scanf("%d",&choice);

        switch(choice)
        {
            case 1: runSimulation(); break;
            case 2: showPageTable(); break;
            case 3: showFrameTable(); break;
            case 4: showTLB(); break;
            case 5: showStats(); break;

            case 6:
                printf("\n1 FIFO\n2 LRU\nChoice: ");
                scanf("%d",&algorithmChoice);
                break;

            case 7:
                initializeSystem();
                printf("System Reset\n");
                break;

            case 8:
                exit(0);

            default:
                printf("Invalid choice\n");
        }
    }
}


/* Main */

int main()
{
    initializeSystem();
    menu();
    return 0;
}