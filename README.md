# C++ DoG Hit-Finding
A C++ dll to find blobs using the Difference of Gaussian (DoG) algorithm.
The external functions accessible from the dll are
- findHitsDOG
  This function takes in the file name, sigma (width) for the Gaussian blur, and threshold used to determine what counts as a hit.
  This function returns a 2D array containing the locations of the identified hits. The 0th index is reserved for the number of hits detected i.e
  [[NUM_HITS,2], [HIT1_Y,HIT1_X], ...]
  The values used were found from experimentation and were as follows:
  1) sigma1 = 1
  2) sigma2 = 2 * sigma1 = 2
  3) threshold = 1

  The kernel size for the Gaussian blur is 5, which can be modified via the define statement at the top of the file.
  
  *Notes on file format*: the file is assumed to be formatted as follows:
  * Numbers are assumed to be big endian 32-bit integers
  * Data is expected to be formatted as "NUM_ROWS NUM_COLS DATA_PT1 DATA_PT2 ..." (note spaces were added for readability and are not assumed to be present in the file format)

- freeArray
  This function takes in a 2D array and the number of rows.
  This function returns nothing.
  Calling this function frees the 2D array (namely the array containing the hits) from memory.
  
  *Note regarding freeArray*: it is recommended to immediately copy the results of freeArray to a new array in Python in order to free up memory and allow Python to handle garbage collection. Not doing so may result in program crashes.

  ## Regarding Implimentation
  Please see lines 16-24 in the [example code](example/BlobFinding.py) for the necessary steps to implement this code, or refer to the ctypes library documentation.
  ```py
  import ctypes

  # Load the DLL
  clibrary = ct.CDLL('./hit_finding.dll')
  # Define args and return types of findHitsDOG function
  findHitsDOG = clibrary.findHitsDOG
  findHitsDOG.argtypes = [ct.c_char_p, ct.c_float, ct.c_float, ct.c_float]
  findHitsDOG.restype = ct.POINTER(ct.POINTER(ct.c_int))
  # Define args and return types of freeArray function
  freeArray = clibrary.freeArray
  freeArray.argtypes = [ct.POINTER(ct.POINTER(ct.c_int)), ct.c_int]
  ```

  To recompile the dll, run
  ```bash
  g++ -shared -o hit_finding.dll hit_finding.cpp
  ```
