#pragma once
#include "parsguiconfig.h"
#include "hdf5.h"
#include <iostream>

extern hid_t file, space, dset1, dset2, dset3, dset4, dset_timestamp, dcpl, mspace;
extern int saveCount, saveLength;
extern hsize_t		dims[2], chunk[2], offset[2];

bool runSaveThread();
bool startSave(int saveLength_segments);
bool stopSave();