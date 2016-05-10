Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  us/call  us/call  name    
 96.54    146.70   146.70   154801   947.66   947.66  acquire_lock
  2.42    150.37     3.68    68086    53.98    53.98  SortedList_insert
  1.18    152.17     1.79    77317    23.19    23.19  SortedList_lookup
  0.01    152.18     0.01   159026     0.06     0.06  getModulo
  0.01    152.19     0.01       18   556.39   556.39  SortedList_length
  0.00    152.19     0.00   159025     0.00     0.00  release_lock
  0.00    152.19     0.00    80000     0.00     0.00  alloc_rand_string
  0.00    152.19     0.00    79682     0.00     0.00  SortedList_delete
  0.00    152.19     0.00        1     0.00     0.00  free_memory
  0.00    152.19     0.00        1     0.00     0.00  init_and_fill
  0.00    152.19     0.00        1     0.00     0.00  init_sublists

 %         the percentage of the total running time of the
time       program used by this function.

cumulative a running sum of the number of seconds accounted
 seconds   for by this function and those listed above it.

 self      the number of seconds accounted for by this
seconds    function alone.  This is the major sort for this
           listing.

calls      the number of times this function was invoked, if
           this function is profiled, else blank.
 
 self      the average number of milliseconds spent in this
ms/call    function per call, if this function is profiled,
	   else blank.

 total     the average number of milliseconds spent in this
ms/call    function and its descendents per call, if this 
	   function is profiled, else blank.

name       the name of the function.  This is the minor sort
           for this listing. The index shows the location of
	   the function in the gprof listing. If the index is
	   in parenthesis it shows where it would appear in
	   the gprof listing if it were to be printed.

Copyright (C) 2012 Free Software Foundation, Inc.

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.

		     Call graph (explanation follows)


granularity: each sample hit covers 2 byte(s) for 0.01% of 152.19 seconds

index % time    self  children    called     name
                                                 <spontaneous>
[1]    100.0    0.00  152.19                 doWork [1]
              146.70    0.00  154801/154801      acquire_lock [2]
                3.68    0.00   68086/68086       SortedList_insert [3]
                1.79    0.00   77317/77317       SortedList_lookup [4]
                0.01    0.00  159026/159026      getModulo [5]
                0.01    0.00      16/18          SortedList_length [6]
                0.00    0.00  159025/159025      release_lock [8]
                0.00    0.00   79682/79682       SortedList_delete [10]
-----------------------------------------------
              146.70    0.00  154801/154801      doWork [1]
[2]     96.4  146.70    0.00  154801         acquire_lock [2]
-----------------------------------------------
                3.68    0.00   68086/68086       doWork [1]
[3]      2.4    3.68    0.00   68086         SortedList_insert [3]
-----------------------------------------------
                1.79    0.00   77317/77317       doWork [1]
[4]      1.2    1.79    0.00   77317         SortedList_lookup [4]
-----------------------------------------------
                0.01    0.00  159026/159026      doWork [1]
[5]      0.0    0.01    0.00  159026         getModulo [5]
-----------------------------------------------
                0.00    0.00       2/18          main [7]
                0.01    0.00      16/18          doWork [1]
[6]      0.0    0.01    0.00      18         SortedList_length [6]
-----------------------------------------------
                                                 <spontaneous>
[7]      0.0    0.00    0.00                 main [7]
                0.00    0.00       2/18          SortedList_length [6]
                0.00    0.00       1/1           init_sublists [13]
                0.00    0.00       1/1           init_and_fill [12]
                0.00    0.00       1/1           free_memory [11]
-----------------------------------------------
                0.00    0.00  159025/159025      doWork [1]
[8]      0.0    0.00    0.00  159025         release_lock [8]
-----------------------------------------------
                0.00    0.00   80000/80000       init_and_fill [12]
[9]      0.0    0.00    0.00   80000         alloc_rand_string [9]
-----------------------------------------------
                0.00    0.00   79682/79682       doWork [1]
[10]     0.0    0.00    0.00   79682         SortedList_delete [10]
-----------------------------------------------
                0.00    0.00       1/1           main [7]
[11]     0.0    0.00    0.00       1         free_memory [11]
-----------------------------------------------
                0.00    0.00       1/1           main [7]
[12]     0.0    0.00    0.00       1         init_and_fill [12]
                0.00    0.00   80000/80000       alloc_rand_string [9]
-----------------------------------------------
                0.00    0.00       1/1           main [7]
[13]     0.0    0.00    0.00       1         init_sublists [13]
-----------------------------------------------

 This table describes the call tree of the program, and was sorted by
 the total amount of time spent in each function and its children.

 Each entry in this table consists of several lines.  The line with the
 index number at the left hand margin lists the current function.
 The lines above it list the functions that called this function,
 and the lines below it list the functions this one called.
 This line lists:
     index	A unique number given to each element of the table.
		Index numbers are sorted numerically.
		The index number is printed next to every function name so
		it is easier to look up where the function is in the table.

     % time	This is the percentage of the `total' time that was spent
		in this function and its children.  Note that due to
		different viewpoints, functions excluded by options, etc,
		these numbers will NOT add up to 100%.

     self	This is the total amount of time spent in this function.

     children	This is the total amount of time propagated into this
		function by its children.

     called	This is the number of times the function was called.
		If the function called itself recursively, the number
		only includes non-recursive calls, and is followed by
		a `+' and the number of recursive calls.

     name	The name of the current function.  The index number is
		printed after it.  If the function is a member of a
		cycle, the cycle number is printed between the
		function's name and the index number.


 For the function's parents, the fields have the following meanings:

     self	This is the amount of time that was propagated directly
		from the function into this parent.

     children	This is the amount of time that was propagated from
		the function's children into this parent.

     called	This is the number of times this parent called the
		function `/' the total number of times the function
		was called.  Recursive calls to the function are not
		included in the number after the `/'.

     name	This is the name of the parent.  The parent's index
		number is printed after it.  If the parent is a
		member of a cycle, the cycle number is printed between
		the name and the index number.

 If the parents of the function cannot be determined, the word
 `<spontaneous>' is printed in the `name' field, and all the other
 fields are blank.

 For the function's children, the fields have the following meanings:

     self	This is the amount of time that was propagated directly
		from the child into the function.

     children	This is the amount of time that was propagated from the
		child's children to the function.

     called	This is the number of times the function called
		this child `/' the total number of times the child
		was called.  Recursive calls by the child are not
		listed in the number after the `/'.

     name	This is the name of the child.  The child's index
		number is printed after it.  If the child is a
		member of a cycle, the cycle number is printed
		between the name and the index number.

 If there are any cycles (circles) in the call graph, there is an
 entry for the cycle-as-a-whole.  This entry shows who called the
 cycle (as parents) and the members of the cycle (as children.)
 The `+' recursive calls entry shows the number of function calls that
 were internal to the cycle, and the calls entry for each member shows,
 for that member, how many times it was called from other members of
 the cycle.

Copyright (C) 2012 Free Software Foundation, Inc.

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.

Index by function name

  [10] SortedList_delete       [2] acquire_lock           [12] init_and_fill
   [3] SortedList_insert       [9] alloc_rand_string      [13] init_sublists
   [6] SortedList_length      [11] free_memory             [8] release_lock
   [4] SortedList_lookup       [5] getModulo
