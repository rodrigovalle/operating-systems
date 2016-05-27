unallocated_inode = "UNALLOCATED INODE < {} > REFERENCED BY"
unallocated_block = "UNALLOCATED BLOCK < {} > REFERENCED BY"
invalid_block = "INVALID BLOCK < {} > IN INODE < {} >"
dup_alloc_block = "MULTIPLY REFERENCED BLOCK < {} > BY"
incorrect_entry = "INCORRECT ENTRY IN < {} > NAME < {} > LINK TO < {} > SHOULD BE < {} >"
inode_linkcount = "LINKCOUNT < {} > IS < {} > SHOULD BE < {} >"
missing_inode = "MISSING INODE < {} > SHOULD BE IN FREE LIST < {} >"
block_linkcount = "LINKCOUNT < {} > SHOULD BE < {} >"

invalid_indirect_block = "INDIRECT BLOCK < {} >"
inode_block_entry = "ENTRY < {} >"

inode_ref = "INODE < {} > ENTRY < {} >"
directory_ref = "DIRECTORY < {} > ENTRY < {} >"


class warning:
    def __init__(self, outfile):
        self.outfile = open(outfile, mode='w')
        
    def __del__(self):
        self.outfile.close()

    def UNALLOCATED_INODE(self, inode_nr, dir_ref_list):
        print(
            unallocated_inode.format(
                inode_nr
            ), file=self.outfile, end=''
        )

        # sort by dir_num (directory inode num) order
        for dir_num, dir_entry in sorted(dir_ref_list, key=lambda ref: ref[0]):
            print(file=self.outfile, end=' ')
            print(
                directory_ref.format(
                    dir_num,
                    dir_entry
                ), file=self.outfile, end=''
            )
        print(file=self.outfile)

    def UNALLOCATED_BLOCK(self, block_nr, inode_ref_list): #[ (ref_inode, entry), ... ]
        print(
            unallocated_block.format(
                block_nr,
            ), file=self.outfile, end=''
        )

        # sort by inode number
        for ref_inode, entry in sorted(inode_ref_list, key=lambda ref: ref[0]):
            print(file=self.outfile, end=' ')
            print(
                inode_ref.format(
                    ref_inode,
                    entry
                ), file=self.outfile, end=''
            )
        print(file=self.outfile)

    def INVALID_BLOCK(self, block_nr, inode_nr, indirect_block_nr, entry):
        print(
            invalid_block.format(
                block_nr,
                inode_nr
            ), file=self.outfile, end=' '
        )

        if indirect_block_nr > 0:
            print(
                invalid_indirect_block.format(
                    indirect_block_nr
                ), file=self.outfile, end=' '
            )

        print(
            incorrect_entry.format(
                entry
            ), file=self.outfile
        )
        

    def INCORRECT_DIR_ENTRY(self, dir_inode, dir_name, dir_link, link_should_be):
        print(
            incorrect_entry.format(
                dir_inode,
                dir_name,
                dir_link,
                link_should_be
            ), file=self.outfile
        )

    def INODE_LINKCOUNT(self, inode_nr, link_count, should_be):
        print(
            inode_linkcount.format(
                inode_nr,
                link_count,
                should_be
            ), file=self.outfile
        )

    def BLOCK_LINKCOUNT(self, block_nr, link_count, should_be):
        print(
            block_linkcount.format(
                block_nr,
                link_count,
                should_be
            ), file=self.outfile
        )

    def MISSING_INODE(self, inode_nr, correct_free_list):
        print(
            missing_inode.format(
                inode_nr,
                correct_free_list
            ), file=self.outfile
        )

    def DUPLICATELY_ALLOCATED_BLOCK(self, block_nr, inode_ref_list):
        print(
            dup_alloc_block.format(
                block_nr
            ), file=self.outfile, end=''
        )

        # sort by inode_nr
        for ref_inode, entry in sorted(inode_ref_list, key=lambda ref: ref[0]):
            print(file=self.outfile, end=' ')
            print(
                inode_ref.format(
                    ref_inode,
                    entry
                ), file=self.outfile, end=''
            )
        print(file=self.outfile)
