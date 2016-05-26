import csv
from optparse import OptionParser

# Constants used to index fields for ease of use

# Super block
s_magic, s_total_inodes, s_total_blocks, s_blocksize, s_fragsize, s_blocks_per_group, s_inodes_per_group, s_frags_per_group, s_first_data_block = tuple(range(9))

# Block Group Descriptors
bg_total_blocks, bg_free_blocks, bg_free_indoes, bg_total_dirs, bg_inode_bitmap_block, bg_block_bitmap_block, bg_inode_start_block = tuple(range(7))

# Free Bitmap entry
bm_map_blocknum, bm_num = tuple(range(2))

# Inode
i_num, i_filetype, i_mode, i_owner, i_group, i_linkcount, i_createtime, i_modtime, i_accesstime, i_filesize, i_numblocks, i_firstblockpointer = tuple(range(12))

# Directory entry
dir_parentinode, dir_entrynum, dir_entrylen, dir_namelen, dir_fileinode, dir_name = tuple(range(6))

# Indirect block entry
ind_containingblock, ind_entrynum, ind_blockpointerval = tuple(range(3))

# End constants

# Read verbose option, if any
parser          = OptionParser()
parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False)
(options, args) = parser.parse_args()
verbose         = options.verbose

class block():
    def __init__(self, number):
        self.number = number;
        self.referenced_by = []

class inode():
    def __init__(self, number):
        self.number = number;
        self.referenced_by = []

# initialize helper data structures
BitmapPointers_FreeInodes = [];
BitmapPointers_FreeBlocks = [];
FreeInodes        = [];
FreeBlocks        = [];

BLOCKS_IN_USE     = dict()
INODES_IN_USE     = dict()

# global arrays that contain final values to be printed
MISSING_INODES               = []
UNALLOCATED_INODES           = []
UNALLOCATED_BLOCKS           = []
INVALID_BLOCK_POINTERS       = []
INCORRECT_LINKCOUNT_INODES   = []
INCORRECT_DIRECTORY_ENTRIES  = []
DUPLICATELY_ALLOCATED_BLOCKS = []

# Read CSV files into program
superblock       = csv.reader(open('super.csv', 'rb'), delimiter=',', quotechar='"');
group_descriptor = csv.reader(open('group.csv', 'rb'), delimiter=',', quotechar='"');
bitmap           = csv.reader(open('bitmap.csv', 'rb'), delimiter=',');
inode            = csv.reader(open('inode.csv', 'rb'), delimiter=',');
directory        = csv.reader(open('directory.csv', 'rb'), delimiter=',');
indirect_blocks  = csv.reader(open('indirect.csv', 'rb'), delimiter=',');

# parse superblock data
for line in superblock:
    MagicNumber       = int(line[0], 16);
    InodeCount        = int(line[1]);
    BlockCount        = int(line[2]);
    BlockSize         = int(line[3]);
    FragmentSize      = int(line[4]);
    BlocksPerGroup    = int(line[5]);
    InodesPerGroup    = int(line[6]);
    FragmentsPerGroup = int(line[7]);
    FirstDataBlock    = int(line[8]);

# parse group descriptor data : information for each group descriptor
for line in group_descriptor:
    free_inode_bitmap_block = int(line[4], 16)
    free_block_bitmap_block = int(line[5], 16)
    BitmapPointers_FreeInodes.append(free_inode_bitmap_block)
    BitmapPointers_FreeBlocks.append(free_block_bitmap_block)


# parse free_bitmap_entry data : list of free inodes and free blocks
for line in bitmap:
    MapBlock_Number       = int(line[0], 16);
    Block_or_Inode_Number = int(line[1]);
    if   MapBlock_Number in BITMAP_FreeInodes: FreeInodes.append(Block_or_Inode_Number)
    elif MapBlock_Number in BITMAP_FreeBlocks: FreeBlocks.append(Block_or_Inode_Number)
    elif (verbose == True): print "MapBlock_Number: %s is not present in either bitmap file!!" % MapBlock_Number

if MagicNumber != 0xef53 : print 'This doesnt appear to be an EXT2 Filesytem. No guarantees from this point on...'



# UNALLOCATED BLOCK : Blocks that are in use but also listed on the free bitmap.
#                     Here the INODEs should be listed in increasing order of inode_num.


# UNALLOCATED_DIRECTORY :





# Missing iNode :: inodes that are not in use, and not listed on the free bitmap





# Open file to write the resulting output
output_file = open('lab3b_check.txt', 'w+')

for item in UNALLOCATED_BLOCKS : output_file.write("UNALLOCATED BLOCK < " + 1035 + " > REFERENCED BY INODE < " + 16 + " > ENTRY < " + 0 + " > INODE < " + 17 + " > INDIRECT BLOCK < " + 10 + " > ENTRY < " + 0 + " >\n")


# if verbose == True:
#     for line in superblock: print line
#     for line in group_descriptor: print line
#     for line in bitmap: print line
#     for line in inode: print line
#     for line in directory: print line
#     for line in indirect_blocks: print line
#     # for line in superblock: fout.write(str(line))
#     # for line in group_descriptor: fout.write(str(line))
#     # for line in bitmap: fout.write(str(line))
#     # for line in inode: fout.write(str(line))
#     # for line in directory: fout.write(str(line))
#     # for line in indirect_blocks: fout.write(str(line))
