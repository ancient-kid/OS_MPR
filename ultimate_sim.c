#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ===============================
   CONFIGURATION
================================ */

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 32
#define TLB_SIZE 8
#define MAX_ADDRESSES 5000

/* ===============================
   ANSI COLORS
================================ */

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define CYAN "\033[1;36m"
#define YELLOW "\033[1;33m"
#define RESET "\033[0m"

/* ===============================
   DATA STRUCTURES
================================ */

typedef struct
{
    int frame;
    int valid;
} PageTableEntry;

typedef struct
{
    int page;
    int frame;
} TLBEntry;

/* ===============================
   GLOBAL STATE
================================ */

PageTableEntry pageTable[NUM_PAGES];
TLBEntry tlb[TLB_SIZE];

int physicalMemory[NUM_FRAMES][PAGE_SIZE];
int framePageMap[NUM_FRAMES];
int lruTime[NUM_FRAMES];

int addressStream[MAX_ADDRESSES];
int addressCount = 0;

int nextFreeFrame = 0;
int tlbIndex = 0;

int pageFaults = 0;
int tlbHits = 0;
int totalAddresses = 0;

int algorithm = 1;
int stepMode = 0;

/* ===============================
   CLEAR SCREEN
================================ */

void clear()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* ===============================
   INITIALIZATION
================================ */

void initialize()
{

    for (int i = 0; i < NUM_PAGES; i++)
    {
        pageTable[i].valid = 0;
        pageTable[i].frame = -1;
    }

    for (int i = 0; i < NUM_FRAMES; i++)
    {
        framePageMap[i] = -1;
        lruTime[i] = 0;
    }

    for (int i = 0; i < TLB_SIZE; i++)
    {
        tlb[i].page = -1;
        tlb[i].frame = -1;
    }

    nextFreeFrame = 0;
    tlbIndex = 0;

    pageFaults = 0;
    tlbHits = 0;
    totalAddresses = 0;

    printf(GREEN "System Initialized\n" RESET);
}

/* ===============================
   MEMORY VISUALIZATION
================================ */

void drawMemory()
{

    printf(CYAN "\n===== PHYSICAL MEMORY =====\n" RESET);

    for (int i = 0; i < NUM_FRAMES; i++)
    {

        if (framePageMap[i] == -1)
            printf("[F%-2d: --] ", i);
        else
            printf(GREEN "[F%-2d:P%-2d]" RESET " ", i, framePageMap[i]);

        if ((i + 1) % 4 == 0)
            printf("\n");
    }
}

/* ===============================
   PAGE FAULT GRAPH
================================ */

void drawFaultGraph()
{

    printf(YELLOW "\nPage Fault Graph\n" RESET);

    for (int i = 0; i < pageFaults; i++)
        printf(RED "|");

    printf("\n%d faults\n", pageFaults);
}

/* ===============================
   DISK LOAD
================================ */

void loadPage(int page, int frame)
{

    for (int i = 0; i < PAGE_SIZE; i++)
        physicalMemory[frame][i] = page;
}

/* ===============================
   TLB
================================ */

int searchTLB(int page)
{

    for (int i = 0; i < TLB_SIZE; i++)
    {

        if (tlb[i].page == page)
        {
            tlbHits++;
            return tlb[i].frame;
        }
    }

    return -1;
}

void updateTLB(int page, int frame)
{

    tlb[tlbIndex].page = page;
    tlb[tlbIndex].frame = frame;

    tlbIndex = (tlbIndex + 1) % TLB_SIZE;
}

/* ===============================
   REPLACEMENT ALGORITHMS
================================ */

int fifo()
{

    static int pointer = 0;

    int victim = pointer;
    pointer = (pointer + 1) % NUM_FRAMES;

    return victim;
}

int lru()
{

    int min = lruTime[0];
    int victim = 0;

    for (int i = 1; i < NUM_FRAMES; i++)
    {

        if (lruTime[i] < min)
        {
            min = lruTime[i];
            victim = i;
        }
    }

    return victim;
}

/* ===============================
   PAGE FAULT HANDLER
================================ */

int pageFault(int page)
{

    pageFaults++;

    int frame;

    if (nextFreeFrame < NUM_FRAMES)
    {
        frame = nextFreeFrame++;
    }
    else
    {

        if (algorithm == 1)
            frame = fifo();
        else
            frame = lru();

        int oldPage = framePageMap[frame];
        pageTable[oldPage].valid = 0;
    }

    loadPage(page, frame);

    pageTable[page].frame = frame;
    pageTable[page].valid = 1;

    framePageMap[frame] = page;

    return frame;
}

/* ===============================
   ADDRESS TRANSLATION
================================ */

void translate(int addr)
{

    int page = (addr >> 8) & 255;
    int offset = addr & 255;

    printf("\nAddress %d Page %d Offset %d\n", addr, page, offset);

    int frame = searchTLB(page);

    if (frame != -1)
        printf(GREEN "TLB HIT\n" RESET);

    else
    {

        printf("TLB MISS\n");

        if (pageTable[page].valid)
        {
            frame = pageTable[page].frame;
            printf(GREEN "Page Table HIT\n" RESET);
        }

        else
        {
            printf(RED "PAGE FAULT\n" RESET);
            frame = pageFault(page);
        }

        updateTLB(page, frame);
    }

    int physical = frame * PAGE_SIZE + offset;

    printf("Frame %d Physical %d\n", frame, physical);

    lruTime[frame] = totalAddresses;

    drawMemory();

    drawFaultGraph();

    totalAddresses++;

    if (stepMode)
    {
        printf("\nPress ENTER\n");
        getchar();
        getchar();
    }
}

/* ===============================
   ADDRESS STREAM
================================ */

void generate()
{

    printf("How many addresses: ");
    scanf("%d", &addressCount);

    for (int i = 0; i < addressCount; i++)
        addressStream[i] = rand() % 65536;
}

/* ===============================
   SIMULATION
================================ */

void run()
{

    for (int i = 0; i < addressCount; i++)
        translate(addressStream[i]);
}

/* ===============================
   STATISTICS
================================ */

void stats()
{

    printf("\n===== STATISTICS =====\n");

    printf("Addresses: %d\n", totalAddresses);
    printf("Page Faults: %d\n", pageFaults);
    printf("Fault Rate: %.3f\n", (float)pageFaults / totalAddresses);

    printf("TLB Hits: %d\n", tlbHits);
    printf("TLB Rate: %.3f\n", (float)tlbHits / totalAddresses);
}

/* ===============================
   EXPERIMENT MODE
================================ */

void experiment()
{

    printf("\nRunning Algorithm Comparison\n");

    for (int a = 1; a <= 2; a++)
    {

        initialize();

        algorithm = a;

        for (int i = 0; i < addressCount; i++)
            translate(addressStream[i]);

        if (a == 1)
            printf("\nFIFO RESULTS\n");
        else
            printf("\nLRU RESULTS\n");

        printf("Page Faults: %d\n", pageFaults);
    }
}

/* ===============================
   MENU
================================ */

void menu()
{

    printf(CYAN "\n===== VIRTUAL MEMORY RESEARCH SIMULATOR =====\n" RESET);

    printf("1 Initialize System\n");
    printf("2 Generate Address Stream\n");
    printf("3 Run Simulation\n");
    printf("4 Show Memory\n");
    printf("5 Show Stats\n");
    printf("6 Toggle Step Mode\n");
    printf("7 Select Algorithm\n");
    printf("8 Experiment Mode\n");
    printf("9 Exit\n");

    printf("Choice: ");
}

/* ===============================
   MAIN
================================ */

int main()
{

    srand(time(NULL));

    int c;

    while (1)
    {

        menu();

        scanf("%d", &c);

        switch (c)
        {

        case 1:
            initialize();
            break;

        case 2:
            generate();
            break;

        case 3:
            run();
            break;

        case 4:
            drawMemory();
            break;

        case 5:
            stats();
            break;

        case 6:
            stepMode = !stepMode;
            break;

        case 7:
            printf("1 FIFO\n2 LRU\n");
            scanf("%d", &algorithm);
            break;

        case 8:
            experiment();
            break;

        case 9:
            exit(0);
        }
    }
}