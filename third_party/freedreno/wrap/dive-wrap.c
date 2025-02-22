/*
Copyright 2023 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "dive-wrap.h"

#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t capture_state_lock = PTHREAD_MUTEX_INITIALIZER;
static int capture_state = 0;

void collect_trace_file(const char* capture_file_path);
int IsCapturing() 
{
	pthread_mutex_lock(&capture_state_lock);
	int is_capturing = (capture_state == 1);
	pthread_mutex_unlock(&capture_state_lock);
	return is_capturing;
}

extern void SetCaptureState(int state) 
{
	LOGD("SetCaptureState %d", state);
	pthread_mutex_lock(&capture_state_lock);

	if(state == 0 && capture_state == 1)
	{
		const char *num = getenv("TESTNUM");
		if (!num)
			num = "0";
		int frame_num = atoi(num);
		char path[1024];
		snprintf(path, 1024, "/sdcard/Download/trace-frame-%04u.rd", frame_num);		
		collect_trace_file(path);
	}
		
	capture_state = state;
	pthread_mutex_unlock(&capture_state_lock);
}

void StartCapture() { SetCaptureState(1); }

void StopCapture() { SetCaptureState(0); }

extern void SetCaptureName(const char* name, const char* frame_num) 
{
	if(name) {
		setenv("TESTNAME", name, 1);
	}
	if(frame_num) {
		setenv("TESTNUM", frame_num, 1);
	}
	// Cap dump size per buffer as 128K.
	// TODO(renfeng): find out doc where states the max size of command stream.
	setenv("WRAP_BUF_LEN_CAP", "131072", 1);
}

