/* 

SHOULD JUST USE NUMPY

 */

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

void remove_newline(char * string)
{
    int lenth = strlen(string);
    for (int i = 0; i < lenth; ++i)
    {
        if (string[i] == '\n' || string[i] == '\r') string[i] = 0;
    }
}

int main(int argc, char ** argv)
{
    /* Argument should be the data folder */
    if (argc == 2)
    {
        char * base_path = argv[1];
        char info_file_path[500];
        sprintf(info_file_path, "%s/info.txt", base_path);
        FILE * info_file = fopen(info_file_path, "r");

        puts("Processing data");
        if (info_file)
        {
            char diode_response[500];
            char start_wl[500];
            char step_wl[500];

            fgets(diode_response, sizeof(diode_response), info_file); // first, get past the irelevant lines
            fgets(diode_response, sizeof(diode_response), info_file);
            fgets(diode_response, sizeof(diode_response), info_file);
            fgets(diode_response, sizeof(diode_response), info_file);
            fgets(start_wl, sizeof(start_wl), info_file);
            fgets(step_wl, sizeof(step_wl), info_file);
            remove_newline(diode_response);

            printf("diode response file: %s\n", diode_response);

            char diode_response_full_path[500];
            char camera_data_full_path[500];
            char diode_data_full_path[500];
            sprintf(diode_response_full_path, "%s/%s", base_path, diode_response);
            sprintf(camera_data_full_path, "%s/camera.dat", base_path);
            sprintf(diode_data_full_path, "%s/diode.dat", base_path);

            int diode_response_len = 0;
            double * diode_response_data = read_data_file(diode_response_full_path, &diode_response_len, NULL);
            if (diode_response_len != 0) puts("Suckcess, Diode response loaded!");
            if (diode_response_len == 0) puts("Diode response NOT FOUND !!!!!!!!!!!!!!\n DATW MAY COME OUT WRONGF!");
            int camera_data_len = 0;
            int camera_data_width = 0;
            double * camera_data = read_data_file(camera_data_full_path, &camera_data_len, &camera_data_width);
            int diode_data_len = 0;
            double * diode_data = read_data_file(diode_data_full_path, &diode_data_len, NULL);

            char output_path[500]; /* IHATE C */
            sprintf(output_path, "%s/response.dat", base_path);
            FILE * output = fopen(output_path, "w");

            int wavelength = atoi(start_wl);
            int wavelength_step = atoi(step_wl);
            if (camera_data != NULL && diode_data != NULL)
            {
                for (int i = 0; i < camera_data_len; i += 2, wavelength += wavelength_step)
                {
                    /* Print wavelength */
                    fprintf(output, "%lf", (double)wavelength/10.0);

                    double diode_value = diode_data[i+1] - diode_data[i];
                    if (i < camera_data_len-3) 
                        diode_value = diode_data[i+1] - (diode_data[i]+diode_data[i+2])/2.0;

                    /* Loop thru channels */
                    for (int j = 0; j < camera_data_width; ++j)
                    {
                        double camera_value = camera_data[(i+1)*camera_data_width+j] - camera_data[i*camera_data_width+j];
                        if (camera_value < 0.0) camera_value = 0.0; // cause negative values rnt possible. This can happen in infrared region where response is probably 0 anyway
                        if (diode_response_len > 0)
                        {
                            /* Factor in diode response in this case */
                            double diode_response = linear_interpolate(((double)wavelength) / 10.0, diode_response_data, 2, diode_response_data+1, 2, diode_response_len);
                            fprintf(output, " %lf", (camera_value / (diode_value / diode_response)));
                        }
                        else
                        {
                            fprintf(output, " %lf", camera_value / diode_value);
                        }
                    }

                    fprintf(output, "\n");
                }
            }

            fclose(output);
        }
        else
        {
            puts("NOt processing data! No info.txt file found.");
        }
    }

    return 1;
}