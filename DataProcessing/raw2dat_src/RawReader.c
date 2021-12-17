#include "RawReader.h"

#include <stdlib.h>
#include "LibRaw/libraw/libraw.h"

struct RawReader {
    libraw_data_t * libraw;
    double matrix[9];
    char cam_name[32];

    int width;
    int height;
};


#define MIN(A, B) ((A < B) ? A : B)
#define MAX(A, B) ((A > B) ? A : B)


RawReader_t * new_RawReader(char * File)
{
    RawReader_t * Raw = malloc(sizeof(RawReader_t));
    memset(Raw, 0, sizeof(RawReader_t));

    libraw_data_t * libraw = libraw_init(0);

    Raw->libraw = libraw;

    /* No flipping */
    libraw->params.user_flip = 0;

    printf("Opening file %s\n", File);
    if (libraw_open_file(libraw, File) != 0) {
        puts("failed to open file");
        return NULL;
    }
    puts("unpacking raw...");
    if (libraw_unpack(libraw) != 0) {
        puts("failed to unpack");
        return NULL;
    }

    // if (libraw_strerror(libraw) != NULL) puts(libraw_strerror(libraw));

    /* Get matrix */
    double mat1[9]
    = /* { 0.625, -0.0711, -0.0808, -0.5153, 1.2794, 0.2636, -0.1249, 0.2198, 0.561 };    */  
        { /* 5d3 tungsten */
            0.7234, -0.1413, -0.0600,
            -0.3631,  1.1150,  0.2850,
            -0.0382,  0.1335,  0.6437
        };
    double wb[3] = {1,1,1};

    /* SUm all matrix values, to check if there is a matrix */
    float sum = 0.0;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            sum += fabsf(libraw->rawdata.color.cam_xyz[y][x]);
        } printf("\n");
    }

    if (sum > 0.0001)
    {
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                // printf("%3.5f, ", libraw->rawdata.color.cam_xyz[y][x]);
                mat1[y*3+x] = libraw->rawdata.color.cam_xyz[y][x] * wb[y];
            } printf("\n");
        }
    }
    else
    {
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                printf("%3.5f, ", libraw->rawdata.color.cam_xyz[y][x]);
                mat1[y*3+x] = libraw->rawdata.color.dng_color[0].colormatrix[y][x] * wb[y];
            } printf("\n");
        }
    }

    /* Get camera name */
    snprintf(Raw->cam_name, 32, "%s %s",libraw->idata.make,libraw->idata.model);
    printf("%s\nBlack level: %i, White: %i\n", Raw->cam_name, Raw->libraw->rawdata.color.black, Raw->libraw->rawdata.color.maximum);

    /* Get margins and width/height, canons for example have black area */
    int crop_top = libraw->rawdata.sizes.top_margin;
    int crop_left = libraw->rawdata.sizes.left_margin;
    int width = libraw_get_iwidth(libraw);
    /* Round width down to a multiple of 8 for MLV compatibility */
    width = width - (width % 8);
    int height = libraw_get_iheight(libraw);
    int raw_width = libraw_get_raw_width(libraw);
    int raw_height = libraw_get_raw_height(libraw);

    printf("Width: %i, Height: %i\n", width, height);

    if (crop_top != 0) printf("crop_top = %i\n", crop_top);
    if (crop_left != 0) printf("crop_left = %i\n", crop_left);

    /* Make sure bayer pattern starts at red to improve mlv support */
    #define START_AT_RED
    #ifdef START_AT_RED
    int top_pixel = libraw_COLOR(libraw, 0, 0);
    switch (top_pixel) {
        case 1:
            crop_left += 1;
            width -= 1;
            break;
        case 3:
            crop_top += 1;
            height -= 1;
            break;
        case 2:
            crop_top += 1;
            height -= 1;
            crop_left += 1;
            width -= 1;
            break;
    }
    #endif

    /***************** sony stareater to remove hotpixels ******************/
    uint16_t * temp_img = malloc(raw_width * raw_height * sizeof(uint16_t));
    for (int y = 2; y < raw_height-2; ++y)
    {
        uint16_t * prev_row = libraw->rawdata.raw_image + raw_width*(y-2);
        uint16_t * row = libraw->rawdata.raw_image + raw_width*(y);
        uint16_t * next_row = libraw->rawdata.raw_image + raw_width*(y+2);
        uint16_t * row_out = temp_img + raw_width*(y);
        for (int x = 2; x < raw_width-2; ++x)
        {
            uint16_t minimum = MIN(prev_row[x-2], prev_row[x]);
            minimum = MIN(minimum, prev_row[x+2]);
            minimum = MIN(minimum, row[x-2]);
            minimum = MIN(minimum, row[x+2]);
            minimum = MIN(minimum, next_row[x-2]);
            minimum = MIN(minimum, next_row[x]);
            minimum = MIN(minimum, next_row[x+2]);
            uint16_t maximum = MAX(prev_row[x-2], prev_row[x]);
            maximum = MAX(maximum, prev_row[x+2]);
            maximum = MAX(maximum, row[x-2]);
            maximum = MAX(maximum, row[x+2]);
            maximum = MAX(maximum, next_row[x-2]);
            maximum = MAX(maximum, next_row[x]);
            maximum = MAX(maximum, next_row[x   +2]);
            uint16_t result = row[x];
            if (row[x] < minimum) result = minimum;
            if (row[x] > maximum) result = maximum;
            row_out[x] = result;
        }
    }
    /* Copy back */
    for (int y = 2; y < raw_height-2; ++y)
        memcpy(libraw->rawdata.raw_image+(y*raw_width)+2, temp_img+(y*raw_width)+2, ((raw_width-4)*sizeof(uint16_t)));
    free(temp_img);

    /* HACK - memmove the image rows to remove the margins.
     * Not a great solution, as we are modifying inside libraw's memory */

    uint16_t * dst = libraw->rawdata.raw_image;
    uint16_t * src = dst + crop_top*raw_width + crop_left;

    for (int i = 0; i < height; ++i)
    {
        memmove(dst, src, width*sizeof(uint16_t));
        dst += width;
        src += raw_width;
    }

    Raw->width = width;
    Raw->height = height;

    Raw->libraw = libraw;
    return Raw;
}

void delete_RawReader(RawReader_t * Raw)
{
    libraw_recycle(Raw->libraw);
    libraw_close(Raw->libraw);
    free(Raw);
}

uint16_t * RawGetImageData(RawReader_t * Raw)
{
    return Raw->libraw->rawdata.raw_image;
}

int RawGetBlackLevel(RawReader_t * Raw)
{
    // return 2048;
    return Raw->libraw->rawdata.color.black;
}

int RawGetWhiteLevel(RawReader_t * Raw)
{
    // return 100;
    return Raw->libraw->rawdata.color.maximum-1000;
}

int RawGetWidth(RawReader_t * Raw)
{
    return Raw->width;
}

int RawGetHeight(RawReader_t * Raw)
{
    return Raw->height;
}

double * RawGetMatrix(RawReader_t * Raw)
{
    return Raw->matrix;
}

char * RawGetCamName(RawReader_t * Raw)
{
    return Raw->cam_name;
}

void RawGetTime(RawReader_t * Raw, time_t * TimeOutput)
{
    memcpy(TimeOutput, &Raw->libraw->other.timestamp, sizeof(time_t));
}

int RawGetNumChannels(RawReader_t * Raw)
{
    /* THis is xtrans */
    if (Raw->libraw->idata.filters == 9) return 3;
    /* Four for bayer, just in case the greens are different */
    else return 4;
}

void RawGetChannelAverages(RawReader_t * Raw, double * Output, int Radius)
{
    double channel_sums[4] = {0.0,0.0,0.0,0.0};
    uint64_t channel_count[4] = {0,0,0,0};

    int width = RawGetWidth(Raw);
    int height = RawGetHeight(Raw);
    uint16_t * img = RawGetImageData(Raw);

    int start_y = 0, end_y = height, start_x = 0, end_x = width;
    if (Radius != 0)
    {
        start_y = height/2 - Radius/2;
        end_y = height/2 + Radius/2;
        start_x = width/2 - Radius/2;
        end_x = width/2 + Radius/2;
    }

    /* Only sample the center with radius */
    for (int y = start_y; y < end_y; ++y)
    {
        int half = (y%2)*2;
        uint16_t * row = img + (y * width);

        for (int x = start_x; x < end_x; ++x)
        {
            int index = half+(x%2);
            /* For xtrans */
            if (Raw->libraw->idata.filters == 9) index = Raw->libraw->idata.xtrans[y%6][x%6];
            channel_sums[index] += row[x];
            channel_count[index]++;
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        Output[i] = channel_sums[i] / ((double)channel_count[i]);
    }
}