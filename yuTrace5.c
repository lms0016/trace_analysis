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
unsigned long long pc, inode, blkno;
int start;

// The global counter of inode
unsigned long long glo_inode = 0; //0~4294697295
// The global counter of block
unsigned long long glo_blkno = 0; //0~18446744073709551615
// The global counter of pc
unsigned long long glo_pc = 0;
//LMS
//用來計算連續的比例
int currentBlock = -1;
int currentInode = -1;
double numerator = 0;
double denominator = 1;
double FracOfCons = 0;

// Record program counter
struct pc_item {
    unsigned long long pc;
    unsigned long long old_pc;
    struct pc_item *next;
};

struct pc_item *myPCTable = NULL;

// Record which block within inode table
/*
struct blk_item {
    unsigned long long blkno;
    unsigned long long old_blkno;
    struct blk_item *next;
};
*/

// Record which inode has the maximum of block number
struct inode_item {
    unsigned long long inode;
    unsigned long long max_blkno;
    unsigned long long min_blkno;
    struct inode_item *next;
};
struct inode_item *myInodeTable = NULL;
//struct inode_item *tailInode = NULL;

char TAG = TAG_DEFAULT;


//LMS
//用來計算連續的比例
void ConsRatio(unsigned long long block, unsigned long long inode){	
	if (currentBlock == -1)
	{
		currentBlock = block;
		currentInode = inode;		
	}else
	{			
		if (currentInode == inode && (block == currentBlock + 1 || block == currentBlock))
		{
			numerator++;
			denominator++;
		}else{
			denominator++;			
		}
		currentBlock = block;
		currentInode = inode;	
	}
	FracOfCons = (numerator/denominator)*100;
}

void printInodeTable() {
    FILE *fInode;
    fInode = fopen("fInode", "w");
    struct inode_item *searcher;
    searcher = myInodeTable;
    while(searcher != NULL) {
        fprintf(fInode, "Inode:%llu, max_blkno:%llu, min_blkno:%llu\n", searcher->inode, searcher->max_blkno, searcher->min_blkno);
        glo_blkno += (searcher->max_blkno - searcher->min_blkno + 1);
        searcher = searcher->next;
    }
    //printf("Total inode:%llu\n", glo_inode);
    //printf("Total blocks:%llu\n", glo_blkno);
    fclose(fInode);
}

int constructInodeTable(unsigned long long targetInode, unsigned long long targetBlkno) {
	if(myInodeTable != NULL) {
	       struct inode_item *searcher;
    	       searcher = myInodeTable;
                     // New an inode and it will be the head of 'myInodeTable'
    	       if(searcher->inode > targetInode){
                           struct inode_item *temp;
                           temp = (struct inode_item *)malloc(sizeof(struct inode_item));
                           temp->inode = targetInode;
                           temp->max_blkno = targetBlkno;
                           temp->min_blkno = targetBlkno;
                           temp->next = searcher;
                           myInodeTable = temp;
                           glo_inode++;
                           //printf("[INFO]updateInodeTable:New Header Inode Item \n");
                           return 0;
                     }
                     // Find an inode and add its
    	       else if(searcher->inode == targetInode) {
    		if(targetBlkno > searcher->max_blkno) {
    			searcher->max_blkno = targetBlkno;
    			//printf("[INFO]updateInodeTable:Update Max Blockno of Header Inode Item\n");
    		}
                            else if(targetBlkno < searcher->min_blkno) {
                                         searcher->min_blkno = targetBlkno;
                                         //printf("[INFO]updateInodeTable:Update Min Blockno of Header Inode Item\n");
                            }
    		return 0;
    	       }
                     // New an inode and it will be the tail of 'myInodeTable'
    	       else if(searcher->inode < targetInode && searcher->next == NULL) {
    		struct inode_item *temp;
    		temp = (struct inode_item *)malloc(sizeof(struct inode_item));
    		temp->inode = targetInode;
    		temp->max_blkno = targetBlkno;
                            temp->min_blkno = targetBlkno;
    		temp->next = NULL;
    		searcher->next = temp;
    		glo_inode++;
    		//printf("[INFO]updateInodeTable:New Inode Item after Header Inode Item \n");
    		return 0;
    	       }
    	       else {
    	              while(searcher->next != NULL) {
    	    	      // Some one inode in 'myInodeTable'
    	    	      if(searcher->inode == targetInode) {
    	    		if(targetBlkno > searcher->max_blkno) {
    	    			searcher->max_blkno = targetBlkno;
    	    			//printf("[INFO]updateInodeTable:Update Max Blockno \n");
    	    		}
                                          else if(targetBlkno < searcher->min_blkno) {
                                                        searcher->min_blkno = targetBlkno;
                                                        //printf("[INFO]updateInodeTable:Update Min Blockno of Header Inode Item\n");
                                          }
    	    		return 0;
    	    	      }
    	    	      else if(searcher->inode < targetInode) {
    	    		if(searcher->next->inode > targetInode) {
    	    			struct inode_item *temp;
    	    			temp = (struct inode_item *)malloc(sizeof(struct inode_item));
    	    			temp->inode = targetInode;
    	    			temp->max_blkno = targetBlkno;
                                                        temp->min_blkno = targetBlkno;
    	    			temp->next = searcher->next;
    	    			searcher->next = temp;
    	    			glo_inode++;
    	    			//printf("[INFO]updateInodeTable:New Inode Item between Two Inode Items \n");
    	    			return 0;
    	    		}
    	    	      }
    	    	      searcher = searcher->next;
    	              }
    	              if(searcher->inode == targetInode) {
    	    	      if(targetBlkno > searcher->max_blkno) {
    	    		searcher->max_blkno = targetBlkno;
    	    		//printf("[INFO]updateInodeTable:Update Max Blockno of Tail Inode Item\n");
    	    	      }
                                  else if(targetBlkno < searcher->min_blkno) {
                                          searcher->min_blkno = targetBlkno;
                                          //printf("[INFO]updateInodeTable:Update Min Blockno of Header Inode Item\n");
                                  }
    	    	      return 0;
    	               }
    	              else {
    	    	      struct inode_item *temp;
    	    	      temp = (struct inode_item *)malloc(sizeof(struct inode_item));
    	    	      temp->inode = targetInode;
    	    	      temp->max_blkno = targetBlkno;
                                  temp->min_blkno = targetBlkno;
    	    	      temp->next = NULL;
    	    	      searcher->next = temp;
    	    	      glo_inode++;
    	    	      //printf("[INFO]updateInodeTable:New Tail Inode Item \n");
    	    	      return 0;
    	              }
    	       }
	}
	else{
		myInodeTable = (struct inode_item *)malloc(sizeof(struct inode_item));
		myInodeTable->inode = targetInode;
		myInodeTable->max_blkno = targetBlkno;
                            myInodeTable->min_blkno = targetBlkno;
		myInodeTable->next = NULL;
		glo_inode++;
		//printf("[INFO]updateInodeTable:New first Inode Item in table\n");
		return 0;
	}
	// Return error
	return -1;
}

unsigned long long lookupInodeTable(unsigned long long targetInode, unsigned long long targetBlkno) {
	if(myInodeTable != NULL) {
		struct inode_item *searcher;
    	              searcher = myInodeTable;
    	              unsigned long long newBlkno=0;
    	              while(searcher != NULL) {
    		      if(searcher->inode == targetInode) {
    			newBlkno += targetBlkno;
    			//printf("%llu->%llu\n", targetBlkno, newBlkno);
    			return newBlkno;
    		      }
    		      else {
                                          if (searcher->min_blkno > 0) {
    			     newBlkno += (searcher->max_blkno - searcher->min_blkno +1);
                                               newBlkno +=1; // A space for the fraction of consecutive references
    			}
                                          else
                                                newBlkno += (searcher->max_blkno +1);
                                            //printf("%llu\n", newBlkno);
    		      }
    		      searcher = searcher->next;
    	              }
    	              printf("[ERROR] lookupInodeTable:One Inode is Not in Table\n");
    	              exit(1);
              }
              else {
    	       printf("[ERROR] lookupInodeTable:No Inode Table\n");
    	       exit(1);
              }
}

unsigned long long printPCTable() {
    struct pc_item *searcher;
    searcher = myPCTable;
    unsigned long long newPC=0;
    while (searcher != NULL) {
        //printf("PC:%llu, New PC:%llu\n", searcher->old_pc, newPC);
        newPC++;
        searcher = searcher->next;
    }
    //printf("Total pcs:%llu\n", glo_pc);
    return 1;
}

unsigned long long constructPCTable(unsigned long long targetPC) {
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

unsigned long long lookupPCTable(unsigned long long targetPC) {
    struct pc_item *searcher;
    searcher = myPCTable;
    unsigned long long newPC=0;
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
        printf("[ERROR] 1st ROUND:Input file open error.\n");
        exit(1);
    }

    while(!feof(fin)) {
    	start = -100;
        fscanf(fin, "%llu%llu%d%llu", &pc, &inode, &start, &blkno);
        if(start != -100) {
        	//printf("%8u %8u %3d %8u\n", pc, inode, -1, blkno);
        	// Construct inode table
        	if(constructInodeTable(inode, blkno) == -1) {
        		printf("[ERROR] constructInodeTable\n");	
        	}
        	// Construct pc table
        	constructPCTable(pc);
        	
        }
    }
    printInodeTable();
    printPCTable();
    printf("[yuTrace] Total inode:%llu\n", glo_inode);
    printf("[yuTrace] Total blocks:%llu  (Working set size)\n", glo_blkno);
    printf("[yuTrace] Total pcs:%llu\n", glo_pc);
}

void secondRound(const char *format) {
    if (!fin) {
        printf("[ERROR] 2nd ROUND:Input file open error.\n");
        exit(1);
    }

    if (!fout) {
        printf("[ERROR] 2nd ROUND:Output file open error.\n");
        exit(1);
    }

    int flag = 0; // For format2 output (first line)
    unsigned long long previousPC = 0; // For format2 output

    while(!feof(fin)) {
    	start = -100; // Any number to check & stop the last line 
    	fscanf(fin, "%llu%llu%d%llu", &pc, &inode, &start, &blkno);
    	if(start != -100) {
    		// Output file
    		// Modify inode
    		// Modify pc
    		if(!strcmp(format, "0")) {
    			// Output Format0: pc inode -1 blkno
    			fprintf(fout, "%llu %llu %d %llu\n", lookupPCTable(pc), inode, -1, lookupInodeTable(inode, blkno));
    		}
    		else if(!strcmp(format, "1")) {
    			// Output Format1: blkno
    			fprintf(fout, "%llu\n", lookupInodeTable(inode, blkno)); 
    		}
    		else if(!strcmp(format, "2")) {
    			// Output Format2:
    			// -1 pc
    			// blkno
    			// blkno
    			// ...
    			if(flag == 1) {
    				if(previousPC == lookupPCTable(pc)) {
    					fprintf(fout, "%llu\n", lookupInodeTable(inode, blkno));
    				}
    				else {
    					fprintf(fout, "-1 %llu\n%llu\n", lookupPCTable(pc), lookupInodeTable(inode, blkno)); 
    				}
    				previousPC = lookupPCTable(pc);
    			}
    			else if(flag == 0) {
    				fprintf(fout, "-1 %llu\n%llu\n", lookupPCTable(pc), lookupInodeTable(inode, blkno)); 
    				flag =1;
    				previousPC = lookupPCTable(pc);
    			}
    		}
    		else{
    			printf("[ERROR] 2nd ROUND:Output format error.\n");
    			exit(1);
    		}
    	}
    }
}

//LMS
void thirdRound() {
    if (!fin) {
        printf("[ERROR] 3nd ROUND:Input file open error.\n");
        exit(1);
    }

    while(!feof(fin)) {
    	start = -100;
    	fscanf(fin, "%llu%llu%d%llu", &pc, &inode, &start, &blkno);
    	if(start != -100) {  		
            //計算連續的比例 
        	ConsRatio(blkno,inode);   
    	}
    }    
    printf("[yuTrace] Fraction of consecutive references:%.1f\n", FracOfCons);    
}

int main(int argc, char const *argv[]){
    fin = fopen(argv[1], "r");
    fout = fopen(argv[2], "w");

    if (argc != 4) {
        printf("Used:%s inputTrace outputTrace format[0-2]\n", argv[0]);
        exit(1);
    }

    printf("[yuTrace] Output Format: %s   (0:RACE, 1:LIRS, 2:PCLIRS)\n", argv[3]); 

    printf("[yuTrace] Construct inode & pc table ...\n"); 
    firstRound();
    printf("[yuTrace] ***First Round Completed!\n"); 

    rewind(fin);

    printf("[yuTrace] Generate new trace file ...\n"); 
    secondRound(argv[3]);
    printf("[yuTrace] ***Second Round Completed!\n"); 
/*
    while(!feof(fin)) {
        fscanf(fin, "%llu%llu%d%llu", &pc, &inode, &start, &blkno);
        constructPCTable(pc);
    }

    rewind(fin);
*/
    /*
    while(!feof(fin)) {
        fscanf(fin, "%llu%llu%d%llu", &pc, &inode, &start, &blkno);
        fprintf(fout, "%llu %llu %d %llu\n", pc, inode, -1, lookupInodeTable(inode, blkno));
        //fprintf(fout, "%llu %llu %d %llu\n", lookupPCTable(pc), inode, -1, lookupInodeTable(inode, blkno));
        //lookupInodeTable(inode, blkno);
        //printf("PC:%llu , inode:%llu , blkno:%llu, new blkno:%llu\n", pc, inode, blkno, lookupInodeTable(inode, blkno));
        //getchar();
        //constructPCTable(pc);
    }
    
    printf("Total inode:%llu\n", glo_inode);
    printf("Total blocks:%llu\n", glo_blkno);
    printf("Total pcs:%llu\n", glo_pc);
    */
    //printPCTable();
    
    fclose(fin);
    fclose(fout);
    //LMS
    if(!strcmp(argv[3], "0")){
    	fin = fopen(argv[2], "r");
              printf("[yuTrace] Compute the fraction of consecutive references ...\n");
    	thirdRound();
              printf("[yuTrace] ***Third Round Completed!\n"); 
    	fclose(fin);
    }
    return 0;
}

