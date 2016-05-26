import csv
from optparse import OptionParser

# Constants used to index fields for ease of use
ROOT_DIR = 2

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

class blockObj():
    def __init__(self, bnum, inodenum, entrynum, indirectblock = 0):
        self.blockNumber   = bnum
        self.inodeNumber   = inodenum
        self.entryNumber   = entrynum
        self.indirectBlock = indirectblock
        self.blockPtrs = []

class inodeObj():
    def __init__(self, inum, linkcount = 0):
        self.inodeNumber = inum
        self.linkCount   = linkcount
        self.dirEntries  = []
        self.blockPtrs   = []

class directoryEntry():
    def __init__(self, inodenumber, parentinode, entrynum, entryname = ''):
        self.inodeNumber = inodenumber
        self.parentInode = parentinode
        self.entryNumber = entrynum
        self.entryName   = entryname

# initialize helper data structures
BitmapPointers_FreeInodes = [];
BitmapPointers_FreeBlocks = [];
FreeInodes        = [];
FreeBlocks        = [];

BLOCKS_IN_USE       = dict()
INODES_IN_USE       = dict()
INDIRECT_BLOCKS     = dict()
UNALLOCATED_INODES  = dict()

# global arrays that contain final values to be printed
MISSING_INODES               = []
UNALLOCATED_BLOCKS           = []
INVALID_BLOCK_POINTERS       = []
INCORRECT_LINKCOUNT_INODES   = []
INCORRECT_DIRECTORY_ENTRIES  = []
DUPLICATELY_ALLOCATED_BLOCKS = []

ALL_DIR_ENTRIES = dict()
ALL_BLOCKS      = dict()
ALL_INODES      = dict()

# Super block entries
MagicNumber,InodeCount,BlockCount,BlockSize,FragmentSize,BlocksPerGroup,InodesPerGroup,FragmentsPerGroup,FirstDataBlock = tuple([None]*9)

# Read CSV files into program
superblock       = csv.reader(open('super.csv', 'rb'), delimiter=',', quotechar='"');
group_descriptor = csv.reader(open('group.csv', 'rb'), delimiter=',', quotechar='"');
bitmap           = csv.reader(open('bitmap.csv', 'rb'), delimiter=',');
inode            = csv.reader(open('inode.csv', 'rb'), delimiter=',');
directory        = csv.reader(open('directory.csv', 'rb'), delimiter=',');
indirect_blocks  = csv.reader(open('indirect.csv', 'rb'), delimiter=',');

def initStructs():
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
        # ALL_INODES[free_inode_bitmap_block] = inodeObj(free_inode_bitmap_block, 0, 0)
        ALL_BLOCKS[free_inode_bitmap_block] = blockObj(free_inode_bitmap_block, 0, 0)
        ALL_BLOCKS[free_block_bitmap_block] = blockObj(free_block_bitmap_block, 0, 0)

    def __init__(self, bnum, inodenum, entrynum, indirectblock = 0):
        self.blockNumber = bnum
        self.inodeNumber = inodenum
        self.entryNumber = entrynum
        self.indirectBlock = indirectblock

    # parse free_bitmap_entry data : list of free inodes and free blocks
    for line in bitmap:
        MapBlock_Number       = int(line[0], 16);
        Block_or_Inode_Number = int(line[1]);
        if   MapBlock_Number in BitmapPointers_FreeInodes: FreeInodes.append(Block_or_Inode_Number)
        elif MapBlock_Number in BitmapPointers_FreeBlocks: FreeBlocks.append(Block_or_Inode_Number)
        elif (verbose == True): print "MapBlock_Number: %s is not present in either bitmap file!!" % MapBlock_Number

    if MagicNumber != 0xef53 : print 'This doesnt appear to be an EXT2 Filesytem. No guarantees from this point on...'
    # UNALLOCATED BLOCK : Blocks that are in use but also listed on the free bitmap.
    #                     Here the INODEs should be listed in increasing order of inode_num.


    # UNALLOCATED_DIRECTORY :


def handleInodesInUse():
    # Parse Inodes into INODES_IN_USE
    for line in inode:
        inodenum    = line[i_num]
        linkcount   = line[i_linkcount]
        iobj = inodeObj(inodenum, linkcount)
        blkptrs = []
        for i in range(15) : blkptrs.append(int(line[i_firstblockpointer], 16) + i)
        iobj.blockPtrs = blkptrs
        INODES_IN_USE[inodenum] = iobj


def handleDirectories():
    for line in directory :
        this_DirEntry  = directoryEntry(int(line[dir_fileinode]), int(line[dir_parentinode]), int(line[dir_entrynum]), line[dir_name])
        #if this_DirEntry.parentInode == 2:
        # Add this_DirEntry to the ALL_DIR_ENTRIES dictionary
        if this_DirEntry.inodeNumber != this_DirEntry.parentInode or this_DirEntry.parentInode == ROOT_DIR:
            ALL_DIR_ENTRIES[this_DirEntry.inodeNumber] = this_DirEntry

        if this_DirEntry.inodeNumber in INODES_IN_USE : INODES_IN_USE[this_DirEntry.inodeNumber].dirEntries.append(this_DirEntry)
        elif this_DirEntry.inodeNumber in UNALLOCATED_INODES :
            UNALLOCATED_INODES[this_DirEntry.inodeNumber].append(this_DirEntry)
        else:
            UNALLOCATED_INODES[this_DirEntry.inodeNumber] = [this_DirEntry]


    # for (inum, entry) in ALL_DIR_ENTRIES.iteritems():
        inum = this_DirEntry.inodeNumber
        entry = this_DirEntry
        if (entry.entryNumber == 0 or entry.entryName == '.') and (entry.inodeNumber != entry.parentInode):
            INCORRECT_DIRECTORY_ENTRIES.append((entry, entry.parentInode))
            #                                 DirEntry, Should link to value
        elif (entry.entryNumber == 1 or entry.entryName == '..') and ((entry.inodeNumber != ALL_DIR_ENTRIES[entry.parentInode].parentInode) or (entry.parentInode not in ALL_DIR_ENTRIES)):
            INCORRECT_DIRECTORY_ENTRIES.append((entry, ALL_DIR_ENTRIES[entry.parentInode].parentInode))
            #                                 DirEntry, Should link to value

def handleMissingInodes():
    # Finding Missing inodes
    totalinodes = 0
    for line in superblock: totalinodes = line[s_total_inodes]
    MISSING_INODES = [i for i in range(totalinodes) if i not in ALL_DIR_ENTRIES]

def handleIndirectBlocks():
    # parse indirect block entry: These are all the non-zero block pointers in an indirect block.
    #                             The blocks that contain indirect block pointers are included.
    for line in indirect_blocks:
        ContainingBlockNumber             = int(line[0], 16)
        (EntryNumber, BlockPointer_Value) = (int(line[1]), int(line[2], 16))
        if ContainingBlockNumber not in INDIRECT_BLOCKS:
            INDIRECT_BLOCKS[ContainingBlockNumber] = [(EntryNumber, BlockPointer_Value)]
        else:
            INDIRECT_BLOCKS[ContainingBlockNumber].append((EntryNumber, BlockPointer_Value))



# Incorrect Link Count
def write5():
    buff = ""
    for inum, iobj in INODES_IN_USE.iteritems():
        if iobj.linkCount != len(iobj.dirEntries):
            buff += ("LINKCOUNT < " + str(iobj.inodeNumber) + " >")
            buff += (" IS < " + str(iobj.linkCount) + " >")
            buff += (" SHOULD BE < " + str(len(iobj.dirEntries)) + " >")
            buff += "\n"

    if verbose: print(buff)
    # output_file.write(buff)

# Incorrect Directory Entry
def write6():
    buff = ""
    for (entry, shouldbe) in INCORRECT_DIRECTORY_ENTRIES:
        buff += ("INCORRECT ENTRY IN < " + str(entry.parentInode) + " >")
        buff += (" NAME < " + str(entry.entryName) + " >")
        buff += (" LINK TO < " + str(entry.inodeNumber) + " >")
        buff += (" SHOULD BE < " + str(shouldbe) + " >")
        buff += "\n"

    print(buff)

if __name__ == "__main__":
    # Open file to write the resulting output
    output_file = open('lab3b_check.txt', 'w+')

    initStructs()
    handleInodesInUse()
    handleIndirectBlocks()
    handleDirectories()
    handleMissingInodes()


    write5()
    write6()

    for item in UNALLOCATED_BLOCKS : output_file.write("UNALLOCATED BLOCK < " + 1035 + " > REFERENCED BY INODE < " + 16 + " > ENTRY < " + 0 + " > INODE < " + 17 + " > INDIRECT BLOCK < " + 10 + " > ENTRY < " + 0 + " >\n")
