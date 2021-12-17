#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "RawReader.h"

#define MIN(A, B) ((A < B) ? A : B)
#define MAX(A, B) ((A > B) ? A : B)

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

        if (info_file)
        {
            char raw_path1[100];
            char raw_path_number[100];
            char raw_path2[100];

            fgets(raw_path1, sizeof(raw_path1), info_file);
            fgets(raw_path_number, sizeof(raw_path_number), info_file);
            fgets(raw_path2, sizeof(raw_path2), info_file);

            remove_newline(raw_path1);
            remove_newline(raw_path_number);
            remove_newline(raw_path2);

            /* How many digits in the filename */
            int num_digits = strlen(raw_path_number);
            int raw_file_num = atoi(raw_path_number);

            /* Output file */
            char output_path[500];
            sprintf(info_file_path, "%s/camera.dat", base_path);
            FILE * output_file = fopen(info_file_path, "w");

            int keep_going = 1;
            while (keep_going)
            {
                char format_string[100];
                sprintf(format_string, "%%s/%%s%%0%dd%%s", num_digits);
                char raw_path[500];
                sprintf(raw_path, format_string, base_path, raw_path1, raw_file_num, raw_path2);
                puts(raw_path);

                FILE * test = fopen(raw_path, "r");
                if (test)
                {
                    fclose(test);
                    /* Get raw value an output it */
                    RawReader_t * raw = new_RawReader(raw_path);
                    double channel_sums[4];
                    RawGetChannelAverages(raw, channel_sums, MAX((RawGetHeight(raw)/3.0), 350));

                    fprintf(output_file, "%lf %lf %lf\n", channel_sums[0], (channel_sums[1]+channel_sums[2])/2.0, channel_sums[3]); // average the two greens
                    fflush(output_file);

                    delete_RawReader(raw);
                }
                else if (raw_file_num != 0)
                {
                    keep_going = 0;
                }

                raw_file_num = (raw_file_num + 1) % ((int)pow(10,num_digits));
            }

            fclose(output_file);
        }
        else
        {
            puts("couldnt open file info.txt");
        }

        fclose(info_file);
    }

    return 0;
}