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



# Read CSV files into program
superblock       = csv.reader(open('super.csv', 'rb'), delimiter=',')
group_descriptor = csv.reader(open('group.csv', 'rb'), delimiter=',')
bitmap           = csv.reader(open('bitmap.csv', 'rb'), delimiter=',')
inode            = csv.reader(open('inode.csv', 'rb'), delimiter=',')
directory        = csv.reader(open('directory.csv', 'rb'), delimiter=',')
indirect_blocks  = csv.reader(open('indirect.csv', 'rb'), delimiter=',')

# Open file to write for solution
fout = open('lab3b_check.txt', 'w+')

# Read verbose option, if any
verbose = False
parser = OptionParser()
parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False)
(options, args) = parser.parse_args()
verbose = options.verbose

if verbose == True:
    for line in superblock: print line
    for line in group_descriptor: print line
    for line in bitmap: print line
    for line in inode: print line
    for line in directory: print line
    for line in indirect_blocks: print line
    # for line in superblock: fout.write(str(line))
    # for line in group_descriptor: fout.write(str(line))
    # for line in bitmap: fout.write(str(line))
    # for line in inode: fout.write(str(line))
    # for line in directory: fout.write(str(line))
    # for line in indirect_blocks: fout.write(str(line))


# Create free block bitmap
bbmapblocks = []
for row in group_descriptor: bbmapblocks.append(int(row[bg_block_bitmap_block]))
print(bbmapblocks)


# Missing iNode :: inodes that are not in use, and not listed on the free bitmap
