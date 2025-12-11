#include "saveThread.h"

hid_t   file, space, dset1, dset2, dset3, dset4, dset_timestamp, dcpl, mspace, coord_space, coord_dcpl, timestamp_space, timestamp_dcpl, timestamp_fspace, xy_coord;
hsize_t     dims[2] = { 0,0 },
            chunk[2] = { 0,0 },
            coord_dims[2] = {2,1},
            timestamp_dims[2] = {0, 0},
            offset[2];

int saveCount, saveLength;
std::string save_file_name;
uint64_t* save_timestamps;

bool runSaveThread() {
    herr_t      status;

	while (gui_running) {
		std::unique_lock<std::mutex> ul(data_mutex);
		data_cv.wait(ul, []() { return ch_data_ready.load(); });
        ch_data_ready = false;
        ul.unlock();
        auto start = std::chrono::high_resolution_clock::now();
        if (save_bool) {
            // Add time stamp to array
            memcpy(&save_timestamps[saveCount * NUMBER_OF_SEGMENTS_PER_BUFFER], timestamps, sizeof(uint64_t) * NUMBER_OF_SEGMENTS_PER_BUFFER);

            // Save the time domains
            offset[0] = saveCount * chunk[0];
            offset[1] = 0;

            status = H5Dwrite_chunk(dset1, H5P_DEFAULT, 0, offset, chunk[0] * chunk[1] * sizeof(int16_t), channel1_V);
            status = H5Dwrite_chunk(dset2, H5P_DEFAULT, 0, offset, chunk[0] * chunk[1] * sizeof(int16_t), channel2_V);
            status = H5Dwrite_chunk(dset3, H5P_DEFAULT, 0, offset, chunk[0] * chunk[1] * sizeof(int16_t), channel3_V);
            status = H5Dwrite_chunk(dset4, H5P_DEFAULT, 0, offset, chunk[0] * chunk[1] * sizeof(int16_t), channel4_V);
            //H5Fflush(file, H5F_SCOPE_GLOBAL);

            saveCount++;

            if (!(saveCount < saveLength)) {
                stopSave();
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        save_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            avg_save_time += save_time[i];
        }
        avg_save_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;
	}
	return 0;
}

bool startSave(int saveLength_segments) {
    herr_t      status;

    // Saving time domains
    dims[0] = saveLength_segments;
    dims[1] = NUMBER_OF_SAMPLES_PER_SEGMENT;

    chunk[0] = NUMBER_OF_SEGMENTS_PER_BUFFER;
    chunk[1] = NUMBER_OF_SAMPLES_PER_SEGMENT;

    time_t t = time(NULL);
    struct tm buff;
    localtime_s(&buff, &t);
    char dateTime[100];
    std::strftime(dateTime, sizeof(dateTime), "%Y_%m_%d_%H_%M_%S", &buff);
    std::string dateTime_str = dateTime;
    save_file_name = SAVE_PATH + dateTime_str + "_data.h5";
    
    /*
    * Create a new file using the default properties.
    */
    file = H5Fcreate(save_file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // Code for creating and closing XY coords
    coord_space = H5Screate_simple(2, coord_dims, NULL);
    coord_dcpl = H5Pcreate(H5P_DATASET_CREATE);
    xy_coord = H5Dcreate(file, "xy_coord_mm", H5T_IEEE_F32LE, coord_space, H5P_DEFAULT, coord_dcpl, H5P_DEFAULT);

    status = H5Pclose(coord_dcpl);
    status = H5Dclose(xy_coord);
    status = H5Sclose(coord_space);


    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    space = H5Screate_simple(2, dims, NULL);

    /*
    * Create the dataset creation property list, and set the chunk
    * size.
    */
    dcpl = H5Pcreate(H5P_DATASET_CREATE);
    status = H5Pset_chunk(dcpl, 2, chunk);
    //status = H5Pset_alloc_time(dcpl, H5D_ALLOC_TIME_EARLY);
    //status = H5Pset_fill_time(dcpl, H5D_FILL_TIME_ALLOC);

    std::cout << status << "\n";
    /*
    * Create the chunked dataset.
    */
    dset1 = H5Dcreate(file, "Channel1", H5T_STD_I16LE, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    dset2 = H5Dcreate(file, "Channel2", H5T_STD_I16LE, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    dset3 = H5Dcreate(file, "Channel3", H5T_STD_I16LE, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    dset4 = H5Dcreate(file, "Channel4", H5T_STD_I16LE, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);


    // Saving time stamps
    save_timestamps = new uint64_t[saveLength_segments];
    timestamp_dims[0] = 1;
    timestamp_dims[1] = saveLength_segments;

    timestamp_space = H5Screate_simple(2, timestamp_dims, NULL);
    timestamp_dcpl = H5Pcreate(H5P_DATASET_CREATE);
    dset_timestamp = H5Dcreate(file, "Time Stamps", H5T_STD_U64LE, timestamp_space, H5P_DEFAULT, timestamp_dcpl, H5P_DEFAULT);

    saveCount = 0;
    saveLength = saveLength_segments/chunk[0];
    save_bool = true;
    std::cout << "Save started...";


	return 0;
}


bool stopSave() {
    herr_t      status;
    save_bool = false;
    saveCount = 0;
    timestamp_fspace  = H5Dget_space(dset_timestamp);
    status = H5Dwrite(dset_timestamp, H5T_STD_U64LE, timestamp_space, timestamp_fspace, H5P_DEFAULT, save_timestamps);

    /*
     * Close and release resources.
     */
    status = H5Pclose(dcpl);
    status = H5Pclose(timestamp_dcpl);

    status = H5Dclose(dset1);
    status = H5Dclose(dset2);
    status = H5Dclose(dset3);
    status = H5Dclose(dset4);
    status = H5Dclose(dset_timestamp);


    status = H5Sclose(space);
    status = H5Sclose(timestamp_space);

    status = H5Fclose(file);

    delete [] save_timestamps;

    std::cout << "Save stopped\n";

    return 0;
}