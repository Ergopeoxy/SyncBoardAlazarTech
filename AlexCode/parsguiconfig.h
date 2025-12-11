#pragma once
#include <atomic>
#include <mutex>
#include<condition_variable>

# define SAVE_PATH ("D:\\PARS_GUI_Save_Folder\\")


// Global constants
const long long NUMBER_OF_CHANNELS = 4;
const long long NUMBER_OF_SEGMENTS_PER_BUFFER = 1;
const long long NUMBER_OF_SAMPLES_PER_SEGMENT = 68000;
const long long NUMBER_OF_BUFFERS_PER_ACQUISITION = 1000;
const long long ACQUISITION_CARD_SAMPLING_FREQUENCY = 250000000LL;
const long long NUMBER_OF_PROCESSING_TIME_SAMPLES = 100;
const long long NUMBER_OF_TD_SIGNALS = 50;
const long long NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER = 100;

const int NUMBER_OF_POINTS_PER_LINE = 513;


// Defined constants for alazartech card
#define PRE_TRIGGER_SAMPLES 0
#define POST_TRIGGER_SAMPLES 68000
#define BUFFER_COUNT 8


// General use variables
extern double	x_samples[NUMBER_OF_SAMPLES_PER_SEGMENT],
				x_segments[NUMBER_OF_SEGMENTS_PER_BUFFER],
				x_cb_segments[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER],
				x_line_samples[NUMBER_OF_POINTS_PER_LINE],
				x_time_samples[NUMBER_OF_PROCESSING_TIME_SAMPLES];

// timing for acquisition cards
extern double data_deinterlacing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES];
extern double avg_data_deinterlacing_time;

extern double data_acquisition_time[NUMBER_OF_PROCESSING_TIME_SAMPLES];
extern double avg_data_acquisition_time;

extern double save_time[NUMBER_OF_PROCESSING_TIME_SAMPLES]; 
extern double avg_save_time;


// raw td channel variables
extern int16_t	channel1_V[NUMBER_OF_SEGMENTS_PER_BUFFER][NUMBER_OF_SAMPLES_PER_SEGMENT],
				channel2_V[NUMBER_OF_SEGMENTS_PER_BUFFER][NUMBER_OF_SAMPLES_PER_SEGMENT],
				channel3_V[NUMBER_OF_SEGMENTS_PER_BUFFER][NUMBER_OF_SAMPLES_PER_SEGMENT],
				channel4_V[NUMBER_OF_SEGMENTS_PER_BUFFER][NUMBER_OF_SAMPLES_PER_SEGMENT];

extern uint64_t	timestamps[NUMBER_OF_SEGMENTS_PER_BUFFER];

extern int time_index;
extern long buffer_index;


extern std::mutex data_mutex;
extern std::condition_variable data_cv;
extern std::atomic<bool>	ch1_data_ready,
							ch2_data_ready,
							ch3_data_ready,
							ch4_data_ready;
extern std::atomic<int> channel_read_count;

extern bool gui_running;

// Alazartech code
extern bool running;

// Board configuration parameters
extern double samplesPerSec;
extern uint32_t preTriggerSamples, postTriggerSamples, recordsPerBuffer, buffersPerAcquisition, bytesPerBuffer;
extern uint16_t* BufferArray[2][BUFFER_COUNT];


//Save thread variables
extern std::atomic<bool> save_bool;
extern std::atomic<bool> ch_data_ready;