

uint8 AudioDevice::contextInitialized = false;
AUDIOSTREAM *AudioDevice::stream;
int AudioDevice::numSamples;
std::vector<short> AudioDevice::buf;
float *AudioDevice::floatBuf = NULL;
short *AudioDevice::wordBuf = NULL;

static int wssSampleRate;

static int sampleConvSize;
static volatile int audio_tick = 0;

static short *sampleConvBuf;

static int latency_size = 48000 / 30;

static int wssaudio_write(short * buffer, int len)
{
	int samples = w_get_buffer_size() - w_get_latency() - latency_size;
	if((samples <= 0) || (len == 0))
		return 0;
	if(len > samples)
		len = samples;
	w_lock_mixing_buffer(len);
	w_mixing_stereo(buffer, len, 256, 256);
	w_unlock_mixing_buffer();
	return len;
}

static bool wssaudio_init_device(int rate) {    
    if (!w_sound_device_init(28, rate)) { // 28 = High-Definition Audio
	PrintLog(PRINT_NORMAL, "Unable to open HDA audio device: %s", w_get_error_message());
    } else {
        PrintLog(PRINT_NORMAL, "WSS audio HDA init OK.");
	return true;
    }
    
    if (!w_sound_device_init(3, rate)) { // 3 = AC97 auto detected
        PrintLog(PRINT_NORMAL, "Unable to open AC97 audio device: %s", w_get_error_message());
    } else {
        PrintLog(PRINT_NORMAL, "WSS audio AC97 init OK.");
        return true;
    }

    if (!w_sound_device_init(1, rate)) { // 1 = Sound Blaster autodetect
        PrintLog(PRINT_NORMAL, "Unable to open Sound Blaster audio device: %s", w_get_error_message());
    } else {
        PrintLog(PRINT_NORMAL, "WSS audio Sound Blaster init OK.");
        return true;
    }
    
    
    return false;
}

// from https://github.com/cpuimage/resampler, by Zhihan Gao

static uint64_t Resample_s16(const int16_t *input, int16_t *output, int inSampleRate, int outSampleRate, uint64_t inputSize,
                      uint32_t channels
) {
    uint64_t outputSize = (uint64_t) (inputSize * (double) outSampleRate / (double) inSampleRate);
    outputSize -= outputSize % channels;
    if (output == NULL)
        return outputSize;
    if (input == NULL)
        return 0;
    double stepDist = ((double) inSampleRate / (double) outSampleRate);
    const uint64_t fixedFraction = (1LL << 32);
    const double normFixed = (1.0 / (1LL << 32));
    uint64_t step = ((uint64_t) (stepDist * fixedFraction + 0.5));
    uint64_t curOffset = 0;
    for (uint32_t i = 0; i < outputSize; i += 1) {
        for (uint32_t c = 0; c < channels; c += 1) {
            *output++ = (int16_t) (input[c] + (input[c + channels] - input[c]) * (
                    (double) (curOffset >> 32) + ((curOffset & (fixedFraction - 1)) * normFixed)
            )
            );
        }
        curOffset += step;
        input += (curOffset >> 32) * channels;
        curOffset &= (fixedFraction - 1);
    }
    return outputSize;
}

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
    }
    
    w_set_device_master_volume(0);
    
    if (!wssaudio_init_device(44100)) {
        PrintLog(PRINT_NORMAL, "Unable to open audio device");
        audioState = false;
        return true;
    }

    audioState = true;
    
    wssSampleRate = w_get_nominal_sample_rate();
    PrintLog(PRINT_NORMAL, "Audio sample rate = %d", wssSampleRate); 
    
    numSamples = MIX_BUFFER_SIZE / AUDIO_CHANNELS;


    if (wssSampleRate != 44100) {
	sampleConvSize = Resample_s16(NULL, NULL, 44100, 
	    wssSampleRate, numSamples, 2) * 2 * 2;
	sampleConvBuf = new short[sampleConvSize / sizeof(short)];
    }
    else
        sampleConvSize = -1;
    
    PrintLog(PRINT_NORMAL, "Initialized WSS audio.");
    
    return true;
}

void AudioDevice::Release()
{
    delete floatBuf;
    delete wordBuf;
    delete sampleConvBuf;
	
    w_sound_device_exit();
}

void AudioDevice::InitAudioChannels()
{
    for (int32 i = 0; i < CHANNEL_COUNT; ++i) {
        channels[i].soundID = -1;
        channels[i].state   = CHANNEL_IDLE;
    }

    for (int32 i = 0; i < 0x400; i += 2) {
        speedMixAmounts[i]     = (i + 0) * (1.0f / 1024.0f);
        speedMixAmounts[i + 1] = (i + 1) * (1.0f / 1024.0f);
    }

    GEN_HASH_MD5("Stream Channel 0", sfxList[SFX_COUNT - 1].hash);
    sfxList[SFX_COUNT - 1].scope              = SCOPE_GLOBAL;
    sfxList[SFX_COUNT - 1].maxConcurrentPlays = 1;
    sfxList[SFX_COUNT - 1].length             = MIX_BUFFER_SIZE;
    AllocateStorage((void **)&sfxList[SFX_COUNT - 1].buffer, MIX_BUFFER_SIZE * sizeof(SAMPLE_FORMAT), DATASET_MUS, false);

    initializedAudioChannels = true;
}

void AudioDevice::AudioCallback(void *data, short *stream, int32 len)
{
    (void)data; // Unused	
	
    int req = w_get_requested_sample_count();
	
//    PrintLog(PRINT_NORMAL, "req = %d\n", req);
    
    if (req == 0)
	    return;
	
	
    len *= 2;

    int L = len;
	
    if (!floatBuf)
    {
	    L = len + (len/2);
	    floatBuf = new float[L];
	    wordBuf = new short[L];
    }
    
    bzero(floatBuf, L * sizeof(float));

    ProcessAudioMixing(floatBuf, L);
    
    for (int x = 0; x < L; x++)
	wordBuf[x] = CLAMP(floatBuf[x], -0.5, 0.5) * 0xFFFF;
	    
    buf.insert(buf.end(), wordBuf, wordBuf + L);
	
    if (buf.size() > (len * 2)) {
	memcpy(wordBuf, buf.data(), len * sizeof(short));
	buf.erase(buf.begin(), buf.begin() + len);
    }
    else
	bzero(wordBuf, len * sizeof(short));

    if (sampleConvSize == -1)
	wssaudio_write(wordBuf, numSamples);
    else {
	Resample_s16(wordBuf, sampleConvBuf, 44100, 
	    wssSampleRate, numSamples, 2);
	wssaudio_write(sampleConvBuf, (sampleConvSize / 2) / 2);
    }
}
