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
/*
struct blk_item {
    unsigned blkno;
    unsigned old_blkno;
    struct blk_item *next;
};
*/

// Record which inode has the maximum of block number
struct inode_item {
    unsigned inode;
    unsigned max_blkno;
    struct inode_item *next;
};
struct inode_item *myInodeTable = NULL;
//struct inode_item *tailInode = NULL;

char TAG = TAG_DEFAULT;


void printInodeTable() {
    FILE *fInode;
    fInode = fopen("fInode", "w");
    struct inode_item *searcher;
    searcher = myInodeTable;
    while(searcher != NULL) {
        fprintf(fInode, "Inode:%u, max_blkno:%u\n", searcher->inode, searcher->max_blkno);
        glo_blkno += (searcher->max_blkno + 1);
        searcher = searcher->next;
    }
    //printf("Total inode:%u\n", glo_inode);
    //printf("Total blocks:%u\n", glo_blkno);
    fclose(fInode);
}

int constructInodeTable(unsigned targetInode, unsigned targetBlkno) {
	if(myInodeTable != NULL) {
		struct inode_item *searcher;
    	searcher = myInodeTable;
    	if(searcher->inode > targetInode){
    		struct inode_item *temp;
    		temp = (struct inode_item *)malloc(sizeof(struct inode_item));
    		temp->inode = targetInode;
    		temp->max_blkno = targetBlkno;
    		temp->next = searcher;
    		myInodeTable = temp;
    		glo_inode++;
    		printf("[INFO]updateInodeTable:New Header Inode Item \n");
    		return 0;
    	}
    	else if(searcher->inode == targetInode) {
    		if(targetBlkno > searcher->max_blkno) {
    			searcher->max_blkno = targetBlkno;
    			printf("[INFO]updateInodeTable:Update Max Blockno of Header Inode Item\n");
    		}
    		return 0;
    	}
    	else if(searcher->inode < targetInode && searcher->next == NULL) {
    		struct inode_item *temp;
    		temp = (struct inode_item *)malloc(sizeof(struct inode_item));
    		temp->inode = targetInode;
    		temp->max_blkno = targetBlkno;
    		temp->next = NULL;
    		searcher->next = temp;
    		glo_inode++;
    		printf("[INFO]updateInodeTable:New Inode Item after Header Inode Item \n");
    		return 0;
    	}
    	else {
    	    while(searcher->next != NULL) {
    	    	// Some one inode in 'myInodeTable'
    	    	if(searcher->inode == targetInode) {
    	    		if(targetBlkno > searcher->max_blkno) {
    	    			searcher->max_blkno = targetBlkno;
    	    			printf("[INFO]updateInodeTable:Update Max Blockno \n");
    	    		}
    	    		return 0;
    	    	}
    	    	else if(searcher->inode < targetInode) {
    	    		if(searcher->next->inode > targetInode) {
    	    			struct inode_item *temp;
    	    			temp = (struct inode_item *)malloc(sizeof(struct inode_item));
    	    			temp->inode = targetInode;
    	    			temp->max_blkno = targetBlkno;
    	    			temp->next = searcher->next;
    	    			searcher->next = temp;
    	    			glo_inode++;
    	    			printf("[INFO]updateInodeTable:New Inode Item between Two Inode Items \n");
    	    			return 0;
    	    		}
    	    	}

    	    	searcher = searcher->next;
    	    }
    	    if(searcher->inode == targetInode) {
    	    	if(targetBlkno > searcher->max_blkno) {
    	    		searcher->max_blkno = targetBlkno;
    	    		printf("[INFO]updateInodeTable:Update Max Blockno of Tail Inode Item\n");
    	    	}
    	    	return 0;
    	    }
    	    else {
    	    	struct inode_item *temp;
    	    	temp = (struct inode_item *)malloc(sizeof(struct inode_item));
    	    	temp->inode = targetInode;
    	    	temp->max_blkno = targetBlkno;
    	    	temp->next = NULL;
    	    	searcher->next = temp;
    	    	glo_inode++;
    	    	printf("[INFO]updateInodeTable:New Tail Inode Item \n");
    	    	return 0;
    	    }
    	}
 	
	}
	else{
		myInodeTable = (struct inode_item *)malloc(sizeof(struct inode_item));
		myInodeTable->inode = targetInode;
		myInodeTable->max_blkno = targetBlkno;
		myInodeTable->next = NULL;
		glo_inode++;
		printf("[INFO]updateInodeTable:New first Inode Item in table\n");
		return 0;
	}
	// Return error
	return -1;
}

unsigned lookupInodeTable(unsigned targetInode, unsigned targetBlkno) {
	if(myInodeTable != NULL) {
		struct inode_item *searcher;
    	searcher = myInodeTable;
    	unsigned newBlkno=0;
    	while(searcher != NULL) {
    		if(searcher->inode == targetInode) {
    			newBlkno += targetBlkno;
    			//printf("%u->%u\n", targetBlkno, newBlkno);
    			return newBlkno;
    		}
    		else {
    			newBlkno += (searcher->max_blkno+1);
    			//printf("%u\n", newBlkno);
    		}
    		searcher = searcher->next;
    	}
    	printf("[ERROR]lookupInodeTable:One Inode is Not in Table\n");
    	exit(1);
    }
    else {
    	printf("[ERROR]lookupInodeTable:No Inode Table\n");
    	exit(1);
    }
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
    //printf("Total pcs:%u\n", glo_pc);
    return 1;
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

void firstRound() {
	if (!fin) {
        printf("[ERROR]1st ROUND:Input file open error.\n");
        exit(1);
    }

    while(!feof(fin)) {
    	start = -100;
        fscanf(fin, "%u%u%d%u", &pc, &inode, &start, &blkno);
        if(start != -100) {
        	printf("%8u %8u %3d %8u\n", pc, inode, -1, blkno);
        	// Construct inode table
        	if(constructInodeTable(inode, blkno) == -1) {
        		printf("[ERROR]constructInodeTable\n");	
        	}
        	// Construct pc table
        	constructPCTable(pc);
        }
    }
    printInodeTable();
    printPCTable();
    printf("Total inode:%u\n", glo_inode);
    printf("Total blocks:%u\n", glo_blkno);
    printf("Total pcs:%u\n", glo_pc);
}

void secondRound() {
	if (!fin) {
        printf("[ERROR]2nd ROUND:Input file open error.\n");
        exit(1);
    }

    if (!fout) {
        printf("[ERROR]2nd ROUND:Output file open error.\n");
        exit(1);
    }

    while(!feof(fin)) {
    	start = -100;
    	fscanf(fin, "%u%u%d%u", &pc, &inode, &start, &blkno);
    	if(start != -100) {
    		// Output file
    		// Modify inode
    		// Modify pc
    		fprintf(fout, "%u %u %d %u\n", lookupPCTable(pc), inode, -1, lookupInodeTable(inode, blkno));
                            
                            //fprintf(fout, "%u\n", lookupInodeTable(inode, blkno));
    		
    	}
    }
    
}

int main(int argc, char const *argv[]){
    fin = fopen(argv[1], "r");
    fout = fopen(argv[2], "w");

    if (argc != 3) {
        printf("Used:%s inputTrace outputTrace\n", argv[0]);
        exit(1);
    }

    firstRound();
    rewind(fin);
    secondRound();
/*
    while(!feof(fin)) {
        fscanf(fin, "%u%u%d%u", &pc, &inode, &start, &blkno);
        constructPCTable(pc);
    }

    rewind(fin);
*/
    /*
    while(!feof(fin)) {
        fscanf(fin, "%u%u%d%u", &pc, &inode, &start, &blkno);
        fprintf(fout, "%u %u %d %u\n", pc, inode, -1, lookupInodeTable(inode, blkno));
        //fprintf(fout, "%u %u %d %u\n", lookupPCTable(pc), inode, -1, lookupInodeTable(inode, blkno));
        //lookupInodeTable(inode, blkno);
        //printf("PC:%u , inode:%u , blkno:%u, new blkno:%u\n", pc, inode, blkno, lookupInodeTable(inode, blkno));
        //getchar();
        //constructPCTable(pc);
    }
    
    printf("Total inode:%u\n", glo_inode);
    printf("Total blocks:%u\n", glo_blkno);
    printf("Total pcs:%u\n", glo_pc);
    */
    //printPCTable();
    
    fclose(fin);
    fclose(fout);
    return 0;
}

