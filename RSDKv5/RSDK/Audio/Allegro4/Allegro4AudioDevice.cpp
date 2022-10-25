

uint8 AudioDevice::contextInitialized = false;
AUDIOSTREAM *AudioDevice::stream;
int AudioDevice::numSamples;
std::vector<short> AudioDevice::buf;
float *AudioDevice::floatBuf = NULL;
short *AudioDevice::wordBuf = NULL;

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
    }
    
    if (install_sound(DIGI_AUTODETECT, 0, NULL) == -1) {
        PrintLog(PRINT_NORMAL, "Unable to open audio device: %s", allegro_error);
        audioState = false;
        return true;
    }

    audioState = true;
    numSamples = MIX_BUFFER_SIZE / AUDIO_CHANNELS;
    
    
    stream = play_audio_stream(numSamples, 16, 1, AUDIO_FREQUENCY, 
	255, 127);
    
    PrintLog(PRINT_NORMAL, "Initialized Allegro audio.");
    
    return true;
}

void AudioDevice::Release()
{
    delete floatBuf;
    delete wordBuf;

    stop_audio_stream(stream);
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
	memcpy(stream, buf.data(), len * sizeof(short));
	buf.erase(buf.begin(), buf.begin() + len);
    }
    else
	bzero(stream, len * sizeof(short));

// Allegro wants unsigned samples    
    for(int x = 0; x < len; x++)
        stream[x] ^= 0x8000;
}
