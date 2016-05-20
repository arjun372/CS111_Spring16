import csv
from optparse import OptionParser

# Parse options, if any
# parser = OptionParser()
# parser.add_option("-v", "--verbose", help="verbose debugging", default=False)
# (options, args) = parser.parse_args()

superblock       = list(csv.reader(open('super.csv', 'rb')))
group_descriptor = list(csv.reader(open('group.csv', 'rb')))
bitmap           = list(csv.reader(open('bitmap.csv', 'rb')))
inode            = list(csv.reader(open('inode.csv', 'rb')))
directory        = list(csv.reader(open('directory.csv', 'rb')))
indirect_blocks  = list(csv.reader(open('indirect.csv', 'rb')))

for line in superblock: print line
for line in group_descriptor: print line
for line in bitmap: print line
for line in inode: print line
for line in directory: print line
for line in indirect_blocks: print line
