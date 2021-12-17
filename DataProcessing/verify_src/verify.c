#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define MIN(A, B) ((A < B) ? A : B)
#define MAX(A, B) ((A > B) ? A : B)

double linear_interpolate(double value, double * x_values, int x_stride, double * y_values, int y_stride, int num_values)
{
    // printf("value %lf, x_values %p, x_stride %i, y_values %p, y_stride %i, num_values %i\n", value, x_values, x_stride, y_values, y_stride, num_values);
    if (x_values == NULL) return 1.0;
    if (y_values == NULL) return 1.0;

    int i = 1;

    if (value == x_values[0]) return y_values[0];

    while (i < num_values)
    {
        if (x_values[i*x_stride] == value) return y_values[i*y_stride];
        if (x_values[(i-1)*x_stride] < value && value < x_values[i*x_stride]) break;
        ++i;
    }

    double fac = (value-x_values[(i-1)*x_stride]) / (x_values[i*x_stride]-x_values[(i-1)*x_stride]);

    return (y_values[(i-1)*y_stride] * (1.0-fac)) + (y_values[i*y_stride] * fac);
}


/* Returns new text position */
char * read_row(char * TextData, double * OutputValues, int * OutputNumValuesRead)
{
    char * text = TextData;
    int num_values_read = 0;
    
    while (1)
    {
        if (isalnum(*text) || *text == '.')
        { /* Read numbver value */
            *OutputValues = atof(text);
            OutputValues++;
            ++num_values_read;
            while (isalnum(*text) || *text == '.') ++text;
        }
        else if (*text == '\n' || *text == '\r')
        { /* IDK I think \r may be newline on windows or somethinf (avoid wibmows) */
            break;
        }
        else ++text;
        if (*text == 0) break;
    }

    *OutputNumValuesRead = num_values_read;
    return (*text = 0) ? NULL : text;
}

double * read_data_file(char * FileName, int * OutputNumRows, int * OutputNumColumns)
{
    FILE * datafile = fopen(FileName, "r");
    if (datafile == NULL){printf("Couldn't read %s\n", FileName); return NULL;}
    fseek(datafile, 0, SEEK_END);
    int filesize = ftell(datafile);
    char * text_data = calloc(filesize+100, sizeof(char));
    fseek(datafile, 0, SEEK_SET);
    fread(text_data, filesize, 1, datafile);
    fclose(datafile);

    int num_cols = 0;
    int num_rows = 0;
    double * data = malloc(sizeof(double) * 100000); /* Up to 100000 values */
    double * data_pointer = data;

    char * text_ptr = text_data;
    while (1)
    {
        int num_cols_read = 0;
        text_ptr = read_row(text_ptr, data_pointer, &num_cols_read);
        if (text_ptr == NULL) {break;puts("hu");}

        if (num_rows == 0) num_cols = num_cols_read;
        else {
            /* This means either an inconsistency in number of columns or the
             * end of the file (zero), so stopping makes sesne */
            if (num_cols_read != num_cols) break;
        }

        ++num_rows;
        data_pointer += num_cols;
    }

    data = realloc(data, sizeof(double) * num_cols * num_rows);

    if (OutputNumColumns != NULL) *OutputNumColumns = num_cols;
    if (OutputNumRows != NULL) *OutputNumRows = num_rows;
    return data;
}

/* Very uzeful diagonal flip feature */
static int diagonal_flip[9] = { 0, 3, 6, 1, 4, 7, 2, 5, 8 };
/* Makes it like a function */
#define diag_flip(X) diagonal_flip[(X)]

void invertMatrix(double * inputMatrix, double * outputMatrix)
{
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            /* Determenant locations for 2 x 2 */
            int dX[2] = { (x + 1) % 3, (x + 2) % 3 };
            int dY[2] = { 3 * ((y + 1) % 3), 3 * ((y + 2) % 3) };

            outputMatrix[ diag_flip(y*3 + x) ] = 
            (   /* Determinant caluclation 2 x 2 */
                  inputMatrix[dY[0] + dX[0]] 
                * inputMatrix[dY[1] + dX[1]]
                - inputMatrix[dY[0] + dX[1]] 
                * inputMatrix[dY[1] + dX[0]]
            );
        }
    }

    /* Calculate whole matrix determinant */
    double determinant = 1.0 / (
          inputMatrix[0] * ( inputMatrix[8] * inputMatrix[4] - inputMatrix[7] * inputMatrix[5] )
        - inputMatrix[3] * ( inputMatrix[8] * inputMatrix[1] - inputMatrix[7] * inputMatrix[2] )
        + inputMatrix[6] * ( inputMatrix[5] * inputMatrix[1] - inputMatrix[4] * inputMatrix[2] )
    );

    /* Multiply all elements by the determinant */
    for (int i = 0; i < 9; ++i) outputMatrix[i] *= determinant;
}


int main(int argc, char ** argv)
{
    double cam[] = {
        0.6722, -0.0635, -0.0963,
        -0.4287,  1.2460,  0.2028,
        -0.0908,  0.2162,  0.5668
    };
    double M[9];
    invertMatrix(cam, M);
    if (argc >= 3)
    {
        int cam_response_len;
        int cam_response_width;
        double * cam_response = read_data_file(argv[1], &cam_response_len, &cam_response_width);

        for (int i = 2; i < argc; ++i)
        {
            int spectrum_len;
            double * spectrum = read_data_file(argv[i], &spectrum_len, NULL);

            float R = 0.0;
            float G = 0.0;
            float B = 0.0;

            for (int j = 0; j < cam_response_len; ++j)
            {
                double wl = cam_response[j*cam_response_width];

                // printf("wl=%f\n", wl);
                double spectrum_value = linear_interpolate(((double)wl), spectrum, 2, spectrum+1, 2, spectrum_len);

                R += cam_response[j*cam_response_width+1] * spectrum_value;
                G += cam_response[j*cam_response_width+2] * spectrum_value;
                B += cam_response[j*cam_response_width+(cam_response_width-1)] * spectrum_value;
            }

            printf("R=%f, G=%f, B=%f\n", R, G, B);
            double Y = R*M[3]+G*M[4]+B*M[5];
            double X = (R*M[0]+G*M[1]+B*M[2]) / Y;
            double Z = (R*M[6]+G*M[7]+B*M[8]) / Y;
            Y = 1.0;
            printf("X=%f, Y=%f, Z=%f\n", X, Y, Z);
            printf("x=%f, y=%f\n", X/(X+Y+Z), Y/(X+Y+Z));
        }
    }
    else
    {
        puts("Usage: ./verify /path/to/camera_response.dat /path/to/spectrum_0.dat ... /path/to/spectrum_n.dat");
    }

    return 0;
}