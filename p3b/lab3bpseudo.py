#!/bin/python3
import sys
import pprint
from helper import *
from csv_helper import *

pp = pprint.PrettyPrinter(indent=4)

# directory with csv files can be specified as the first argument
# if no argument, assume csv files are in the current directory
current_dir = ""
if len(sys.argv) == 2:
    current_dir = sys.argv[1];


# UNALLOCATED BLOCKS: blocks that are in use but also listed on the free bitmap
#def unallocated_blocks():
#    group_csv = get_reader('group.csv')
#    bitmap_csv = get_reader('bitmap.csv')
#    inode_csv = get_reader('inode.csv')
#
#    # list comprehension for all bitmaps representing free blocks in group.csv
#    block_bitmap_blocks = [row['block_bitmap_block'] for row in group_csv]
#    free_blocks = set()
#
#    # check which bits in block bitmaps represent free blocks
#    for row in bitmap_csv:
#        if row['bitmap_block'] in block_bitmap_blocks:
#            free_blocks.add(int(row['element_number'], 16))
#
#    # look for all allocated blocks (in inode or indirect)
#    # TODO: probably have to handle indirect blocks too, i'll get there someday
#    # TODO: yeah this definitely still does something wrong
#    allocated_blocks = set()
#    for row in inode_csv:
#        for block in row['block_pointers']:
#            if block != '0':
#                allocated_blocks.add(int(block, 16))
#
#    indirect_blocks = set()
#    #for row in indirect_csv:
#        #for
#    # isn't python wonderful?
#    pp.pprint(allocated_blocks & free_blocks)

def main():
    # build a simple model of the file system, which we'll check for
    # inconsistencies
    r = csv_reader(current_dir)
    group_csv = r.get_reader('group')
    bitmap_csv = r.get_reader('bitmap')
    inode_csv = r.get_reader('inode')

    inodeBitmapBlocks = []

    """ Begin superblock.csv parsing
    """
    #store maxinodenum = total_inodes
    #store maxblocknum = total_blocks - 1 // to account for index offset?
    """ end superblock.csv parsing
    """

    """ Begin group.csv parsing
    """
    #store contained blocks, inode bitmap block, block bitmap block,
    #and inode start table block in the lists we've made (done?)
    """ End group.csv parsing
    """

    """ Begin bitmap.csv parsing
    """
    #if (inode)
        #put in freeInodeList
    #else
        #put in freeBlockList
    """ End bitmap.csv parsing
    """

    """ Begin indirect.csv parsing
    """
    #for line in bitmap.csv
        #store entry in dict as: <blocknum&dir_entry, ptr>
    """ End indirect.csv parsing
    """

    """ Begin inode.csv parsing 
    """
    #for line in inode.csv
        #i.nlinks = ...
        #i.ptrs = ...
        #store i in inodeAllocated
        #n_blocks = total number of blocks used for this inode
        
        #for first 12 blocks
            #if i == n_blocks
                #break
            #if blocknum == 0 or blocknum > maxblocknum
                #INVALID_BLOCK_ERROR
            #else
                #insert blocknum and its block var into block allocated
        
        #if (n_blocks >= 12)
            #call a function that processes first level of indirect
        #if (n_blocks >= 13)
            #process double indirect block
        #if (n_blocks >= 14)
            #process triple indirect block
    """ End inode.csv parsing
    """

    """ Begin directory.csv parsing
    """
    ##populate the directory table, note that key is child inode and value is
    ##parent inode
    #for line in directory.csv
        #append (line.inodenumber of the file entry, line.parent inode number)
    #for line in directory.csv
        #child_inode = inode number of the file  entry
        #parent_inode = parent inode number
        
        #if child_inode in directory table != parent inode or parent inode == 2
            #directoryTable[child_inode] = parent_inode;
        
        #if child_inode in inodeAllocated
            #add(parent_inode, entry_num) into referenced by list
        #else
            #UNALLOCATED_INODE

        ##some checking
        #if entry_num == 0 and child_inode != parent_inode
            #INCORRECT_ENTRY
        #else if entry_num == 1 and child_inode != directoryTable[parent_inode]
            #INCORRECT_ENTRY
    """ End directory.csv parsing
    """

    """ Assorted error checking that doesn't belong to any .csv file
    """
    ##TODO: do what the TA said and make this code umm...
    #for each inode, i, in inodeAllocated
        #linkCnt = len(i.inoderefbylist)
        #if i.inodenum > 10 and linkCnt == 0
            #MISSING_INODE_ERROR
        #else if linkCnt != i.nlinks:
            #INCORRECT_LINK_COUNT // see TA notes for proper output
    
    #for inode, i, in inodefreeList
        #for inode, x, in inodeAllocated
            #if i == x
                #UNALLOCATED_INODE_ERROR
    
    #for n in [11, maxinodenumber]
        #inList = 0;
        #for i in inodefreelist
            #if n == i.inode_nr
                #inList = 1
        #for k in inodeAllocated
            #if n == k.inode_nr
                #inList = 1
        #if (inList != 1)
            #MISSING_INODE

    #for blocks, b, in blockAllocated
        #if len(b.referencedbylist) > 1
            #DUPLICATELY_ALLOCATED_BLOCK
    
    #for blocks, b, in blockAllocated
        #for blocks, n, in blockfreeList
            #if b == n
                #UNALLOCATED_BLOCK
    """ End assorted error checking
    """
    #store 
    #blockBimtapBlocks = []
    #inodeFreeList
    #blockFreeList

if __name__ == '__main__':
    main()
