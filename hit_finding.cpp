#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Gaussian kernel size
#define KERNEL_SIZE 5

// Intensity threshold for noise filtering
#define INTENSITY_THRESHOLD 5

/**
 * A helper function that reverses the byte order (endianness) of an integer
 * stored in a character buffer.
 *
 * @param buff A pointer to a character buffer of at least 4 bytes,
 * representing an integer.
 * @return The integer represented by the reversed byte order of the input
 * buffer.
 */
int reverse(char* buff) {
    char temp;
    for (int i = 0; i < 2; i++) {
        temp = buff[i];
        buff[i] = buff[3 - i];
        buff[3 - i] = temp;
    }
    return *(int*)buff;
} /* reverse() */

/**
 * A helper function that reads an integer from a binary file.
 *
 * @param fp A pointer to the file to read from.
 * @return The integer read from the file, or INT_MIN if an error occurred.
 */
int readInt(FILE* fp) {
    char buffer[4] = {};
    if (fread(buffer, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return INT_MIN;
    }
    return reverse(buffer);
} /* readInt() */


/**
 * Applies a Gaussian blur to a matrix.
 *
 * @param sigma The standard deviation of the Gaussian distribution.
 * @param matrix The matrix to apply the blur to.
 * @param output The matrix to store the output in.
 * @param rows The number of rows in the matrix.
 * @param cols The number of columns in the matrix.
 */
void gaussianBlur(float sigma, int** matrix, float** output, int rows, int cols) {
    int kernelRadius = KERNEL_SIZE / 2;
    float kernel[KERNEL_SIZE][KERNEL_SIZE];
    float sum = 0.0;

    // Generate the Gaussian kernel
    for (int x = -kernelRadius; x <= kernelRadius; x++) {
        for (int y = -kernelRadius; y <= kernelRadius; y++) {
            float exponent = -(x*x + y*y) / (2 * sigma * sigma);
            kernel[x + kernelRadius][y + kernelRadius] = exp(exponent) / (2 * M_PI * sigma * sigma);
            sum += kernel[x + kernelRadius][y + kernelRadius];
        }
    }

    // Normalize the kernel
    for (int i = 0; i < KERNEL_SIZE; i++) {
        for (int j = 0; j < KERNEL_SIZE; j++) {
            kernel[i][j] /= sum;
        }
    }

    // Apply the Gaussian kernel to the matrix
    for (int i = kernelRadius; i < rows - kernelRadius; i++) {
        for (int j = kernelRadius; j < cols - kernelRadius; j++) {
            float sum = 0.0;
            for (int x = -kernelRadius; x <= kernelRadius; x++) {
                for (int y = -kernelRadius; y <= kernelRadius; y++) {
                    sum += kernel[x + kernelRadius][y + kernelRadius] * matrix[i + x][j + y];
                }
            }
            output[i][j] = (int) sum;
        }
    }
} /* gaussianBlur() */


/**
 * Computes the difference of Gaussian of a matrix.
 *
 * @param sigma1 The standard deviation of the first Gaussian distribution.
 * @param sigma2 The standard deviation of the second Gaussian distribution.
 * @param input The matrix to compute the difference of Gaussian of.
 * @param dog The matrix to store the output in.
 * @param rows The number of rows in the matrix.
 * @param cols The number of columns in the matrix.
 */
void differenceOfGaussian(float sigma1, float sigma2, int** input, float** dog, int rows, int cols) {
    float** blur1 = (float**) calloc(rows, sizeof(float*));
    float** blur2 = (float**) calloc(rows, sizeof(float*));
    for (int i = 0; i < rows; i++) {
        blur1[i] = (float*) calloc(cols, sizeof(float));
        blur2[i] = (float*) calloc(cols, sizeof(float));
    }

    // Apply Gaussian blur
    gaussianBlur(sigma1, input, blur1, rows, cols);
    gaussianBlur(sigma2, input, blur2, rows, cols);

    // Compute the difference of Gaussian
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            dog[i][j] = blur1[i][j] - blur2[i][j];
        }
    }

    // Free memory
    for (int i = 0; i < rows; i++) {
        free(blur1[i]);
        free(blur2[i]);
    }
    free(blur1);
    free(blur2);
} /* differenceOfGaussian() */


/**
 * Detects blobs in a difference of Gaussian matrix.
 *
 * @param dog The difference of Gaussian matrix.
 * @param output The matrix to store the output in.
 * @param threshold The threshold value for blob detection.
 * @param rows The number of rows in the matrix.
 * @param cols The number of columns in the matrix.
 */
void detectBlobs(float** dog, int** output, float threshold, int rows, int cols) {
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < cols - 1; j++) {
            float center = dog[i][j];
            if (fabs(center) > threshold) {
                int isLocalMax = 1;
                for (int x = -1; x <= 1; x++) {
                    for (int y = -1; y <= 1; y++) {
                        if (x != 0 || y != 0) {
                            if (center <= dog[i + x][j + y]) {
                                isLocalMax = 0;
                                break;
                            }
                        }
                    }
                    if (!isLocalMax) {
                        break;
                    }
                }
                output[i][j] = isLocalMax ? 1 : 0;
            } else {
                output[i][j] = 0;
            }
        }
    }
} /* detectBlobs() */

void filter(int** matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix[i][j] < INTENSITY_THRESHOLD) {
                matrix[i][j] = 0;
            }
        }
    }
}

/**
 * Reads a matrix from a binary file and returns it as a 2D array.
 *
 * @param filename The name of the binary file to read from.
 * @return A 2D array representing the matrix read from the file, or NULL if
 * an error occurred. 
 */
int** readMatrix(char* filename) {
    int rows;
    int cols;
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }
    rows = readInt(file);
    cols = readInt(file);
    if (rows == INT_MIN || cols == INT_MIN) {
        return NULL;
    }
    int** matrix = (int**) calloc((rows + 1), sizeof(int*));
    matrix[0] = (int*) calloc(2, sizeof(int));
    matrix[0][0] = rows;
    matrix[0][1] = cols;
    for (int i = 1; i < rows + 1; i++) {
        matrix[i] = (int*) calloc(cols, sizeof(int));
    }
    for (int i = 1; i < rows + 1; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = readInt(file);
        }
    }
    fclose(file);
    // printf("Matrix read from file\n");
    return matrix;
} /* readMatrix() */

/**
 * Frees the memory allocated for a 2D array.
 *
 * @param matrix A pointer to the 2D array to free.
 * @param rows The number of rows in the matrix.
 */
void freeMatrix(int** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
    // printf("Matrix freed\n");
} /* freeMatrix() */

/**
 * Adjusts the matrix by removing the first row, which contains the number of
 * rows and columns.
 *
 * @param matrix A pointer to the 2D array to adjust.
 * @return A new 2D array representing the adjusted matrix.
 */
int** adjustMatrix(int** matrix) {
    int rows = matrix[0][0];
    int cols = matrix[0][1];
    int** new_matrix = (int**) calloc(rows, sizeof(int*));
    for(int i = 0; i < rows; i++) {
        new_matrix[i] = (int*) calloc(cols, sizeof(int));
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            new_matrix[i][j] = matrix[i+1][j];
        }
    }
    freeMatrix(matrix, rows + 1);
    // printf("Matrix adjusted\n");
    return new_matrix;
} /* adjustMatrix() */

/**
 * Finds blobs in a matrix using the difference of Gaussian method.
 *
 * @param filename The name of the binary file to read the matrix from.
 * @param sigma1 The standard deviation of the first Gaussian distribution.
 * @param sigma2 The standard deviation of the second Gaussian distribution.
 * @param threshold The threshold value for blob detection.
 * @return A 2D array with the coordinates of the detected blobs, or NULL if
 * no blobs were detected.
 */
extern "C" {
    int** findHitsDOG(char* filename, float sigma1, float sigma2, float threshold) {
        int** matrix = readMatrix(filename);
        if (matrix == NULL) {
            return NULL;
        }

        int rows = matrix[0][0];
        int cols = matrix[0][1];
        matrix = adjustMatrix(matrix);

        filter(matrix, rows, cols);

        float** dog = (float**) calloc(rows, sizeof(float*));
        for (int i = 0; i < rows; i++) {
            dog[i] = (float*) calloc(cols, sizeof(float));
        }

        differenceOfGaussian(sigma1, sigma2, matrix, dog, rows, cols);
        freeMatrix(matrix, rows);

        int** output = (int**) calloc(rows, sizeof(int*));
        for (int i = 0; i < rows; i++) {
            output[i] = (int*) calloc(cols, sizeof(int));
        }

        detectBlobs(dog, output, threshold, rows, cols);

        for (int i = 0; i < rows; i++) {
            free(dog[i]);
        }
        free(dog);

        int numBlobs = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (output[i][j] == 1) {
                    numBlobs++;
                }
            }
        }

        if (numBlobs == 0) {
            // printf("No blobs detected\n");
            freeMatrix(output, rows);
            return NULL;
        }

        int** hits = (int**) calloc((numBlobs + 1), sizeof(int*));
        hits[0] = (int*) calloc(2, sizeof(int));
        hits[0][0] = numBlobs;
        hits[0][1] = 2;
        int index = 1;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (output[i][j] == 1) {
                    hits[index] = (int*) calloc(2, sizeof(int));
                    hits[index][0] = i;
                    hits[index][1] = j;
                    index++;
                }
            }
        }

        freeMatrix(output, rows);

        // printf("Blobs detected\n");
        return hits;
    }
} /* findHitsDOG() */

/**
 * Frees the memory allocated for a 2D array.
 * 
 * @param array A pointer to the 2D array to free.
 * @param rows The number of rows in the matrix.
 */
extern "C" {
    void freeArray(int** array, int rows) {
        freeMatrix(array, rows);
    }
} /* freeArray() */
