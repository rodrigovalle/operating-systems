#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <assert.h>  //for debug

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
    uint32_t    data;
};

// file descriptor reference to the open disk image we're analyzing
static int imgfd = -1;

/* These #defines are for bitmap_stat() */
#define BLOCK_BITMAP    0
#define INODE_BITMAP    1
#define TOP_BIT(byte)   ((byte) & (0x08))

/* EXT2 DEFINITIONS
 * these definitions taken directly from <ext2fs/ext2_fs.h>
 */
#define SUPERBLOCK_OFFSET   1024
#define SUPERBLOCK_SIZE     1024
#define EXT2_BLOCK_SIZE(x)  ((1024) << (x))

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
};

/*                                                                              
 * Structure of an inode on the disk                                            
 */                                                                             

//struct ext2_inode {                                                             
//    uint16_t   i_mode;          /* File mode */                                         
//    uint16_t   i_uid;           /* Low 16 bits of Owner Uid */                          
//    uint32_t   i_size;          /* Size in bytes */                                     
//    uint32_t   i_atime;         /* Access time */                                       
//    uint32_t   i_ctime;         /* Inode change time */                                 
//    uint32_t   i_mtime;         /* Modification time */                                 
//    uint32_t   i_dtime;         /* Deletion Time */                                     
//    uint16_t   i_gid;           /* Low 16 bits of Group Id */                           
//    uint16_t   i_links_count;   /* Links count */                                   
//    uint32_t   i_blocks;        /* Blocks count */                                      
//    uint32_t   i_flags;         /* File flags */                                        
//    union {                                                                     
//        struct {                                                                
//            uint32_t   l_i_version; /* was l_i_reserved1 */                        
//        } linux1;                                                               
//        struct {                                                                
//            uint32_t  h_i_translator;                                              
//        } hurd1;                                                                
//    } osd1;         /* OS dependent 1 */                                    
//    uint32_t   i_block[EXT2_N_BLOCKS];  /* Pointers to blocks */                     
//    uint32_t   i_generation;            /* File version (for NFS) */                        
//    uint32_t   i_file_acl;              /* File ACL */                                          
//    uint32_t   i_size_high;             /* Formerly i_dir_acl, directory ACL */             
//    uint32_t   i_faddr;                 /* Fragment address */                                  
//    union {                                                                     
//        struct {                                                                
//            uint16_t   l_i_blocks_hi;                                              
//            uint16_t   l_i_file_acl_high;                                          
//            uint16_t   l_i_uid_high;    /* these 2 fields    */                     
//            uint16_t   l_i_gid_high;    /* were reserved2[0] */                     
//            uint16_t   l_i_checksum_lo; /* crc32c(uuid+inum+inode) */              
//            uint16_t   l_i_reserved;                                               
//        } linux2;                                                               
//        struct {                                                                
//            uint8_t    h_i_frag;        /* Fragment number */                           
//            uint8_t    h_i_fsize;       /* Fragment size */                             
//            uint16_t   h_i_mode_high;                                              
//            uint16_t   h_i_uid_high;                                               
//            uint16_t   h_i_gid_high;                                               
//            uint32_t   h_i_author;                                                 
//        } hurd2;                                                                
//    } osd2;             /* OS dependent 2 */                                    
//};

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
 *  - entry and format must both have length n.
 *  - you can only specify one "type" in the format string. eg %d, %x, etc.
 */
static void write_csv(int file, const struct fmt_entry *entries, int n)
{
    int i = 0;
    while (1) {
        fprintf(csv_files[file], entries[i].fmt_str, entries[i].data);
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
 *
 * TODO: maybe turn this into a function for reading in an entire block based
 * on blocksize.
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

/* Parse the superblock at 1024 bytes in, write information to super.csv
 */
void superblock_stat()
{
    ssize_t s;

    // read in the superblock
    s = pread_all(imgfd, &superblock, SUPERBLOCK_SIZE, SUPERBLOCK_OFFSET);
    assert(s == SUPERBLOCK_SIZE);

    // some logic for the fragment size taken from the documentation
    uint32_t frag_size = superblock.s_log_frag_size;

    // TODO: note, this is always true
    if (frag_size > 0)
        frag_size = 1024 << frag_size;
    else
        frag_size = 1024 >> -frag_size;

    // format entries and print
    struct fmt_entry superblock_info[SUPER_FIELDS] = {
        {"%x", superblock.s_magic},
        {"%u", superblock.s_inodes_count},
        {"%u", superblock.s_blocks_count},
        {"%u", EXT2_BLOCK_SIZE(superblock.s_log_block_size)},
        {"%u", frag_size},
        {"%u", superblock.s_blocks_per_group},
        {"%u", superblock.s_inodes_per_group},
        {"%u", superblock.s_frags_per_group},
        {"%u", superblock.s_first_data_block},
    };
    write_csv(SUPER_CSV, superblock_info, SUPER_FIELDS);
}

/* Writes out information about a block or inode bitmap. Called from within
 * groupdesc_stat() since it iterates through all blockgroup descriptors.
 */
void bitmap_stat(uint32_t bitmap_id, uint32_t cur_block_group, int bitmap_type)
{
    uint32_t blocksize = EXT2_BLOCK_SIZE(superblock.s_log_block_size);
    uint32_t thingies_per_group = bitmap_type ? superblock.s_inodes_per_group : superblock.s_blocks_per_group;
    uint64_t bitmap_off = blocksize * bitmap_id;

    uint8_t *bitmap = malloc(blocksize);
    if (bitmap == NULL) {
        perror("malloc failed");
        exit(1);
    }

    pread_all(imgfd, bitmap, blocksize, bitmap_off); 
    for (uint32_t byte_i = 0; byte_i < blocksize; byte_i++)
    {
        uint8_t byte = bitmap[byte_i];

        for (int bit_i = 0; bit_i < 8; bit_i++) {
            if (!(byte & 0x01)) {
                uint32_t free_block_nr = (byte_i * 8 + bit_i + superblock.s_first_data_block) + cur_block_group * thingies_per_group;
                struct fmt_entry bitmap_info[BITMAP_FIELDS] = {
                    {"%x", bitmap_id},
                    {"%u", free_block_nr}
                };
                write_csv(BITMAP_CSV, bitmap_info, BITMAP_FIELDS);
            }
            /*else
            {
                inodelist<- node // put the bit into the inodelist
            }*/
            byte >>= 1;
        }
    }
}

uint32_t get_inode_number(uint32_t cur_block_group)
{
    return (cur_block_group * superblock.s_inodes_per_group);
}

/* Make sure to call superblock_stat to initialize the superblock global before
 * calling this function.
 */
void groupdesc_stat()
{
    // get some information from the superblock
    uint32_t total_n_blocks = superblock.s_blocks_count;
    uint32_t blocks_per_group = superblock.s_blocks_per_group;
    uint64_t blocksize = EXT2_BLOCK_SIZE(superblock.s_log_block_size);

    // group descriptor is in the block immediately following the superblock
    uint64_t groupdesc_off = SUPERBLOCK_OFFSET + blocksize;

    // this is the number of block groups (+1 to include the last block group
    // which might not be full)
    uint32_t n_block_groups = total_n_blocks / blocks_per_group + 1;

    struct ext2_group_desc blockgroup;
    for (uint32_t i = 0; i < n_block_groups; i++) {
        pread_all(imgfd, &blockgroup, sizeof(blockgroup), groupdesc_off);
        uint32_t n_blocks_for_this_group = superblock.s_blocks_per_group;

        if (i == n_block_groups - 1) {
            n_blocks_for_this_group = total_n_blocks % blocks_per_group;
        }

        // format info and write to group.csv
        struct fmt_entry blockgroup_info[GROUP_FIELDS] = {
            {"%u", n_blocks_for_this_group},
            {"%u", blockgroup.bg_free_blocks_count},
            {"%u", blockgroup.bg_free_inodes_count},
            {"%u", blockgroup.bg_used_dirs_count},
            {"%x", blockgroup.bg_inode_bitmap},
            {"%x", blockgroup.bg_block_bitmap},
            {"%x", blockgroup.bg_inode_table}
        };
        write_csv(GROUP_CSV, blockgroup_info, GROUP_FIELDS);

        // write bitmap info for this blockgroup
        bitmap_stat(blockgroup.bg_block_bitmap, i, 0);
        // bitmanp_stat(blockgroup.bg_inode_bitmap, i, 1);
        groupdesc_off += sizeof(blockgroup);
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

    superblock_stat();
    groupdesc_stat(); //also populates bitmap.csv
    close_csv();
}
