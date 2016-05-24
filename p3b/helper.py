class inode:
    def __init__(self, inode_nr):
        self.inode_number = inode_nr
        self.refcount = 0
        self.ptrs = []
        self.referenced_by = []

    def add_reference(self, directoryInode, entryNum):
        self.referenced_by.append((directoryInode, entryNum))
        self.refcount += 1


class block:
    def __init__(self, block_nr):
        self.block_number = block_nr
        self.refcount = 0
        self.referenced_by = []
    
    def add_reference(self, inode, indirectblocknum, entrynum):
        self.referenced_by.append((inode, indirectblocknum, entrynum))
        self.refcount += 1
