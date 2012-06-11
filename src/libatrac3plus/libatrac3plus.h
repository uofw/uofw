typedef struct
{
    u16 channels; // 0
    u16 dataBlkSz; // 2
    u8 unk4[2]; // 4
    // padding
    u32 dataOff; // 8
    u32 factSz; // 12
    u32 dataSize; // 16
    u32 samplesPerChan; // 20
    int loopStart; // 24
    int loopEnd; // 28
} SceAtracFile; // size: 32

typedef struct
{
    u32 decodePos; // 0
    u32 endSample; // 4
    u32 loopStart; // 8
    u32 loopEnd; // 12
    int samplesPerChan; // 16
    char numFrame; // 20
    // 2: all the stream data on the buffer
    // 6: looping -> second buffer needed
    char state; // 21
    char unk22;
    char numChan; // 23
    u16 sampleSize; // 24
    u16 codec; // 26
    u32 dataOff; // 28
    u32 curOff; // 32
    u32 dataEnd; // 36
    int loopNum; // 40
    u32 streamDataByte; // 44
    u32 unk48;
    u32 unk52;
    u8 *buffer; // 56
    u8 *secondBuffer; // 60
    u32 bufferByte; // 64
    u32 secondBufferByte; // 68
    // ...
} SceAtracIdInfo;

typedef struct
{
    SceAudiocodecCodec codec __attribute__((aligned(256))); // size: 128
    SceAtracIdInfo info __attribute__((aligned(128))); // size: 128
} SceAtracId; // size: 256

typedef struct
{
    u8 *writePositionFirstBuf;
    u32 writableByteFirstBuf;
    u32 minWriteByteFirstBuf;
    u32 readPositionFirstBuf;
    u8 *writePositionSecondBuf;
    u32 writableByteSecondBuf;
    u32 minWriteByteSecondBuf;
    u32 readPositionSecondBuf;
} SceBufferInfo;

typedef struct
{
    u32 unk0;
    u16 unk4;
    u8 unk6, unk7;
    u32 dataOff; // 8
    u32 unk12;
    u32 dataSize; // 16
    u32 unk20, unk24, unk28;
    u32 unk32;
    u32 unk36;
    s32 unk40;
    s32 unk44;
} SceAA3File;

int setHalfwayBuffer(SceAtracId *id, u8 *buffer, u32 readByte, u32 bufferByte, SceAtracFile *info);
int setMOutHalfwayBuffer(SceAtracId *id, u8 *buffer, u32 readByte, u32 bufferByte, SceAtracFile *info);
int resetId(SceAtracId *info);
int allocEdram(void);
int openAA3AndGetID(u8 *buffer, u32 readByte, int arg2, SceAtracFile *info);
int isValidState(char state);
int getOutputChan(SceAtracIdInfo *info, SceAudiocodecCodec *codec);
int decodeFrame(SceAtracId *id, short *outAddr, u32 *samples, u32 *finishFlag);
void sub_1C54(SceAtracIdInfo *info);
int loadWaveFile(u32 size, SceAtracFile *info, u8 *in);
int readWaveData(u8 *in, u32 *curOff, int size);
int getOffFromSample(SceAtracIdInfo *info, u32 sample);
u32 getSecondBufPos(SceAtracIdInfo *info, u32 arg1);
int getFrameFromSample(SceAtracIdInfo *info, u32 sample);
int getNextSample(SceAtracIdInfo *info);
int setBuffer(SceAtracId *id, u8 *buffer, u32 readByte, u32 bufferByte, SceAtracFile *info);
int initDecoder(SceAtracId *info, void *arg1);
int initAT3Decoder(SceAudiocodecCodec *codec, void *arg1);
int initAT3plusDecoder(SceAudiocodecCodec *codec, void *arg1);
int isValidOpt(int arg0, int arg1);
void setBufferInfo(SceAtracIdInfo *info, u32 factSize, u32 dataSize, u32 samplesPerChan, int loopStart, int loopEnd);
int setBufferSize(SceAtracIdInfo *info, u32 readByte, u32 bufferByte);
int getRemainFrame(SceAtracIdInfo *info);
u8 *getWritePtr(SceAtracIdInfo *info);
u32 getReadPosition(SceAtracIdInfo *info);
u32 getWritableByte(SceAtracIdInfo *info);
int sub_2DB8(SceAtracIdInfo *info);
int sub_2DF8(SceAtracIdInfo *info);
int sub_2EA4(SceAtracIdInfo *info);
int sub_2FA8(u32 arg0, u32 arg1, u32 arg2, u32 arg3);
u32 sub_2FEC(u32 arg0, u32 arg1, u32 arg2, u32 arg3);
u32 sub_3004(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4);
u32 sub_3048(SceAtracIdInfo *info);
u32 sub_30C0(SceAtracIdInfo *info);
u32 sub_30E0(SceAtracIdInfo *info);
u32 sub_316C(u32 arg0, u32 arg1, u32 arg2, u32 arg3);
u32 sub_31B4(SceAtracIdInfo *info);
u32 sub_3230(u32 arg0, u32 arg1, u32 arg2);
void copyBuffer(SceAtracIdInfo *info, u32 bufferByte);
void getBufferInfo(SceAtracIdInfo *info, u32 sample, SceBufferInfo *bufferInfo);
int resetPlayPos(SceAtracIdInfo *info, u32 sample, u32 writeByteFirstBuf, u32 writeByteSecondBuf);
int parseAA3(u32 readByte, SceAA3File *aa3, int arg2, u8 *buffer);
int setAtracFileInfo(SceAA3File *aa3, SceAtracFile *info);
int sub_3A18(u8 *buffer, u32 *curOff, u32 arg2);
short sub_3AA0(u8 *buffer, u32 *curOff, u32 readByte);
int sub_3B14(u8 *buffer, u32 *curOff, u32 readByte);
int sub_3B54(u8 *buffer, u32 *curOff, u32 readByte);

