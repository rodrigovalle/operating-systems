import csv
import os

csv_fields = {
    'super.csv': [ ("magic_number", 16),
                   ("total_inodes", 10),
                   ("total_blocks", 10),
                   ("block_size", 10),
                   ("fragment_size", 10),
                   ("blocks_per_group", 10),
                   ("inodes_per_group", 10),
                   ("fragments_per_group", 10),
                   ("first_data_block", 10) ],

    'group.csv': [ ("blocks_in_group", 10),
                   ("free_blocks", 10),
                   ("free_inodes", 10),
                   ("directories", 10),
                   ("inode_bitmap_block", 16),
                   ("block_bitmap_block", 16),
                   ("inode_table_block", 16) ],

    'bitmap.csv': [ ("bitmap_block", 16),
                    ("element_number", 10) ],

    'inode.csv': [ ("inode_number", 10),
                   ("file_type", -1),
                   ("mode", 8),
                   ("owner", 10),
                   ("group", 10),
                   ("ref_count", 10),
                   ("creation_time", 16),
                   ("modification_time", 16),
                   ("access_time", 16),
                   ("file_size", 10),
                   ("allocated_blocks", 10) ],

    'directory.csv': [ ("parent_inode_number", 10),
                       ("entry_number", 10),
                       ("entry_length", 10),
                       ("name_length", 10),
                       ("entry_inode", 10),
                       ("name", -1) ],

    'indirect.csv': [ ("containing_block", 16),
                      ("entry_number", 10),
                      ("block_pointer", 16) ]
}

files = list(csv_fields)


class csv_reader:
    def __init__(self, dir):
        self.filepaths = {f: os.path.join(dir, f) for f in files}
        self.parsedFiles = {}

    def entry_list(self, filename):
        if filename in self.parsedFiles:
            return self.parsedFiles[filename]

        per_line_dict = []
        with open(self.filepaths[filename], newline='') as f:
            for line in csv.reader(f):
                d = {}
                for i, value in enumerate(line):
                    try:
                        key, base = csv_fields[filename][i]
                        if base > 0:  # elif <= zero, assume string
                            value = int(value, base)
                        d[key] = value

                    except IndexError:  # handle inode block pointers
                        key = 'block_pointers'
                        value = [int(s, 16) for s in line[i:]]
                        d[key] = value
                        break

                per_line_dict.append(d)
                
        self.parsedFiles[filename] = per_line_dict
        return per_line_dict
