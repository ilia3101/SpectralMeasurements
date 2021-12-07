#ifndef _RawReader_h_
#define _RawReader_h_

#include <stdint.h>
#include <time.h> 

typedef struct RawReader RawReader_t;

/* Creation */
RawReader_t * new_RawReader(char * File);
void delete_RawReader(RawReader_t * Raw);

/* Getters */
uint16_t * RawGetImageData(RawReader_t * Raw);
int RawGetBlackLevel(RawReader_t * Raw);
int RawGetWhiteLevel(RawReader_t * Raw);
int RawGetWidth(RawReader_t * Raw);
int RawGetHeight(RawReader_t * Raw);
double * RawGetMatrix(RawReader_t * Raw);
char * RawGetCamName(RawReader_t * Raw);
void RawGetTime(RawReader_t * Raw, time_t * TimeOutput);

/* How many channels camera has */
int RawGetNumChannels(RawReader_t * Raw);
/* Get channel averages, if radius is 0, whole frame will be sampled */
void RawGetChannelAverages(RawReader_t * Raw, double * Output, int Radius);

#endif