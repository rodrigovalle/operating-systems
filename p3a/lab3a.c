// Made with love by Rodrigo and Will
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

/* NOTE: #include <ext2fs/ext2_fs.h>
 * not allowed according to Prof. Kampe, but contains a lot of important
 * details about the ext2 file system, so I've read through it
 *
 * ext2fs library manual:  http://www.giis.co.in/libext2fs.pdf
 * other helpful links:    http://www.giis.co.in/manuals.html
 *                         http://wiki.osdev.org/Ext2
 */

/* we use this enum to index the filenames array */
enum outfile {
    SUPER_CSV = 0,
    GROUP_CSV,
    BITMAP_CSV,
    INODE_CSV,
    DIRECTORY_CSV,
    INDIRECT_CSV,
    N_FILES
};

/* our six output files and their names */
static FILE *csv_files[N_FILES];
static char *filenames[N_FILES] = {
    "super.csv",
    "group.csv",
    "bitmap.csv",
    "inode.csv",
    "directory.csv",
    "indirect.csv",
};

/* the number of fields each csv contains */
enum output_fields {
    SUPER_FIELDS      =   9,
    GROUP_FIELDS      =   7,
    BITMAP_FIELDS     =   2,
    INODE_FIELDS      =  26,
    DIRECTORY_FIELDS  =   6,
    INDIRECT_FIELDS   =   3
};

/* specifies a single formatted csv entry
 *
 *   *fmt_str  a printf format string
 *   data      what will be printed/formatted by printf
 */
struct fmt_entry {
    const char *fmt_str;
    uint64_t data;
    uint32_t len;   // optional, used to contain string length
};

// file descriptor reference to the open disk image we're analyzing
static int imgfd = -1;

/* NOTE: these #define are for bitmap_stat() */
#define BLOCK_BITMAP    0
#define INODE_BITMAP    1

/* EXT2 DEFINITIONS
 * these definitions taken directly from <ext2fs/ext2_fs.h>
 * macros were created by us using the ext2 documentation
 */
#define SUPERBLOCK_OFFSET   1024
#define SUPERBLOCK_SIZE     1024
#define EXT2_N_BLOCKS       15
#define EXT2_NAME_LENGTH    255
#define EXT2_S_IFREG        0x8000
#define EXT2_S_IFDIR        0x4000
#define EXT2_S_IFLNK        0xA000
#define EXT2_BLOCK_SIZE(s)  (1024 << (s).s_log_block_size)
#define EXT2_INODE_SIZE(s)  ((s).s_rev_level >= 1 ? (s).s_inode_size : 128)
#define EXT2_FIRST_INODE(s) ((s).s_rev_level >= 1 ? (s).s_first_ino : 11)
#define EXT2_FS_BLOCKS(i,s) ((i).i_blocks / (2 << (s).s_log_block_size))
#define EXT2_FRAG_SIZE(s)   ((s).s_log_frag_size > 0 ? \
                                 1024 << (s).s_log_frag_size : \
                                 1024 >> -(s).s_log_frag_size)
#define EXT2_DIRNAME_MASK(s) ((s).s_rev_level >= 1 ? 0x00FF : 0xFFFF)

// round up the numer of block groups
#define DIV_ROUND_UP(i,j)   (((i) + ((j) - 1)) / (j))
#define EXT2_BLOCKGROUPS(s) (DIV_ROUND_UP((s).s_blocks_count, (s).s_blocks_per_group))

#define EXT2_BLOCK_REMAINDER(s) ((s).s_blocks_per_group - ((EXT2_BLOCKGROUPS(s) * (s).s_blocks_per_group) - (s).s_blocks_count))
#define EXT2_INODE_REMAINDER(s) ((s).s_inodes_per_group - ((EXT2_BLOCKGROUPS(s) * (s).s_inodes_per_group) - (s).s_inodes_count))

/*
 * Structure of the superblock (compatible with all ext2 versions)
 *
 * NOTE: don't allocate this on the stack, it's too big and will cause a
 * stack overflow/segfault. Use malloc instead (or allocate statically).
 */
struct ext2_super_block {
    uint32_t   s_inodes_count;         /* Inodes count */
    uint32_t   s_blocks_count;         /* Blocks count */
    uint32_t   s_r_blocks_count;       /* Reserved blocks count */
    uint32_t   s_free_blocks_count;    /* Free blocks count */
    uint32_t   s_free_inodes_count;    /* Free inodes count */
    uint32_t   s_first_data_block;     /* First data block */
    uint32_t   s_log_block_size;       /* Block size*/
    uint32_t   s_log_frag_size;        /* Allocation cluster/fragment size */
    uint32_t   s_blocks_per_group;     /* Blocks per group */
    uint32_t   s_frags_per_group;      /* # Fragments per group */
    uint32_t   s_inodes_per_group;     /* # Inodes per group */
    uint32_t   s_mtime;                /* Mount time */
    uint32_t   s_wtime;                /* Write time */
    uint16_t   s_mnt_count;            /* Mount count */
     int16_t   s_max_mnt_count;        /* Maximal mount count */
    uint16_t   s_magic;                /* Magic signature */
    uint16_t   s_state;                /* File system state */
    uint16_t   s_errors;               /* Behavior when detecting errors */
    uint16_t   s_minor_rev_level;      /* minor revision level */
    uint32_t   s_lastcheck;            /* time of last check */
    uint32_t   s_checkinterval;        /* max. time between checks */
    uint32_t   s_creator_os;           /* OS */
    uint32_t   s_rev_level;            /* Revision level */
    uint16_t   s_def_resuid;           /* Default uid for reserved blocks */
    uint16_t   s_def_resgid;           /* Default gid for reserved blocks */

    /* EXT2 REVISION 1 -- use the macros to access these values */
    uint32_t   s_first_ino;            /* First inode useable for std files */
    uint16_t   s_inode_size;           /* Size of the inode structure */
} superblock;

/*
 * Structure of a blocks group descriptor.
 */
struct ext2_group_desc
{
	uint32_t	bg_block_bitmap;	    /* Blocks bitmap block */
	uint32_t	bg_inode_bitmap;	    /* Inodes bitmap block */
	uint32_t	bg_inode_table;         /* Inodes table block */
	uint16_t	bg_free_blocks_count;	/* Free blocks count */
	uint16_t	bg_free_inodes_count;	/* Free inodes count */
	uint16_t	bg_used_dirs_count;	    /* Directories count */
	uint16_t	bg_flags;
	uint32_t	bg_exclude_bitmap_lo;	/* Exclude bitmap for snapshots */
	uint16_t	bg_block_bitmap_csum_lo;/* crc32c(s_uuid+grp_num+bitmap) LSB */
	uint16_t	bg_inode_bitmap_csum_lo;/* crc32c(s_uuid+grp_num+bitmap) LSB */
	uint16_t	bg_itable_unused;	    /* Unused inodes count */
	uint16_t	bg_checksum;		    /* crc16(s_uuid+group_num+group_desc)*/
} group;

/*
 * Structure of an inode on the disk
 */                                                                             
struct ext2_inode {
    uint16_t   i_mode;          /* File mode */
    uint16_t   i_uid;           /* Low 16 bits of Owner Uid */
    uint32_t   i_size;          /* Size in bytes */
    uint32_t   i_atime;         /* Access time */
    uint32_t   i_ctime;         /* Inode change time */
    uint32_t   i_mtime;         /* Modification time */
    uint32_t   i_dtime;         /* Deletion Time */
    uint16_t   i_gid;           /* Low 16 bits of Group Id */
    uint16_t   i_links_count;   /* Links count */
    uint32_t   i_blocks;        /* Blocks count */
    uint32_t   i_flags;         /* File flags */
    union {
        struct {
            uint32_t   l_i_version; /* was l_i_reserved1 */
        } linux1;
        struct {
            uint32_t  h_i_translator;
        } hurd1;
    } osd1;         /* OS dependent 1 */
    uint32_t   i_block[EXT2_N_BLOCKS];  /* Pointers to blocks */
    uint32_t   i_generation;            /* File version (for NFS) */
    uint32_t   i_file_acl;              /* File ACL */
    uint32_t   i_size_high;             /* Formerly i_dir_acl, directory ACL */
    uint32_t   i_faddr;                 /* Fragment address */
    union {
        struct {
            uint16_t   l_i_blocks_hi;
            uint16_t   l_i_file_acl_high;
            uint16_t   l_i_uid_high;        /* these 2 fields    */
            uint16_t   l_i_gid_high;        /* were reserved2[0] */
            uint16_t   l_i_checksum_lo;     /* crc32c(uuid+inum+inode) */
            uint16_t   l_i_reserved;
        } linux2;
        struct {
            uint8_t    h_i_frag;        /* Fragment number */
            uint8_t    h_i_fsize;       /* Fragment size */
            uint16_t   h_i_mode_high;
            uint16_t   h_i_uid_high;
            uint16_t   h_i_gid_high;
            uint32_t   h_i_author;
        } hurd2;
    } osd2;             /* OS dependent 2 */
} inode;

/* 
 * Structure of a directory entry
 */
struct ext2_dir_entry {
    uint32_t inode;     /* Inode number */
    uint16_t rec_len;   /* Directory entry length */
    uint16_t name_len;  /* Name length */
    char name[EXT2_NAME_LENGTH];    /* File Name */
};

/* Populates the csv_files array with references to appropriately named and
 * newly created output files.
 */
static void open_csv() {
    for (int i = 0; i < N_FILES; i++) {
        csv_files[i] = fopen(filenames[i], "w");
        if (csv_files[i] == NULL)
            perror("create csv file");
    }
}

/* Closes all csv files. Call after we're finished writing.
 */
static void close_csv()
{
    for (int i = 0; i < N_FILES; i++) {
        fclose(csv_files[i]);
    }
}


/* Writes an array of entries out to the corresponding csv file in the
 * appropriate format. Make sure to call open_files() first.
 * NOTES:
 *  - you can only specify one "type" in the format string. eg %d, %x, etc.
 */
static void write_csv(int file, const struct fmt_entry *entries, int n)
{
    int i = 0;
    while (1) {
        if (strcmp(entries[i].fmt_str, "\"%.*s\"") == 0) {
            fprintf(csv_files[file], entries[i].fmt_str, entries[i].len, entries[i].data);
        } else {
            fprintf(csv_files[file], entries[i].fmt_str, entries[i].data);
        }
        i++;
        if (i == n)
            break;
        fprintf(csv_files[file], ",");
    }
    fprintf(csv_files[file], "\n");
}

/* pread() is a tricky bastard and sometimes doesn't read everything in you ask
 * for all at once. this function guarantees that you get all of the data that
 * you asked for (and rightfully deserve).
 *
 * returns: total number of bytes read, kills program (with message) if pread
 * throws an error. 
 *
 * TODO: aligned reads? -- EDIT: don't do this. O_DIRECT is annoying as hell,
 * this works. check out the link though, it's interesting; why would ext2fslib
 * do it this way...
 *
 * https://fossies.org/dox/e2fsprogs-1.42.13/unix__io_8c_source.html#l00119
 * an example taken directly from the ext2fs library
 */
static ssize_t pread_all(int imgfd, void *buf, size_t count, off_t offset)
{
    size_t bytes_read = 0;
    while (bytes_read < count) {
         ssize_t new_bytes = pread(imgfd, buf, count, offset);
         if (new_bytes < 0) {
             perror("pread");
             exit(1);
         }
         bytes_read += new_bytes;  // update the total bytes read in
         buf += new_bytes;         // update the write location
    }
    return bytes_read;
}

/* Parse the superblock 1024 bytes in, write information to super.csv
 * Also, write the superblock global variable */
void superblock_stat()
{
    // read in the superblock
    pread_all(imgfd, &superblock, SUPERBLOCK_SIZE, SUPERBLOCK_OFFSET);

    // format entries and print
    struct fmt_entry superblock_info[SUPER_FIELDS] = {
        {"%x", superblock.s_magic},
        {"%u", superblock.s_inodes_count},
        {"%u", superblock.s_blocks_count},
        {"%u", EXT2_BLOCK_SIZE(superblock)},
        {"%u", EXT2_FRAG_SIZE(superblock)},
        {"%u", superblock.s_blocks_per_group},
        {"%u", superblock.s_inodes_per_group},
        {"%u", superblock.s_frags_per_group},
        {"%u", superblock.s_first_data_block},
    };
    write_csv(SUPER_CSV, superblock_info, SUPER_FIELDS);
}

/*
 * Functions dedicated to traversing indirect blocks of the file system and
 * outputting indirect.csv
 */
void single_indirect_stat(uint32_t blockptr, int firstLevel)
{
    if ((blockptr == 0) && (firstLevel == 0))
        return;
    uint64_t blocksize = EXT2_BLOCK_SIZE(superblock);
    uint32_t *datablock = malloc(blocksize); // array of blockpointers
    uint64_t n_blocks = blocksize / sizeof(blockptr);
    pread_all(imgfd, datablock, blocksize, blockptr * blocksize);
    
    uint64_t n_blocks_read = 0;
    while (n_blocks_read < n_blocks) {
        if (datablock[n_blocks_read] != 0) { // we'll print out its values
	    struct fmt_entry indirect_info[INDIRECT_FIELDS] = {
	        {"%x", blockptr},
	        {"%u", n_blocks_read},
	        {"%x", datablock[n_blocks_read]}  
	    };
	    write_csv(INDIRECT_CSV, indirect_info, INDIRECT_FIELDS);
	}
	n_blocks_read++;	
    }    
    free(datablock);
}

void double_indirect_stat(uint32_t blockptr, int firstLevel)
{    
    if ((blockptr == 0) && (firstLevel == 0))
        return;
    uint64_t blocksize = EXT2_BLOCK_SIZE(superblock);
    uint32_t *datablock = malloc(blocksize); // array of blockpointers
    uint64_t n_blocks = blocksize / sizeof(blockptr); 
    pread_all(imgfd, datablock, blocksize, blockptr * blocksize);

    uint64_t n_blocks_read = 0;
    while (n_blocks_read < n_blocks) {	
        if (datablock[n_blocks_read] != 0) { // we'll print out its values
	    struct fmt_entry indirect_info[INDIRECT_FIELDS] = {
                {"%x", blockptr},
                {"%u", n_blocks_read},
                {"%x", datablock[n_blocks_read]}
            };
            write_csv(INDIRECT_CSV, indirect_info, INDIRECT_FIELDS);

            single_indirect_stat(datablock[n_blocks_read], 0);
        }
        n_blocks_read++;
    }    
    free(datablock);
}

void triple_indirect_stat(uint32_t blockptr, int firstLevel)
{
    if ((blockptr == 0) && (firstLevel == 0))
        return;
    uint64_t blocksize = EXT2_BLOCK_SIZE(superblock);
    uint32_t *datablock = malloc(blocksize); // array of blockpointers
    uint64_t n_blocks = blocksize / sizeof(blockptr); 
    pread_all(imgfd, datablock, blocksize, blockptr * blocksize);

    uint64_t n_blocks_read = 0;
    while (n_blocks_read < n_blocks) {
        if (datablock[n_blocks_read] != 0) { // we'll print out its values
            struct fmt_entry indirect_info[INDIRECT_FIELDS] = {
                {"%x", blockptr},
                {"%u", n_blocks_read},
                {"%x", datablock[n_blocks_read]}
            };
            write_csv(INDIRECT_CSV, indirect_info, INDIRECT_FIELDS);

	    double_indirect_stat(datablock[n_blocks_read], 0);
        }
        n_blocks_read++;
    }    
    free(datablock);
}
 
/*
 * Print out some information on the given directory inode.
 */
void directory_stat(uint32_t blockptr, uint64_t dir_inode, uint64_t *dir_index)
{
    if (blockptr == 0)
        return;
    uint64_t blocksize = EXT2_BLOCK_SIZE(superblock);
    uint64_t entry_off = 0;
    void *datablock = malloc(blocksize);
    // traverse the direct block

	pread_all(imgfd, datablock, blocksize, blockptr * blocksize);

	while (entry_off < blocksize) {
	    struct ext2_dir_entry *entry = datablock + entry_off;
        uint32_t name_len = entry->name_len & EXT2_DIRNAME_MASK(superblock);

	    if (entry->inode != 0) {
            struct fmt_entry directoryentry_info[DIRECTORY_FIELDS] = {
                {"%u", dir_inode},
                {"%u", *dir_index},
                {"%u", entry->rec_len},
                {"%u", name_len},
                {"%u", entry->inode},
                {"\"%.*s\"", (uint64_t)entry->name, name_len}
            };
            write_csv(DIRECTORY_CSV, directoryentry_info, DIRECTORY_FIELDS);
	    }

	    entry_off += entry->rec_len;
	    *dir_index = *dir_index + 1;
	}
    
    free(datablock);
}

void single_indirect_directory(uint32_t blockptr, int dir_inode,
                               uint64_t *dir_index)
{
    if (blockptr == 0)
        return;
    uint64_t blocksize = EXT2_BLOCK_SIZE(superblock);
    uint32_t *datablock = malloc(blocksize); // array of blockpointers
    uint64_t n_blocks = blocksize / sizeof(blockptr);
    pread_all(imgfd, datablock, blocksize, blockptr * blocksize);

    uint64_t n_blocks_read = 0;
    while (n_blocks_read < n_blocks) {
        if (datablock[n_blocks_read] != 0) { 
            directory_stat(datablock[n_blocks_read], dir_inode, dir_index);     
        }
        n_blocks_read++;
    }
    free(datablock);
}

void double_indirect_directory(uint32_t blockptr, int dir_inode,
                               uint64_t *dir_index)
{
    if (blockptr == 0)
        return;
    uint64_t blocksize = EXT2_BLOCK_SIZE(superblock);
    uint32_t *datablock = malloc(blocksize); // array of blockpointers
    uint64_t n_blocks = blocksize / sizeof(blockptr);
    pread_all(imgfd, datablock, blocksize, blockptr * blocksize);

    uint64_t n_blocks_read = 0;
    while (n_blocks_read < n_blocks) {
        if (datablock[n_blocks_read] != 0) {
            single_indirect_directory(datablock[n_blocks_read], dir_inode, dir_index);
        }
        n_blocks_read++;
    }
    free(datablock);
}

void triple_indirect_directory(uint32_t blockptr, int dir_inode,
                               uint64_t *dir_index)
{
    if (blockptr == 0)
        return;
    uint64_t blocksize = EXT2_BLOCK_SIZE(superblock);
    uint32_t *datablock = malloc(blocksize); // array of blockpointers
    uint64_t n_blocks = blocksize / sizeof(blockptr);
    pread_all(imgfd, datablock, blocksize, blockptr * blocksize);

    uint64_t n_blocks_read = 0;
    while (n_blocks_read < n_blocks) {
        if (datablock[n_blocks_read] != 0) {
            double_indirect_directory(datablock[n_blocks_read], dir_inode, dir_index);
        }
        n_blocks_read++;
    }
    free(datablock);
}
 
/* iterates through all nonempty inodes in a group descriptor
 * NOTE: does another bitmap traversal because we don't actually want to go
 * through each bit in the bitmap like bitmap_stat() does. We only want to
 * check inodes that actually exist according to s_inodes_count and we want to
 * use the correct index into the group's local inode table.
 */

/* Takes an inode table block and a (nonempty) global inode number, and a local
 * index (relative to the block group) to examine */
void inode_stat(uint64_t itable_block, uint64_t inode_nr,
                uint64_t local_inode_index)
{
    uint32_t blocksize = EXT2_BLOCK_SIZE(superblock);
    uint64_t itable_off = itable_block * blocksize;
    uint32_t inode_size = EXT2_INODE_SIZE(superblock);
    uint64_t inode_off = itable_off + local_inode_index * inode_size;

    // find the inode table
    // (be careful to use sizeof(struct inode) and not inode_size)
    pread_all(imgfd, &inode, sizeof(struct ext2_inode), inode_off);

    char filetype = '?';
    switch (inode.i_mode & 0xF000) {
        case EXT2_S_IFREG:
            filetype = 'f';
            break;
        case EXT2_S_IFDIR:
            filetype = 'd';
            break;
        case EXT2_S_IFLNK:
            filetype = 's';
            break;
    }

    uint64_t size = inode.i_size;
    if (superblock.s_rev_level >= 1) {
        uint64_t size_upper = (uint64_t)inode.i_size_high;
        uint64_t size_lower = (uint64_t)inode.i_size;
        size = (size_upper << 32) | size_lower;
    }

    struct fmt_entry inode_info[INODE_FIELDS] = {
        {"%d", inode_nr},
        {"%c", filetype},
        {"%o", inode.i_mode},
        {"%d", inode.i_uid}, // TODO uid and gid are only the bottom 16 bits or something, fix this
        {"%d", inode.i_gid},
        {"%d", inode.i_links_count},
        {"%x", inode.i_ctime},
        {"%x", inode.i_mtime},
        {"%x", inode.i_atime},
        {"%d", size},
        {"%d", EXT2_FS_BLOCKS(inode, superblock)}
    };

    // block pointers (15)
    for (int i = 0; i < EXT2_N_BLOCKS; i++) { 
        inode_info[i+11].fmt_str = "%x";
        inode_info[i+11].data = inode.i_block[i];
    }
    write_csv(INODE_CSV, inode_info, INODE_FIELDS);
    
    // process directories and indirect blocks for directory.csv and indirect.csv
    uint64_t *dir_index = malloc(sizeof(uint64_t));
    *dir_index = 0; 
    for (uint64_t i = 0; i < EXT2_FS_BLOCKS(inode, superblock); i++) {
        if (filetype == 'd') { 
            if (i < 12)
                directory_stat(inode.i_block[i], inode_nr, dir_index);
            else if (i == 12)
                single_indirect_directory(inode.i_block[i], inode_nr, dir_index);
            else if (i == 13)
                double_indirect_directory(inode.i_block[i], inode_nr, dir_index);
            else if (i == 14)
                triple_indirect_directory(inode.i_block[i], inode_nr, dir_index);

        } if (i == 12) { // first indirect block
            single_indirect_stat(inode.i_block[i], 1);

        } else if (i == 13) {
                //printf("%x\n", inode.i_block[i]); 
            double_indirect_stat(inode.i_block[i], 1);

        } else if (i == 14) {
            triple_indirect_stat(inode.i_block[i], 1);
        }
    } 
    free(dir_index);
}

/* Writes out information about a block or inode bitmap. Called from within
 * groupdesc_stat() since it iterates through all blockgroup descriptors.
 *
 * bitmap_block - block number of the block containing the bitmap
 * bitmap_size - how many elements the bitmap contains
 * element_off - how many elements have come before this bitmap (used for
 *               calculating element indices)
 * bitmap_type - one of INODE_BITMAP or BLOCK_BITMAP
 */
void bitmap_stat(uint64_t bitmap_block_index, uint64_t bitmap_size,
                 uint64_t element_off, int bitmap_type)
{
    uint32_t blocksize = EXT2_BLOCK_SIZE(superblock);

    // read in the bitmap
    uint8_t *bitmap = malloc(blocksize);
    pread_all(imgfd, bitmap, blocksize, bitmap_block_index * blocksize);

    uint64_t element_count = 0;
    // iterate over the bytes in the bitmap
    // remember: ext2 stores bitmaps in little endian
    for (uint32_t byte_nr = 0; byte_nr < blocksize; byte_nr++) {
        uint8_t octet = bitmap[byte_nr];

        // iterate over the bits in a byte
        for (uint8_t bit_nr = 0; bit_nr < 8; bit_nr++) {
            if (element_count == bitmap_size) {
                goto cleanup;
            }

            uint32_t element_index = element_off + element_count;

            if (!(octet & 0x01)) {  // empty bitmap entry
                struct fmt_entry bitmap_info[BITMAP_FIELDS] = {
                    {"%x", bitmap_block_index},
                    {"%u", element_index}
                };
                write_csv(BITMAP_CSV, bitmap_info, BITMAP_FIELDS);

            } else if (bitmap_type == INODE_BITMAP) {
                // process the non-empty inode
                inode_stat(group.bg_inode_table, element_index, element_count);
            }
            octet >>= 1;
            element_count++;
        }
    }

cleanup:
    free(bitmap);
}

/* Make sure to call superblock_stat to initialize the superblock global before
 * calling this function.
 */
void groupdesc_stat()
{
    // get some information from the superblock
    uint32_t blocksize = EXT2_BLOCK_SIZE(superblock);

    // group descriptor is in the block immediately following the superblock
    uint64_t groupdesc_off = (superblock.s_first_data_block + 1) * blocksize;
    uint64_t n_block_groups = EXT2_BLOCKGROUPS(superblock);


    for (uint64_t i = 0; i < n_block_groups; i++) {
        pread_all(imgfd, &group, sizeof(group), groupdesc_off);

        // blocks and inodes for this particular group
        uint64_t n_blocks_g = superblock.s_blocks_per_group;
        uint64_t n_inodes_g = superblock.s_inodes_per_group;
        if (i == n_block_groups - 1) {
        //    printf("%d\n", EXT2_BLOCK_REMAINDER(superblock));
            n_blocks_g = EXT2_BLOCK_REMAINDER(superblock);
            n_inodes_g = EXT2_INODE_REMAINDER(superblock);
        }

        // format info and write to group.csv
        struct fmt_entry blockgroup_info[GROUP_FIELDS] = {
            {"%u", n_blocks_g},
            {"%u", group.bg_free_blocks_count},
            {"%u", group.bg_free_inodes_count},
            {"%u", group.bg_used_dirs_count},
            {"%x", group.bg_inode_bitmap},
            {"%x", group.bg_block_bitmap},
            {"%x", group.bg_inode_table}
        };
        write_csv(GROUP_CSV, blockgroup_info, GROUP_FIELDS);


        /* BITMAP */

        // inodes are 1 indexed
        uint64_t ino_off = i * superblock.s_inodes_per_group + 1;

        // add s_first_data_block offset, since the first block group may or
        // may not contain the first (super) block
        uint64_t blk_off = i * superblock.s_blocks_per_group +
                             superblock.s_first_data_block;

        bitmap_stat(group.bg_block_bitmap, n_blocks_g, blk_off, BLOCK_BITMAP);
        bitmap_stat(group.bg_inode_bitmap, n_inodes_g, ino_off, INODE_BITMAP);
        groupdesc_off += sizeof(struct ext2_group_desc);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Please provide a disk image to analyze.\n");
        exit(1);
    }

    // create the output files
    open_csv();

    // open the disk image
    char *filename = argv[1];
    imgfd = open(filename, O_RDONLY);
    if (imgfd == -1)
        perror("opening image");

    /* examine the file system */
    superblock_stat(); //writes the globally declared superblock
    groupdesc_stat(); //also populates bitmap.csv
    close_csv();
}
