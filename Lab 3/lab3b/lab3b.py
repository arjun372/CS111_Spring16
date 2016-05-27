import csv
from optparse import OptionParser

ROOT_DIR = 2

# Read verbose option, if any
parser          = OptionParser()
parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False)
(options, args) = parser.parse_args()
verbose         = options.verbose

class blockObj():
    def __init__(self, bnum, inodenum, entrynum, indirectblock = 0):
        self.blockNumber    = bnum
        self.inodeNumber    = inodenum
        self.entryNumber    = entrynum
        self.indirectBlock  = indirectblock
        self.referencePtrs  = []

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

# global arrays that contain final values to be printed
BitmapPointers_FreeInodes    = []
BitmapPointers_FreeBlocks    = []
FreeInodes                   = []
FreeBlocks                   = []
INVALID_BLOCK_POINTERS       = []
INCORRECT_DIRECTORY_ENTRIES  = []

BLOCKS_IN_USE       = dict()
INODES_IN_USE       = dict()
INDIRECT_BLOCKS     = dict()
UNALLOCATED_INODES  = dict()
DIRS_IN_USE         = dict()

# Read CSV files into program
superblock       = csv.reader(open('super.csv', 'rb'), delimiter=',', quotechar='"');
group_descriptor = csv.reader(open('group.csv', 'rb'), delimiter=',', quotechar='"');
bitmap           = csv.reader(open('bitmap.csv', 'rb'), delimiter=',');
inode            = csv.reader(open('inode.csv', 'rb'), delimiter=',');
directory        = csv.reader(open('directory.csv', 'rb'), delimiter=',');
indirect_blocks  = csv.reader(open('indirect.csv', 'rb'), delimiter=',');
output_file      = open('lab3b_check.txt', 'w+')

def initStructs():
    # parse superblock data
    for line in superblock:
        global MagicNumber
        global InodeCount
        global BlockCount
        global BlockSize
        global FragmentSize
        global BlocksPerGroup
        global InodesPerGroup
        global FragmentsPerGroup
        global FirstDataBlock
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
        global BLOCKS_IN_USE
        global BitmapPointers_FreeInodes
        global BitmapPointers_FreeBlocks
        free_inode_bitmap_block = int(line[4], 16)
        free_block_bitmap_block = int(line[5], 16)
        BitmapPointers_FreeInodes.append(free_inode_bitmap_block)
        BitmapPointers_FreeBlocks.append(free_block_bitmap_block)
        BLOCKS_IN_USE[free_inode_bitmap_block] = blockObj(free_inode_bitmap_block, 0, 0)
        BLOCKS_IN_USE[free_block_bitmap_block] = blockObj(free_block_bitmap_block, 0, 0)

    # parse free_bitmap_entry data : list of free inodes and free blocks
    for line in bitmap:
        global FreeInodes
        global FreeBlocks
        Block_or_Inode_Number = int(line[1]);
        MapBlock_Number       = int(line[0], 16);
        if   MapBlock_Number in BitmapPointers_FreeBlocks: FreeBlocks.append(Block_or_Inode_Number)
        elif MapBlock_Number in BitmapPointers_FreeInodes: FreeInodes.append(Block_or_Inode_Number)
        elif (verbose == True): print "MapBlock_Number: %s is not present in either bitmap file!!" % MapBlock_Number
    if MagicNumber != 0xef53 : print 'This doesnt appear to be an EXT2 Filesytem. No guarantees from this point on...'
    return

def appendReference(blockNum, inodeNum, entry, indirectBlockNum):
    global BlockCount
    global BLOCKS_IN_USE
    global INVALID_BLOCK_POINTERS
    if blockNum == 0 or blockNum > BlockCount : INVALID_BLOCK_POINTERS.append((blockNum, inodeNum, indirectBlockNum, entry))
    else:
        if blockNum not in BLOCKS_IN_USE: BLOCKS_IN_USE[blockNum] = blockObj(blockNum, inodeNum, entry, indirectBlockNum)
        BLOCKS_IN_USE[blockNum].referencePtrs.append((inodeNum, indirectBlockNum, entry))
    return

def getNumBlocks_Single(blockNum, inodeNum, indirectBlockNum, entry):
    global INDIRECT_BLOCKS
    count = 1
    appendReference(blockNum, inodeNum, entry, indirectBlockNum)
    if blockNum in INDIRECT_BLOCKS:
      for item in INDIRECT_BLOCKS[blockNum]:
        count += 1
        appendReference(item[1], inodeNum, item[0], blockNum)
    return count

def getNumBlocks_Double(blockNum, inodeNum, indirectBlockNum, entry):
    global INDIRECT_BLOCKS
    count = 1
    appendReference(blockNum, inodeNum, entry, indirectBlockNum)
    if blockNum in INDIRECT_BLOCKS:
      for item in INDIRECT_BLOCKS[blockNum]:
        count += getNumBlocks_Single(item[1], inodeNum, blockNum, item[0])
    return count

def getNumBlocks_Triple(blockNum, inodeNum, indirectBlockNum, entry):
    global INDIRECT_BLOCKS
    count = 1
    appendReference(blockNum, inodeNum, entry, indirectBlockNum)
    if blockNum in INDIRECT_BLOCKS:
      for item in INDIRECT_BLOCKS[blockNum]:
        count += getNumBlocks_Double(item[1], inodeNum, blockNum, item[0])
    return count

# Parse Inodes into INODES_IN_USE
def handleInodesInUse():
    global BlockCount
    global INODES_IN_USE
    global INVALID_BLOCK_POINTERS
    for line in inode:
        blkptrs                 = []
        inodenum                = int(line[0])
        linkcount               = int(line[5])
        numBlocks               = int(line[10])
        pendingIndirectBlocks   = numBlocks - 12
        INODES_IN_USE[inodenum] = inodeObj(inodenum, linkcount)

        # While iterating thru all the blockPtrs, check to see if they are valid and referenced correctly :
        for ptrEntry in range(11, 11 + min(11+1, numBlocks)) :
            appendReference(int(line[ptrEntry], 16), inodenum, ptrEntry-11, 0)

        # now iterate over the indirect block pointers recursively
        for i in range(1, 4):
            if (pendingIndirectBlocks > 0):
                currentBlock = int(line[i+22], 16) # TODO :: Check if this is supposed to be 0 or not
                if currentBlock == 0 or currentBlock > BlockCount :
                    INVALID_BLOCK_POINTERS.append((currentBlock, inodenum, 0, i+11))
                else:
                    if   i==1: blocksInsideBlock = getNumBlocks_Single(currentBlock, inodenum, 0, i+11)
                    elif i==2: blocksInsideBlock = getNumBlocks_Double(currentBlock, inodenum, 0, i+11)
                    elif i==3: blocksInsideBlock = getNumBlocks_Triple(currentBlock, inodenum, 0, i+11)
                    pendingIndirectBlocks -= blocksInsideBlock
    return

def handleDirectories():
    global INODES_IN_USE
    global DIRS_IN_USE
    global UNALLOCATED_INODES
    global INCORRECT_DIRECTORY_ENTRIES
    for line in directory :
        currEntry = directoryEntry(int(line[4]), int(line[0]), int(line[1]), str(line[5]))
        error_entries = (currEntry.parentInode, currEntry.entryNumber)
        if currEntry.parentInode == ROOT_DIR or currEntry.inodeNumber != currEntry.parentInode:
            DIRS_IN_USE[currEntry.inodeNumber] = currEntry

        if currEntry.inodeNumber in INODES_IN_USE : INODES_IN_USE[currEntry.inodeNumber].dirEntries.append(error_entries)
        elif currEntry.inodeNumber in UNALLOCATED_INODES : UNALLOCATED_INODES[currEntry.inodeNumber].append(error_entries)
        else: UNALLOCATED_INODES[currEntry.inodeNumber] = [error_entries]

        # DirEntry, Should link to value
        correctParentInode = -1
        if (currEntry.entryNumber == 0 or currEntry.entryName == '.') and (currEntry.inodeNumber != currEntry.parentInode):
            correctParentInode = currEntry.parentInode
        elif (currEntry.entryNumber == 1 or currEntry.entryName == '..') and ((currEntry.inodeNumber != DIRS_IN_USE[currEntry.parentInode].parentInode) or (currEntry.parentInode not in DIRS_IN_USE)):
            correctParentInode = DIRS_IN_USE[currEntry.parentInode].parentInode
        if correctParentInode != -1:
            INCORRECT_DIRECTORY_ENTRIES.append((currEntry.parentInode, currEntry.entryName, currEntry.inodeNumber, correctParentInode))
    return

# parse indirect block entry: These are all the non-zero block pointers in an indirect block.
#                             The blocks that contain indirect block pointers are included.
def handleIndirectBlocks():
    global INDIRECT_BLOCKS
    for line in indirect_blocks:
        ContainingBlockNumber = int(line[0], 16)
        EntryNumber           = int(line[1])
        BlockPointer_Value    = int(line[2], 16)
        if ContainingBlockNumber not in INDIRECT_BLOCKS: INDIRECT_BLOCKS[ContainingBlockNumber] = [(EntryNumber, BlockPointer_Value)]
        else: INDIRECT_BLOCKS[ContainingBlockNumber].append((EntryNumber, BlockPointer_Value))
    return

# 1. UNALLOCATED BLOCKS
def write1():
    global FreeBlocks
    global BLOCKS_IN_USE
    for item in sorted(BLOCKS_IN_USE):
        if item in FreeBlocks:
            line = "UNALLOCATED BLOCK < " + str(item) + " > REFERENCED BY "
            for (inodeNum, indirectBlockNum, entryNum) in sorted(BLOCKS_IN_USE[item].referencePtrs):
                if int(indirectBlockNum == 0): line += "INODE < " + str(inodeNum) + " > ENTRY < " + str(entryNum) + " > "
                else: line += "INODE < " + str(inodeNum) + " > INDIRECT BLOCK < " + str(indirectBlockNum) + " > ENTRY < " + str(entryNum) + " > "
            output_file.write(line.strip() + "\n");
    return

# 2. DUPLICATELY ALLOCATED BLOCKS
def write2():
    global BLOCKS_IN_USE
    for item in sorted(BLOCKS_IN_USE):
        if len(BLOCKS_IN_USE[item].referencePtrs) > 1:
            line = "MULTIPLY REFERENCED BLOCK < " + str(item) + " > BY "
            for (inodeNum, indirectBlockNum, entryNum) in sorted(BLOCKS_IN_USE[item].referencePtrs):
                if int(indirectBlockNum == 0): line += "INODE < " + str(inodeNum) + " > ENTRY < " + str(entryNum) + " > "
                else: line += "INODE < " + str(inodeNum) + " > INDIRECT BLOCK < " + str(indirectBlockNum) + " > ENTRY < " + str(entryNum) + " > "
            output_file.write(line.strip() + "\n");
    return

# 3. UNALLOCATED INODE
def write3():
    global FreeInodes
    global INODES_IN_USE
    global UNALLOCATED_INODES
    for item in sorted(UNALLOCATED_INODES):
        line = "UNALLOCATED INODE < " + str(item) + " > REFERENCED BY "
        for entry in sorted(UNALLOCATED_INODES[item]):
            line += "DIRECTORY < " + str(entry[0]) + " > ENTRY < " + str(entry[1]) + " > "
        output_file.write(line.strip() + "\n");
    return

# Incorrect Link Count
def write4and5():
    buff = ""
    global InodeCount
    global FreeInodes
    global INODES_IN_USE
    global InodesPerGroup
    global BitmapPointers_FreeInodes
    global BitmapPointers_FreeBlocks
    for inum in sorted(INODES_IN_USE):
        entry    = INODES_IN_USE[inum]
        dirlinks = len(entry.dirEntries)
        if inum > 10 and dirlinks == 0:
            buff += ("MISSING INODE < " + str(inum) + " > SHOULD BE IN FREE LIST < ")
            buff += str(BitmapPointers_FreeInodes[int(inum)/InodesPerGroup])
            buff += " >\n"
        elif dirlinks != entry.linkCount:
            buff += ("LINKCOUNT < " + str(inum) + " >")
            buff += (" IS < " + str(entry.linkCount) + " >")
            buff += (" SHOULD BE < " + str(dirlinks) + " >")
            buff += "\n"
        if inum in FreeInodes:
            buff += ("UNALLOCATED INODE < " + str(inum) + " >\n")
    if verbose: print(buff)
    output_file.write(buff)
    buff = ""
    for inode in range(11, InodeCount):
        if inode not in INODES_IN_USE and inode not in FreeInodes:
            buff += ("MISSING INODE < " + str(inode) + " > SHOULD BE IN FREE LIST < ")
            buff += str(BitmapPointers_FreeBlocks[int(inode)/InodesPerGroup])
            buff += " >\n"
    if verbose: print(buff)
    output_file.write(buff)
    return

# Incorrect Directory Entry
def write6():
    buff = ""
    global INCORRECT_DIRECTORY_ENTRIES
    for (parentInode, entryName, inodeNum, correct) in INCORRECT_DIRECTORY_ENTRIES:
        buff += ("INCORRECT ENTRY IN < " + str(parentInode) + " >")
        buff += (" NAME < " + str(entryName) + " >")
        buff += (" LINK TO < " + str(inodeNum) + " >")
        buff += (" SHOULD BE < " + str(correct) + " >\n")
    if verbose: print(buff)
    output_file.write(buff)
    return

# invalid block pointers
def write7():
    global INVALID_BLOCK_POINTERS
    buff = ""
    for (blocknum, inodenum, indirect_block, entry) in sorted(INVALID_BLOCK_POINTERS):
        buff += ("INVALID BLOCK < " + str(blocknum) + " >")
        buff += (" IN INODE < "     + str(inodenum) + " >")
        if int(indirect_block > 0): # ensure that indirect block is not zero
            buff += (" INDIRECT BLOCK < " + str(indirect_block) + " >")
        buff += (" ENTRY < " + str(entry) + " >\n")
    if verbose: print(buff)
    output_file.write(buff)
    return

if __name__ == "__main__":

    initStructs()
    handleIndirectBlocks()
    handleInodesInUse()
    handleDirectories()

    write1()
    write2()
    write3()
    write4and5()
    write6()
    write7()

    output_file.close() # finally close the file to ensure that all buffers are flushed
