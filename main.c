#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// int addressSplitScheme[] = {10,10,12};
int table1Length = 1024;
int table2Length = 1024;
int twoLevelPageTable[1024][1024];

// A linked list node
struct List {
    unsigned int data;
    struct List* next;
};

int addressInRange( unsigned int virtualRegions[], int vrLength, unsigned int virtualAddress) {
    int pointer = 0;
    // printf("vrlength: %d\n", vrLength);
    while ( pointer < vrLength) {
        // printf("va %#010x va %d vr %d vr %d\n", virtualAddress, virtualAddress, virtualRegions[0], virtualRegions[1]);
        if ( virtualAddress >= virtualRegions[pointer++] && virtualAddress < virtualRegions[pointer++])
            return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {

    if ( argc == 7) {
        int frameCount = atoi(argv[3]);
        int alg = atoi(argv[6]);
        char* outfile = argv[4];
        printf("Frame count: %d\n",frameCount);
        printf("Outfile: %s\n", outfile);

        // Infile 1
        FILE *fp;
        int lineCount = 0;  // Line counter (result)
        char* filename = argv[1];
        fp = fopen(filename, "r");

        if (fp == NULL)
        {
            printf("Could not open file %s\n", filename);
            return -1;
        }

        // Extract characters from file and store in character c
        for (char c = getc(fp); c != EOF; c = getc(fp))
            if (c == '\n') // Increment count if this character is newline
                lineCount = lineCount + 1;
    
        // Close the file
        fclose(fp);
        printf("The file %s has %d lines\n", filename, lineCount);

        int vrLength = (lineCount+1)*2;
        unsigned int virtualRegions[vrLength];

        fp = fopen(filename, "r");
        unsigned int num1 = 0;
        unsigned int num2 = 0;
        int index = 0;

        while (EOF != fscanf(fp, "%x %x\n", &num1, &num2))
        {
            virtualRegions[index++] = num1;
            virtualRegions[index++] = num2;

        }

        printf("virtualRegions length: %d\n", vrLength);

        for ( int i = 0; i < vrLength; i++) {
            printf("%#010x\n", virtualRegions[i]);
        }

        fclose(fp);

        // Infile 2
        char* filename2 = argv[2];
        lineCount = 0;
        fp = fopen(filename2, "r");

        if (fp == NULL)
        {
            printf("Could not open file %s\n", filename2);
            return -1;
        }

        // Extract characters from file and store in character c
        for (char c = getc(fp); c != EOF; c = getc(fp))
            if (c == '\n') // Increment count if this character is newline
                lineCount = lineCount + 1;
    
        fclose(fp);
        printf("The file %s has %d lines\n", filename2, lineCount);

        int vaLength = lineCount+1;
        unsigned int virtualAddresses[vaLength];

        fp = fopen(filename2, "r");
        num1 = 0;
        index = 0;

        while (EOF != fscanf(fp, "%x\n", &num1))
        {
            virtualAddresses[index++] = num1;
        }
        fclose(fp);
        
        printf("virtualAddresses length: %d\n", vaLength);

        for ( int i = 0; i < vaLength; i++) {
            printf("%#010x\n", virtualAddresses[i]);
        }

        unsigned int frames[frameCount];
        for ( int i = 0; i < frameCount; i++) {
            frames[i] = -1;
        }

        for ( int i = 0; i < table1Length; i++) 
            for ( int j = 0; j < table2Length; j++)
                twoLevelPageTable[i][j] = -1;


        /**
         * Korpe dedi ki frame table framein kullanilip kullanilmadigini tutsun
         * page table da framelere maplesin
         * 
        */

        if ( alg == 2) {
            // FIFO
            printf("\nFIFO start\n");
            int fifoPointer = 0;

            for ( int i = 0; i < vaLength; i++) {
                // printf("Virtual address %#010x\n", virtualAddresses[i]);

                unsigned int firstTableIndex = ((virtualAddresses[i] >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
                unsigned int secondTableIndex = ((virtualAddresses[i] >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)
                unsigned int offset = (virtualAddresses[i] & 0xFFF); // (get last 12 bits)
                //f = ((virtualAddresses[i] >> 10) & 0xFF);
                // printf("first %d second %d offset %d\n", firstTableIndex, secondTableIndex, offset);

                // Address dogru araliktami check!
                if ( !addressInRange( virtualRegions, vrLength, virtualAddresses[i])) {
                    printf("%#010x e\n", virtualAddresses[i]);
                } 
                else {
                    if ( twoLevelPageTable[firstTableIndex][secondTableIndex] == -1)
                    {
                        // page fault occurred

                        twoLevelPageTable[firstTableIndex][secondTableIndex] = fifoPointer;

                        // Make the updates frames's virtual address's page table index invalid
                        if ( frames[fifoPointer] != -1) {
                            unsigned int removedFrameVA = frames[fifoPointer];
                            unsigned int removedFirstTableIndex = ((removedFrameVA >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
                            unsigned int removedSecondTableIndex = ((removedFrameVA >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)

                            twoLevelPageTable[removedFirstTableIndex][removedSecondTableIndex] = -1;
                        }

                        frames[fifoPointer] = virtualAddresses[i];

                        // physical address translation (last 12 bits are the offset and the first 20 bits are frame number)
                        // printf("fifoPointer %d\n", fifoPointer);
                        unsigned int shiftedFrameNumber = ((fifoPointer << 12) & 0xfffff000);
                        // printf("shifted frame no %d\n", shiftedFrameNumber);
                        unsigned int physicalAddress = shiftedFrameNumber + offset;
                        printf("%#010x x\n", physicalAddress);

                        fifoPointer++;
                        if ( fifoPointer == frameCount)
                            fifoPointer = 0;

                        
                    } else {
                        // page fault did not occur

                        int frameIndex = twoLevelPageTable[firstTableIndex][secondTableIndex];
                        // For the case in which indices are the same but offset is different
                        if ( frames[frameIndex] != virtualAddresses[i])
                            frames[frameIndex] = virtualAddresses[i];

                        // get frame index from page table and print physicall address
                        // printf("frameIndex %d\n", frameIndex);
                        unsigned int shiftedFrameNumber = ((frameIndex << 12) & 0xfffff000);
                        // printf("shifted frame no %d\n", shiftedFrameNumber);
                        unsigned int physicalAddress = shiftedFrameNumber + offset;
                        printf("%#010x\n", physicalAddress);
                    }
                } 

                // printf("\n");
            }
        } 
        else if ( alg == 1) {
            printf("LRU alg\n");

            long int frameTimes[frameCount];
            int framesEmptyPointer = 0;

            for ( int i = 0; i < vaLength; i++) {
                unsigned int firstTableIndex = ((virtualAddresses[i] >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
                unsigned int secondTableIndex = ((virtualAddresses[i] >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)
                unsigned int offset = (virtualAddresses[i] & 0xFFF); // (get last 12 bits)

                // printf("%#010x %d %d %d\n", virtualAddresses[i], firstTableIndex, secondTableIndex, twoLevelPageTable[firstTableIndex][secondTableIndex]);

                // Address dogru araliktami check!
                if ( !addressInRange( virtualRegions, vrLength, virtualAddresses[i])) {
                    printf("%#010x e\n", virtualAddresses[i]);
                } 
                else {
                    if ( twoLevelPageTable[firstTableIndex][secondTableIndex] == -1)
                    {
                        // page fault occurred

                        if(framesEmptyPointer < frameCount){
                            // main memory is not full
                            twoLevelPageTable[firstTableIndex][secondTableIndex] = framesEmptyPointer;
                            frames[framesEmptyPointer] = virtualAddresses[i];
                            frameTimes[framesEmptyPointer] = i; //TIME
                            

                            // get frame index from page table and print physicall address
                            // printf("frameIndex %d\n", frameIndex);
                            unsigned int shiftedFrameNumber = ((framesEmptyPointer << 12) & 0xfffff000);
                            // printf("shifted frame no %d\n", shiftedFrameNumber);
                            unsigned int physicalAddress = shiftedFrameNumber + offset;
                            printf("%#010x x\n", physicalAddress);

                            framesEmptyPointer++;   
                        }
                        else{
                            // LRU because frames are full
                            
                            int min = frameTimes[0];
                            int minIndex = 0;
                            for(int j = 0; j < frameCount; j++){
                                if ( min > frameTimes[j]) {
                                    min = frameTimes[j];
                                    minIndex = j;
                                }
                            }

                            int lruVictimIndex = minIndex;
                            twoLevelPageTable[firstTableIndex][secondTableIndex] = lruVictimIndex;
                            
                            unsigned int removedFrameVA = frames[lruVictimIndex];
                            unsigned int removedFirstTableIndex = ((removedFrameVA >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
                            unsigned int removedSecondTableIndex = ((removedFrameVA >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)

                            twoLevelPageTable[removedFirstTableIndex][removedSecondTableIndex] = -1;

                            frames[lruVictimIndex] = virtualAddresses[i];
                            frameTimes[lruVictimIndex] = i; //UPDATE TIME


                            // get frame index from page table and print physicall address
                            // printf("frameIndex %d\n", frameIndex);
                            unsigned int shiftedFrameNumber = ((lruVictimIndex << 12) & 0xfffff000);
                            // printf("shifted frame no %d\n", shiftedFrameNumber);
                            unsigned int physicalAddress = shiftedFrameNumber + offset;
                            printf("%#010x x\n", physicalAddress);

                        }
                    }
                    else {

                        // Page fault did not occur
                        int frameIndex = twoLevelPageTable[firstTableIndex][secondTableIndex];
                        frameTimes[frameIndex] = i;

                        // get frame index from page table and print physicall address
                        // printf("frameIndex %d\n", frameIndex);
                        unsigned int shiftedFrameNumber = ((frameIndex << 12) & 0xfffff000);
                        // printf("shifted frame no %d\n", shiftedFrameNumber);
                        unsigned int physicalAddress = shiftedFrameNumber + offset;
                        printf("%#010x\n", physicalAddress);
                        
                    }
                }
            }


        } else {
            printf("Error: incorrect algorithm argument\n");
        }
    }
    else if ( argc == 9) {
        printf("Randomized case\n");

        int frameCount = atoi(argv[1]);
        int alg = atoi(argv[4]);
        char* outfile = argv[2];
        printf("Frame count: %d\n",frameCount);
        printf("Outfile: %s\n", outfile);
        printf("Alg: %d\n", alg);

        char* hexvmsize = argv[6];
        unsigned int h;
        sscanf(hexvmsize, "%x", &h);

        printf("%s %#010x %u\n", hexvmsize, h, h);


        int addrcount = atoi(argv[8]);
        printf("addrcount %d\n", addrcount);


        unsigned int virtualAddresses[addrcount];

        int vaLength = addrcount;

        // Randomized virtualAddresses
        srand(time(NULL));
        for ( int i = 0; i < addrcount; i++) {
            virtualAddresses[i] = rand() % h;
            printf("%d\n", virtualAddresses[i]);

        }

        unsigned int frames[frameCount];
        for ( int i = 0; i < frameCount; i++) {
            frames[i] = -1;
        }

        for ( int i = 0; i < table1Length; i++) 
            for ( int j = 0; j < table2Length; j++)
                twoLevelPageTable[i][j] = -1;

        if ( alg == 2) {
            // FIFO
            printf("\nFIFO start\n");
            int fifoPointer = 0;

            for ( int i = 0; i < vaLength; i++) {
                // printf("Virtual address %#010x\n", virtualAddresses[i]);

                unsigned int firstTableIndex = ((virtualAddresses[i] >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
                unsigned int secondTableIndex = ((virtualAddresses[i] >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)
                unsigned int offset = (virtualAddresses[i] & 0xFFF); // (get last 12 bits)
                //f = ((virtualAddresses[i] >> 10) & 0xFF);
                // printf("first %d second %d offset %d\n", firstTableIndex, secondTableIndex, offset);

            
                
                if ( twoLevelPageTable[firstTableIndex][secondTableIndex] == -1)
                {
                    // page fault occurred

                    twoLevelPageTable[firstTableIndex][secondTableIndex] = fifoPointer;

                    // Make the updates frames's virtual address's page table index invalid
                    if ( frames[fifoPointer] != -1) {
                        unsigned int removedFrameVA = frames[fifoPointer];
                        unsigned int removedFirstTableIndex = ((removedFrameVA >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
                        unsigned int removedSecondTableIndex = ((removedFrameVA >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)

                        twoLevelPageTable[removedFirstTableIndex][removedSecondTableIndex] = -1;
                    }

                    frames[fifoPointer] = virtualAddresses[i];

                    // physical address translation (last 12 bits are the offset and the first 20 bits are frame number)
                    // printf("fifoPointer %d\n", fifoPointer);
                    unsigned int shiftedFrameNumber = ((fifoPointer << 12) & 0xfffff000);
                    // printf("shifted frame no %d\n", shiftedFrameNumber);
                    unsigned int physicalAddress = shiftedFrameNumber + offset;
                    printf("%#010x %#010x x\n", virtualAddresses[i], physicalAddress);

                    fifoPointer++;
                    if ( fifoPointer == frameCount)
                        fifoPointer = 0;

                } else {
                    // page fault did not occur

                    int frameIndex = twoLevelPageTable[firstTableIndex][secondTableIndex];
                    // For the case in which indices are the same but offset is different
                    if ( frames[frameIndex] != virtualAddresses[i])
                        frames[frameIndex] = virtualAddresses[i];

                    // get frame index from page table and print physicall address
                    // printf("frameIndex %d\n", frameIndex);
                    unsigned int shiftedFrameNumber = ((frameIndex << 12) & 0xfffff000);
                    // printf("shifted frame no %d\n", shiftedFrameNumber);
                    unsigned int physicalAddress = shiftedFrameNumber + offset;
                    printf("%#010x %#010x\n", virtualAddresses[i], physicalAddress);
                }
            

                // printf("\n");
            }
        } 
        else if ( alg == 1) {
            printf("LRU alg\n");

            long int frameTimes[frameCount];
            int framesEmptyPointer = 0;

            for ( int i = 0; i < vaLength; i++) {
                unsigned int firstTableIndex = ((virtualAddresses[i] >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
                unsigned int secondTableIndex = ((virtualAddresses[i] >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)
                unsigned int offset = (virtualAddresses[i] & 0xFFF); // (get last 12 bits)

                // printf("%#010x %d %d %d\n", virtualAddresses[i], firstTableIndex, secondTableIndex, twoLevelPageTable[firstTableIndex][secondTableIndex]);

                if ( twoLevelPageTable[firstTableIndex][secondTableIndex] == -1)
                {
                    // page fault occurred

                    if(framesEmptyPointer < frameCount){
                        // main memory is not full
                        twoLevelPageTable[firstTableIndex][secondTableIndex] = framesEmptyPointer;
                        frames[framesEmptyPointer] = virtualAddresses[i];
                        frameTimes[framesEmptyPointer] = i; //TIME
                        

                        // get frame index from page table and print physicall address
                        // printf("frameIndex %d\n", frameIndex);
                        unsigned int shiftedFrameNumber = ((framesEmptyPointer << 12) & 0xfffff000);
                        // printf("shifted frame no %d\n", shiftedFrameNumber);
                        unsigned int physicalAddress = shiftedFrameNumber + offset;
                        printf("%#010x %#010x x\n", virtualAddresses[i], physicalAddress);

                        framesEmptyPointer++;   
                    }
                    else{
                        // LRU because frames are full
                        
                        int min = frameTimes[0];
                        int minIndex = 0;
                        for(int j = 0; j < frameCount; j++){
                            if ( min > frameTimes[j]) {
                                min = frameTimes[j];
                                minIndex = j;
                            }
                        }

                        int lruVictimIndex = minIndex;
                        twoLevelPageTable[firstTableIndex][secondTableIndex] = lruVictimIndex;
                        
                        unsigned int removedFrameVA = frames[lruVictimIndex];
                        unsigned int removedFirstTableIndex = ((removedFrameVA >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
                        unsigned int removedSecondTableIndex = ((removedFrameVA >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)

                        twoLevelPageTable[removedFirstTableIndex][removedSecondTableIndex] = -1;

                        frames[lruVictimIndex] = virtualAddresses[i];
                        frameTimes[lruVictimIndex] = i; //UPDATE TIME


                        // get frame index from page table and print physicall address
                        // printf("frameIndex %d\n", frameIndex);
                        unsigned int shiftedFrameNumber = ((lruVictimIndex << 12) & 0xfffff000);
                        // printf("shifted frame no %d\n", shiftedFrameNumber);
                        unsigned int physicalAddress = shiftedFrameNumber + offset;
                        printf("%#010x %#010x x\n", virtualAddresses[i], physicalAddress);

                    }
                }
                else {

                    // Page fault did not occur
                    int frameIndex = twoLevelPageTable[firstTableIndex][secondTableIndex];
                    frameTimes[frameIndex] = i;

                    // get frame index from page table and print physicall address
                    // printf("frameIndex %d\n", frameIndex);
                    unsigned int shiftedFrameNumber = ((frameIndex << 12) & 0xfffff000);
                    // printf("shifted frame no %d\n", shiftedFrameNumber);
                    unsigned int physicalAddress = shiftedFrameNumber + offset;
                    printf("%#010x %#010x\n", virtualAddresses[i], physicalAddress);
                    
                }
            }


        } else {
            printf("Error: incorrect algorithm argument\n");
        }
        



    }

    return 0;
}

// 0-65536 & 1048576-1703936 & 268435456-281018368