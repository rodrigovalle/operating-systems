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

enum outfile {  /* we use this enum to index the filenames array */
    SUPER_CSV = 0,
    GROUP_CSV,
    BITMAP_CSV,
    INODE_CSV,
    DIRECTORY_CSV,
    INDIRECT_CSV,
    N_FILES
};

FILE *csv_files[N_FILES];      /* our six output files */
char *filenames[N_FILES] = {   /* their names */
    "super.csv",
    "group.csv",
    "bitmap.csv",
    "inode.csv",
    "directory.csv",
    "indirect.csv",
};

enum output_fields { /* the number of fields each csv contains */
    SUPER_FIELDS = 9,
    GROUP_FIELDS = 7,
    BITMAP_FIELDS = 2,
    INODE_FIELDS = 26,
    DIRECTORY_FIELDS = 6,
    INDIRECT_FIELDS = 3
};

struct formatted_entry {
    char *format_str;
    uint32_t data;
};

// these definitions taken directly from <ext2fs/ext2_fs.h>
#define SUPERBLOCK_OFFSET 1024  // bytes from beginning of volume to superblock
#define SUPERBLOCK_SIZE   1024

/*
 * Structure of the superblock (compatible with all ext2 versions)
 * NOTE: don't allocate this on the stack, it's too big and will cause a
 * stack overflow/segfault. Use malloc instead.
 */
struct ext2_super_block {
    uint32_t   s_inodes_count;         /* Inodes count */
    uint32_t   s_blocks_count;         /* Blocks count */
    uint32_t   s_r_blocks_count;       /* Reserved blocks count */
    uint32_t   s_free_blocks_count;    /* Free blocks count */
    uint32_t   s_free_inodes_count;    /* Free inodes count */
    uint32_t   s_first_data_block;     /* First data block */
    uint32_t   s_log_block_size;       /* Block size*/
    uint32_t   s_log_cluster_size;     /* Allocation cluster/fragment size */
    uint32_t   s_blocks_per_group;     /* Blocks per group */
    uint32_t   s_clusters_per_group;   /* # Fragments per group */
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
};
/* TODO: might declare a global superblock if we need it after we write 
 * superblock.csv
 */

/* Populates the csv_files array with references to appropriately named and
 * newly created output files.
 */
static void open_csv() {
    for (int i = 0; i < 6; i++) {
        csv_files[i] = fopen(filenames[i], "w");
        if (csv_files[i] == NULL)
            perror("create csv file");
    }
}

/* Writes an array of entries out to the corresponding csv file in the
 * appropriate format. Make sure to call open_files() first.
 * NOTES:
 *  - entry and format must both have length n.
 *  - you can only specify one "type" in the format string. eg %d, %x, etc.
 */
static void write_csv(int file, const struct formatted_entry entry[], int n)
{
    int i = 0;
    while (1) {
        fprintf(csv_files[file], entry[i].format_str, entry[i].data);
        i++;
        if (i == n)
            break;
        fprintf(csv_files[file], ",");
    }
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

void superblock(int imgfd)
{
    ssize_t s;
    struct ext2_super_block *superblock;

    // read in the superblock
    superblock = malloc(sizeof(struct ext2_super_block));
    if (superblock == NULL)
        perror("malloc");
    s = pread_all(imgfd, superblock, SUPERBLOCK_SIZE, SUPERBLOCK_OFFSET);
    assert(s == SUPERBLOCK_SIZE);

    struct formatted_entry superblock_csv[] = {
        {"%x", superblock->s_magic},
        {"%u", superblock->s_inodes_count},
        {"%u", superblock->s_blocks_count},
        {"%u", superblock->s_log_block_size},
        {"%u", superblock->s_log_cluster_size},
        {"%u", superblock->s_blocks_per_group},
        {"%u", superblock->s_inodes_per_group},
        {"%u", superblock->s_clusters_per_group},
        {"%u", superblock->s_first_data_block},
    };

    write_csv(imgfd, superblock_csv, SUPER_FIELDS);
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
    int imgfd = open(filename, O_RDONLY);
    if (imgfd == -1)
        perror("opening image");

    superblock(imgfd);
}
