import ctypes as ct
import sys
import glob
import natsort
import threading
import time
import matplotlib.pyplot as plt

# Variables
path = "" # Path to the file
sigma = 1.0
theshold = 5



# Load the DLL
clibrary = ct.CDLL('./hit_finding.dll')
# Define args and return types of findHitsDOG function
findHitsDOG = clibrary.findHitsDOG
findHitsDOG.argtypes = [ct.c_char_p, ct.c_float, ct.c_float, ct.c_float]
findHitsDOG.restype = ct.POINTER(ct.POINTER(ct.c_int))
# Define args and return types of freeArray function
freeArray = clibrary.freeArray
freeArray.argtypes = [ct.POINTER(ct.POINTER(ct.c_int)), ct.c_int]



def find_hits_dog_wrapper(file_path, sigma, threshold, target, index):
    """
    Wrapper function to find hits using Difference of Gaussians (DoG) function in the dll.

    Args:
        file_path (str): The path to the file.
        sigma (float): The standard deviation of the Gaussian kernel.
        threshold (float): The threshold value for hit detection.
        target (list): The target list to store the hits.
        index (int): The index of the target list to store the hits.

    Returns:
        None
    """
    # Convert file path to bytes
    file_path_bytes = file_path.encode('utf-8')

    # Call findHitsDOG function
    hits = findHitsDOG(file_path_bytes, sigma, sigma * 2, threshold)

    # Create a list to store the hits
    hits_list = []
    for i in range(1, hits[0][0] + 1):
        hits_list.append([hits[i][0], hits[i][1]])
    target[index] = hits_list

    # Call freeArray function
    freeArray(hits, hits[0][0] + 1)



def process_files(path, sigma, threshold):
    """
    Process files in the given path to find hits using Difference of Gaussians (DoG) method.

    Args:
        path (str): The path to the directory or file to process.
        sigma (float): The standard deviation of the Gaussian kernel used for DoG.
        threshold (float): The threshold value for detecting hits.

    Returns:
        None

    Raises:
        None

    Example:
        process_files('/path/to/files', 1.0, 0.5)
    """
    # start stopwatch
    global_start = time.time()

    # Check if program is called from terminal
    if len(sys.argv) == 2:
        path = sys.argv[1]

    # Split path for batch processing if applicable
    batch = []
    if path[-1] == "\\" or path[-1] == "/":
        for name in glob.glob(path + "[0-9]*"):
            batch += [name]
        batch = natsort.natsorted(batch, reverse=True)
        batch.reverse()
    else:
        batch = [path]

    # Process each file in a separate thread
    hits = [None] * len(batch)
    threads = []
    for i, file_path in enumerate(batch):
        thread = threading.Thread(target=find_hits_dog_wrapper, args=(file_path, sigma, threshold, hits, i))
        thread.start()
        threads.append(thread)

    # Wait for all threads to complete
    for thread in threads:
        thread.join()

    # Graph the hits
    for hit in hits:
        if hit is not None:
            for blob in hit:
                plt.plot(blob[1], blob[0], 'ro')

    print("Code ran in " + str(time.time() - global_start) + " seconds")
    plt.axis([0,460,0,460])
    plt.show()



# Call the process_files function
process_files(path, sigma, theshold)