#define DR_WAV_IMPLEMENTATION

#include "dr_wav.h"
#include <vector>
#include <string>
#include <stdio.h>
#include <math.h>



void ma_pcm_f32_to_s24__reference(void* dst, const void* src, uint64_t count)
{
    uint8_t* dst_s24 = (uint8_t*)dst;
    const float* src_f32 = (const float*)src;

    uint64_t i;
    for (i = 0; i < count; i += 1) {
        int32_t r;
        float x = src_f32[i];
        x = ((x < -1) ? -1 : ((x > 1) ? 1 : x));    /* clip */

        /* The fast way. */
        x = x * 8388607.0f;                         /* -1..1 to -8388607..8388607 */


        r = (int32_t)x;
        dst_s24[(i*3)+0] = (uint8_t)((r & 0x0000FF) >>  0);
        dst_s24[(i*3)+1] = (uint8_t)((r & 0x00FF00) >>  8);
        dst_s24[(i*3)+2] = (uint8_t)((r & 0xFF0000) >> 16);
    }


}

bool save(const std::string &path, const float *buffer, size_t numFrames, int numChannels, int sampleRate) {
	
	
	// TODO: stereo
	drwav outfile;
	drwav_data_format fmt;
	fmt.container = drwav_container_riff;
	fmt.format = DR_WAVE_FORMAT_PCM;
	fmt.channels = numChannels;
	fmt.sampleRate = sampleRate;

	fmt.bitsPerSample = 24;

	
	drwav_init_write_sequential_pcm_frames(&outfile, &fmt, numFrames, nullptr, nullptr, nullptr);
	drwav_init_file_write(&outfile, path.c_str(), &fmt, nullptr);

	
	std::vector<int8_t> pcmOut(numFrames * numChannels * fmt.bitsPerSample/8);
	
	ma_pcm_f32_to_s24__reference(pcmOut.data(), buffer, numFrames * numChannels);

	drwav_write_pcm_frames(&outfile, numFrames, pcmOut.data());

	
	drwav_uninit(&outfile);

	
	return true;
}


bool save(const std::string &path, const std::vector<float> &buff, int numChannels, int sampleRate) {
	return save(path, buff.data(), buff.size() / numChannels, numChannels, sampleRate);
}
#define SAMPLE_RATE 44100

void addSilence(std::vector<float> &a, int numFrames) {
	for(int i = 0; i < numFrames; i++) {
		a.push_back(0);
	}
}


void addSine(std::vector<float> &a, float frequency, float amp, int duration) {
	double phase = 0;
	double phaseInc = M_PI * 2.0 * frequency / (double) SAMPLE_RATE;
	for(int i = 0; i < duration; i++) {
		a.push_back(sin(phase) * amp);
		phase += phaseInc;
	}
}



void addLinSineSweep(std::vector<float> &a, float fromFreq, float toFreq, float amp, int duration) {
	double phase = 0;
	
	for(int i = 0; i < duration; i++) {
		float amt = i / (float) duration;
		float frequency = fromFreq + (toFreq - fromFreq) * amt;
		double phaseInc = M_PI * 2.0 * frequency / (double) SAMPLE_RATE;
		a.push_back(sin(phase) * amp);
		phase += phaseInc;
	}
}

void addLogSineSweep(std::vector<float> &a, float fromFreq, float toFreq, float amp, int duration) {
	double phase = 0;
	
	for(int i = 0; i < duration; i++) {


		float beta = duration / log(toFreq / fromFreq);

		phase = 2.0 * M_PI * beta * fromFreq * (pow(toFreq / fromFreq, i/(double)duration) - 1);

		a.push_back(sin((phase + M_PI/180.0)/(double)SAMPLE_RATE) * amp);

	}
}



void addImpulse(std::vector<float> &a, float volume, int trailingZeros) {
	a.push_back(volume);
	addSilence(a, trailingZeros);
}

void addNoise(std::vector<float> &a, float volume, int duration) {
	for(int i = 0; i < duration; i++) {
		float n = (rand() % 100000) / 100000.0;
		n -= 0.5;
		n *= 2.f;
		if(n>1) n = 1;
		else if(n<-1) n = -1;
		a.push_back(n * volume);
	}
}


int main() {
	std::vector<float> out;
	addSilence(out, 50000);


	for(int i = 80; i < 20000; i *= sqrt(2)) {
		addSine(out, i, 0.125, 30000);	
		addSilence(out, 5000);
	}
	addSilence(out, 10000);
	for(int i = 80; i < 20000; i *= sqrt(2)) {
		addSine(out, i, 0.25, 30000);	
		addSilence(out, 5000);
	}
	addSilence(out, 10000);
	for(int i = 80; i < 20000; i *= sqrt(2)) {
		addSine(out, i, 0.5, 30000);	
		addSilence(out, 5000);
	}
	addSilence(out, 10000);
	for(int i = 80; i < 20000; i *= sqrt(2)) {
		addSine(out, i, 0.75, 30000);	
		addSilence(out, 5000);
	}
	

	addSilence(out, 20000);
	for(float v = 0.1; v <=1; v += 0.1) {
		addImpulse(out, v, 20000);
	}

	for(float v = 0.1; v <=1; v += 0.1) {
		addNoise(out, v, 50000);
		addSilence(out, 5000);
	}
	
	addSilence(out, 10000);
	for(float v = 0.1; v <=1; v += 0.1) {
		addLogSineSweep(out, 50, 20000, v, SAMPLE_RATE * 4);
	}



	save("out.wav", out, 1, SAMPLE_RATE);
	return 0;
}
