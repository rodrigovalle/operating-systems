unallocated_inode = "UNALLOCATED INODE < {} > REFERENCED BY DIRECTORY < {} > ENTRY < {} >"
unallocated_block = "UNALLOCATED BLOCK < {} > REFERENCED BY INODE < {} > ENTRY < {} >"
incorrect_entry = "INCORRECT ENTRY IN < {} > NAME < {} > LINK TO < {} > SHOULD BE < {} >"
inode_linkcount = "LINKCOUNT < {} > IS < {} > SHOULD BE < {} >"
missing_inode = "MISSING INODE < {} > SHOULD BE IN FREE LIST < {} >"
block_linkcount = "LINKCOUNT < {} > SHOULD BE < {} >"

multiply_ref_blk = "MULTIPLY REFERENCED BLOCK < {} > BY "
inode_ref = "INODE < {} > ENTRY < {} >"


class warning:
    def __init__(self, outfile):
        self.outfile = open(outfile, mode='w')
        
    def __del__(self):
        self.outfile.close()

    def UNALLOCATED_INODE(self, inode_nr, dir_num, dir_entry):
        print(
            unallocated_inode.format(
                inode_nr,
                dir_num,
                dir_entry
            ), file=self.outfile
        )

    def UNALLOCATED_BLOCK(self, block_nr, ref_inode, entry):
        print(
            unallocated_block.format(
                block_nr,
                ref_inode,
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

    def MULTIPLY_REFERENCED_BLOCK(self, block_nr, inode_ref_list):
        print(
            multiply_ref_blk.format(
                block_nr
            ), file=self.outfile, end=''
        )

        for inode_nr, entry in inode_ref_list:
            print(
                inode_ref.format(
                    inode_nr,
                    entry
                ), file=self.outfile, end=''
            )

        print(file=self.outfile)
