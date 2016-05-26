class inode:
    def __init__(self, inode_nr):
        self.inode_number = inode_nr
        self.refcount = 0
        self.referenced_by = []
        self.ptrs = []

    def add_reference(self, directory_ref):
        directoryInode, entrynum = directory_ref
        self.referenced_by.append((directoryInode, entrynum))
        self.refcount += 1


class block:
    def __init__(self, block_nr):
        self.block_number = block_nr
        self.refcount = 0
        self.referenced_by = []
    
    def add_reference(self, inode_ref):
        inode_nr, entrynum = inode_ref
        self.referenced_by.append((inode_nr, entrynum))
        self.refcount += 1
