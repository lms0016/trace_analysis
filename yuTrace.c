#include <stdio.h>
#include <stdlib.h>

#define TAG_DEFAULT 0
#define TAG_INODE_NOTFOUND 1
#define TAG_INODE_FOUND_BUT_BLK_NOTFOUND 2
#define TAG_INODE_FOUND_AND_BLK_FOUND 3

/******
 * inode, blkno:0 ~ (2^32)-1 without any error
 *******/

// File IO describer
FILE *fin, *fout;
// Register for 'fscanf'
unsigned pc, inode, blkno;
int start;

// The global counter of inode
unsigned glo_inode = 0;
// The global counter of block
unsigned glo_blkno = 0;
// The global counter of pc
unsigned glo_pc = 0;

// Record program counter
struct pc_item {
    unsigned pc;
    unsigned old_pc;
    struct pc_item *next;
};
struct pc_item *myPCTable = NULL;

// Record which block within inode table
struct blk_item {
    unsigned blkno;
    unsigned old_blkno;
    struct blk_item *next;
};

// Record which inode has already access some blocks
struct inode_item {
    unsigned inode;
    unsigned blkcount; // How many blks in 'blk_table'
    struct blk_item *blk_table;
    struct inode_item *next;
};
struct inode_item *myInodeTable = NULL;
struct inode_item *tailInode = NULL;

char TAG = TAG_DEFAULT;

/* Searching for inode, return global blkno*/
unsigned lookupInodeTable(unsigned targetInode, unsigned targetBlk) {
    struct inode_item *searcher;
    searcher = myInodeTable;
    
    while(searcher != NULL) {
        // Some one inode in 'myInodeTable'
        if (searcher->inode == targetInode) {
            if (searcher->blkcount == 0){
                struct blk_item *inserter;
                inserter = (struct blk_item *)malloc(sizeof(struct blk_item));
                inserter->blkno = glo_blkno;
                glo_blkno++;
                inserter->old_blkno = targetBlk;
                inserter->next = NULL;
                searcher->blk_table = inserter;
                searcher->blkcount++;
                return inserter->blkno;
            }
            else {
                struct blk_item *inserter, *picker;
                picker = searcher->blk_table;
                unsigned offset = 0;
                while(picker != NULL) {
                    if(picker->old_blkno == targetBlk) {
                        return picker->blkno;
                    }
                    inserter = picker;
                    picker = picker->next;
                }
                inserter->next = (struct blk_item *)malloc(sizeof(struct blk_item));
                inserter = inserter->next;
                inserter->blkno = glo_blkno;
                glo_blkno++;
                inserter->old_blkno = targetBlk;
                inserter->next = NULL;
                searcher->blkcount++;
                return inserter->blkno;
            }
            /*
            // Some one block is in 'blk_table' of 'myInodeTable'
            if (targetBlk+1 <= searcher->blkcount) {
                // TAG_INODE_FOUND_AND_BLK_FOUND
                TAG = TAG_INODE_FOUND_AND_BLK_FOUND;
                struct blk_item* picker;
                picker = searcher->blk_table;
                int i;
                for (i=0; i<searcher->blkcount; i++) {
                    if(i == targetBlk) {
                        return picker->blkno;
                    }
                    else
                        picker = picker->next;
                }
            }
            else { // Some one block isn't in 'blk_table' of 'myInodeTable'
                // TAG_INODE_FOUND_BUT_BLK_NOTFOUND
                TAG = TAG_INODE_FOUND_BUT_BLK_NOTFOUND;
                // Error control:
                if (targetBlk+1 > searcher->blkcount+1) {
                    printf("Some one blkno error!\nBecause of non-sequential access block stream!\n");
                    printf("PC:%u; inode:%u; blkno:%u; next new block number:%u\n", pc, inode, blkno, glo_blkno);
                    return glo_blkno;
                }
                else{ // New one block under this inode
                    // New first block in this inode item without any block accessed
                    if (searcher->blkcount == 0){
                        struct blk_item* inserter;
                        inserter = (struct blk_item *)malloc(sizeof(struct blk_item));
                        inserter->blkno = glo_blkno;
                        glo_blkno++;
                        inserter->next = NULL;
                        searcher->blk_table = inserter;
                        searcher->blkcount++;
                        return inserter->blkno;
                    }
                    else { // New one block into the bottom of 'blk_table' in this inode 
                        struct blk_item* inserter;
                        inserter = searcher->blk_table;
                        while(inserter->next != NULL) {
                            inserter = inserter->next;
                        }
                        inserter->next = (struct blk_item *)malloc(sizeof(struct blk_item));
                        inserter = inserter->next;
                        inserter->blkno = glo_blkno;
                        glo_blkno++;
                        inserter->next = NULL;
                        searcher->blkcount++;
                        return inserter->blkno;
                    }
                    
                }
            }

            */
        }
        searcher = searcher->next;
    }

    // TAG_INODE_NOTFOUND
    TAG = TAG_INODE_NOTFOUND;
    // Some one inode isn't in 'myInodeTable'
    if(myInodeTable == NULL){
        myInodeTable = (struct inode_item *)malloc(sizeof(struct inode_item));
        glo_inode++;
        tailInode = myInodeTable;
        tailInode->inode = targetInode;
        tailInode->blkcount = 0;
        tailInode->blk_table = NULL;
        tailInode->next = NULL;
        return lookupInodeTable(targetInode, targetBlk);
    }
    else {
        tailInode->next = (struct inode_item *)malloc(sizeof(struct inode_item));
        glo_inode++;
        tailInode = tailInode->next;
        tailInode->inode = targetInode;
        tailInode->blkcount = 0;
        tailInode->blk_table = NULL;
        tailInode->next = NULL;
        return lookupInodeTable(targetInode, targetBlk);
    }
}

unsigned constructPCTable(unsigned targetPC) {
    if (myPCTable != NULL) {
        struct pc_item *searcher;
        searcher = myPCTable;
        // We need to sort items from the smallest to the largest 'old_pc'
        if (searcher->old_pc < targetPC) {
            while(searcher->next != NULL) {
                if (searcher->next->old_pc > targetPC) {
                    struct pc_item *temp=NULL;
                    temp = (struct pc_item *)malloc(sizeof(struct pc_item));
                    temp->pc = glo_pc;
                    glo_pc++;
                    temp->old_pc = targetPC;
                    temp->next = searcher->next;
                    searcher->next = temp;
                    return searcher->next->pc;
                }
                else if (searcher->next->old_pc == targetPC) {
                    return searcher->next->pc;
                }
                else
                    searcher = searcher->next;
            }
            searcher->next = (struct pc_item *)malloc(sizeof(struct pc_item));
            searcher->next->pc = glo_pc;
            glo_pc++;
            searcher->next->old_pc = targetPC;
            searcher->next->next = NULL;
            return searcher->next->pc;
        }
        else if (searcher->old_pc > targetPC) {
            myPCTable = (struct pc_item *)malloc(sizeof(struct pc_item));
            myPCTable->pc = glo_pc;
            glo_pc++;
            myPCTable->old_pc = targetPC;
            myPCTable->next = searcher;
            return myPCTable->pc;
        }
        else
            return searcher->pc;

    }
    else { // No any item in PCTable 
        myPCTable = (struct pc_item *)malloc(sizeof(struct pc_item));
        myPCTable->pc = glo_pc;
        glo_pc++;
        myPCTable->old_pc = targetPC;
        myPCTable->next = NULL;
        return myPCTable->pc;
    }
}
unsigned lookupPCTable(unsigned targetPC) {
    struct pc_item *searcher;
    searcher = myPCTable;
    unsigned newPC=0;
    while (searcher != NULL) {
        if (searcher->old_pc == targetPC)
            return newPC;
        newPC++;
        searcher = searcher->next;
    }
    printf("\'lookupPCTable\' error! Some one pc is not in \'PCTable\'\n");
    exit(1);
}

unsigned printPCTable() {
    struct pc_item *searcher;
    searcher = myPCTable;
    unsigned newPC=0;
    while (searcher != NULL) {
        printf("PC:%u, New PC:%u\n", searcher->old_pc, newPC);
        newPC++;
        searcher = searcher->next;
    }
    return 1;
}

int main(int argc, char const *argv[]){
    fin = fopen(argv[1], "r");
    fout = fopen(argv[2], "w");

    if (argc != 3) {
        printf("Used:%s inputTrace outputTrace\n", argv[0]);
        exit(1);
    }

    if (!fin) {
        printf("Input file open error...\n");
        exit(1);
    }

    if (!fout) {
        printf("Output file open error...\n");
        exit(1);
    }

    while(!feof(fin)) {
        start = -100;
        fscanf(fin, "%u%u%d%u", &pc, &inode, &start, &blkno);
        if(start != -100) {
            constructPCTable(pc);
        }
    }

    rewind(fin);

    while(!feof(fin)) {
        start = -100;
        fscanf(fin, "%u%u%d%u", &pc, &inode, &start, &blkno);
        if(start != -100) {
            //fprintf(fout, "%u %u %d %u\n", pc, inode, -1, lookupInodeTable(inode, blkno));
            fprintf(fout, "%u %u %d %u\n", lookupPCTable(pc), inode, -1, lookupInodeTable(inode, blkno));
        }
        //lookupInodeTable(inode, blkno);
        //printf("PC:%u , inode:%u , blkno:%u, new blkno:%u\n", pc, inode, blkno, lookupInodeTable(inode, blkno));
        //getchar();
        //constructPCTable(pc);
    }
    printf("Total inode:%u\n", glo_inode);
    printf("Total blocks:%u\n", glo_blkno);
    printf("Total pcs:%u\n", glo_pc);
    //printPCTable();
    
    fclose(fin);
    fclose(fout);
    return 0;
}

