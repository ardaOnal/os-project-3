#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// int addressSplitScheme[] = {10,10,12};
int table1Length = 1024;
int table2Length = 1024;
int twoLevelPageTable[1024][1024];

int main(int argc, char* argv[]) {
    // unsigned int a = 0x0000a40c;
    // printf("%#010X %d\n",a, a);

    int frameCount = atoi(argv[3]);
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
    int virtualAddresses[vaLength];

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

    // FIFO
    printf("\nFIFO start\n");
    int fifoPointer = 0;

    for ( int i = 0; i < vaLength; i++) {
        printf("Virtual address %#010x\n", virtualAddresses[i]);

        int firstTableIndex = ((virtualAddresses[i] >> 22) & 1023); // 1023 = 1111111111 (get first 10 bits)
        int secondTableIndex = ((virtualAddresses[i] >> 12) & 1023); // 1023 = 1111111111 (get second 10 bits)
        int offset = (virtualAddresses[i] & 0xFFF); // (get last 12 bits)
        //f = ((virtualAddresses[i] >> 10) & 0xFF);
        printf("first %d second %d offset %d\n", firstTableIndex, secondTableIndex, offset);


        if ( twoLevelPageTable[firstTableIndex][secondTableIndex] == -1)
        {
            // page fault

            twoLevelPageTable[firstTableIndex][secondTableIndex] = fifoPointer;
            frames[fifoPointer] = virtualAddresses[i];

            // physical address translation (pa last 12 bits are the offset and the first 20 bits are frame number)
            int shiftedFrameNumber = (fifoPointer << 12);
            printf("shifted frame no %d\n", shiftedFrameNumber);
            int physicalAddress = shiftedFrameNumber + offset;
            printf("Physical address %#010x\n", physicalAddress);

            fifoPointer++;
            if ( fifoPointer == frameCount)
                fifoPointer = 0;

            
        }


        printf("\n");
        
    }


    return 0;
}