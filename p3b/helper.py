class inode:
    def __init__(self, inode_nr, nlinks=0):
        self.inode_number = inode_nr
        self.ptrs = []
        self.refcount = nlinks
        self.referenced_by = []

class block:
    def __init__(self, block_nr, nlinks=0):
        self.block_number = block_nr
        self.refcount = nlinks
        self.referenced_by = []
