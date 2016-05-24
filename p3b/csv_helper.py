import csv
import os

csv_fields = {
    'super.csv': [ "magic_number",
                   "total_inodes",
                   "total_blocks",
                   "block_size",
                   "fragment_size",
                   "blocks_per_group",
                   "inodes_per_group",
                   "fragments_per_group",
                   "first_data_block" ],

    'group.csv': [ "blocks_in_group",
                   "free_blocks",
                   "free_inodes",
                   "directories",
                   "inode_bitmap_block",
                   "block_bitmap_block",
                   "inode_table_block" ],

    'bitmap.csv': [ "bitmap_block",
                    "element_number" ],

    'inode.csv': [ "inode_number",
                   "file_type",
                   "mode",
                   "owner",
                   "group",
                   "ref_count",
                   "creation_time",
                   "modification_time",
                   "access_time",
                   "file_size",
                   "allocated_blocks" ],

    'directory.csv': [ "parent_inode_number",
                       "entry_number",
                       "entry_length",
                       "name_length",
                       "entry_inode",
                       "name" ],

    'indirect.csv': [ "containing_block",
                      "entry_number",
                      "block_pointer" ]
}


class csv_reader:
    def __init__(self, directory):
        self.filepaths = {f: os.path.join(directory, f) for f in list(csv_fields.keys())}
        self.openfiles = []

    def get_reader(self, type):
        filename = type + ".csv"

        rest = None
        if filename == 'inode.csv':
            rest = 'block_pointers'

        f = open(self.filepaths[filename], newline='')
        self.openfiles.append(f)
        return csv.DictReader(f, fieldnames=csv_fields[filename], restkey=rest)

    def __del__(self):
        for file in self.openfiles:
            file.close()

