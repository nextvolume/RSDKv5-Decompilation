

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

static unsigned int bufAddr;
static unsigned int bufSize;
static unsigned int curBufNumAddr;
static unsigned int sampleRate;

static int playingBuf;

static bool dossound_init(void)
{
    __dpmi_regs i;
    bool r;

    bzero(&i, sizeof(__dpmi_regs));

// Check if the interrupt vector for DOSSound is hooked up.
// If it isn't, it means that DOSSound was not installed.

    i.h.ah = 0x35;
    i.h.al = DOSSOUND_INT;

    __dpmi_int(0x21, &i);

    r = i.x.es || i.x.bx;

    if (!r) {
        PrintLog(PRINT_NORMAL, "Audio not enabled because DOSSound was not installed!");
    } else {
	bzero(&i, sizeof(__dpmi_regs));

        i.h.ah = 0x40; // Return Vendor/Device

        __dpmi_int(DOSSOUND_INT, &i);

        PrintLog(PRINT_NORMAL, "Vendor ID = 0x%04x", i.x.bx);
        PrintLog(PRINT_NORMAL, "Device ID = 0x%04x", i.x.cx);

        i.h.ah = 0x50; // Return segments of internal buffers

        __dpmi_int(DOSSOUND_INT, &i);

	bufAddr = i.x.bx << 4;
	bufSize = i.x.cx << 4;

        PrintLog(PRINT_NORMAL, "Buffer address = 0x%05x", bufAddr);
        PrintLog(PRINT_NORMAL, "Size = %d", bufSize);

        i.h.ah = 0x03; // Query Status

        __dpmi_int(DOSSOUND_INT, &i);

	curBufNumAddr = (i.x.es << 4) + i.x.di + 11;
        sampleRate = 44100;

        PrintLog(PRINT_NORMAL,
	"curBufNumAddr address = 0x%05x", 
	curBufNumAddr);

        i.h.ah = 0x10; // Set Volume
        i.h.bh = 0; // volume for left (max)
        i.h.bl = 0; // volume for right (max)

        __dpmi_int(DOSSOUND_INT, &i);

        i.h.ah = 0x51; // Play PCM data
        i.x.bx = sampleRate;

        __dpmi_int(DOSSOUND_INT, &i);

       //musInfo.stream = new Sint16[musInfo.bufSize / 2];
    }
    
    return r;
}

static int dossound_get_buffer_number(void)
{
	unsigned char c;

	_dosmemgetb(curBufNumAddr, 1, &c);

	return c;
}

static bool dossound_buffer_number_changed(int *bufNum) {
	static int old = -1;

	int n = dossound_get_buffer_number();
	bool r = (old != n);

	old = n;
	*bufNum = n;
	return r;
}

static void dossound_write_to_buffer(int bufNum, void *data) {
	dosmemput(data, bufSize, bufAddr + (bufNum * bufSize));
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
    
    if (!dossound_init()) {
        PrintLog(PRINT_NORMAL, "Unable to open audio device");
        audioState = false;
        return true;
    }

    audioState = true;

    numSamples = MIX_BUFFER_SIZE / AUDIO_CHANNELS;


    if (wssSampleRate != 44100) {
	sampleConvSize = Resample_s16(NULL, NULL, 44100, 
	    wssSampleRate, numSamples, 2) * 2 * 2;
	sampleConvBuf = new short[sampleConvSize / sizeof(short)];
    }
    else
        sampleConvSize = -1;
    
    PrintLog(PRINT_NORMAL, "Initialized DOSSound audio.");
    
    return true;
}

void AudioDevice::Release()
{
    delete floatBuf;
    delete wordBuf;
    delete sampleConvBuf;
	
/*    if (audioState) {
	__dpmi_regs i;

        i.h.ah = 0x21; // stop

        __dpmi_int(DOSSOUND_INT, &i);
    }*/
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
	
    if (!dossound_buffer_number_changed(&playingBuf))
	return;
	
    len *= 4;

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

/*    if (sampleConvSize == -1)
	wssaudio_write(wordBuf, numSamples);
    else {
	Resample_s16(wordBuf, sampleConvBuf, 44100, 
	    wssSampleRate, numSamples, 2);
	wssaudio_write(sampleConvBuf, (sampleConvSize / 2) / 2);
    }*/
    
    dossound_write_to_buffer(!playingBuf, wordBuf);
}
