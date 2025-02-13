#ifndef CDX_PARSER_H
#define CDX_PARSER_H

#include <errno.h>
#include <stdint.h>
#include <CdxTypes.h>
#include <CdxLog.h>
#include <CdxStream.h>
#include <vdecoder.h>
#include <adecoder.h>
#include <sdecoder.h>

enum CdxParserTypeE
{
    CDX_PARSER_UNKNOW = -1,
    CDX_PARSER_MOV,
    CDX_PARSER_MKV,
    CDX_PARSER_ASF,
    CDX_PARSER_TS,
    CDX_PARSER_AVI,
    CDX_PARSER_FLV,
    CDX_PARSER_PMP,
    
    CDX_PARSER_HLS,
    CDX_PARSER_DASH,
    CDX_PARSER_MMS,
    CDX_PARSER_BD,
    CDX_PARSER_OGG,
    CDX_PARSER_M3U9,
    CDX_PARSER_RMVB,
    CDX_PARSER_PLAYLIST,
    CDX_PARSER_APE,
    CDX_PARSER_FLAC,
    CDX_PARSER_AMR,
    CDX_PARSER_ATRAC,
    CDX_PARSER_MP3,
    CDX_PARSER_DTS,
    CDX_PARSER_AC3,
    CDX_PARSER_AAC,
    CDX_PARSER_WAV,	
    CDX_PARSER_REMUX, /* rtsp, etc... */
    CDX_PARSER_WVM,
    CDX_PARSER_MPG,
    CDX_PARSER_MMSHTTP,
    CDX_PARSER_AWTS,
    CDX_PARSER_SSTR,
    CDX_PARSER_CAF,
    CDX_PARSER_G729,
    CDX_PARSER_DSD,
    CDX_PARSER_ID3,
    CDX_PARSER_ENV,
    CDX_PARSER_SSTR_PLAYREADY,
    CDX_PARSER_AWRAWSTREAM,
    CDX_PARSER_AWSPECIALSTREAM,
};

typedef struct CdxPacketS CdxPacketT;
typedef struct CdxMediaInfoS CdxMediaInfoT;
typedef enum CdxParserTypeE CdxParserTypeT;
typedef struct CdxParserCreatorS CdxParserCreatorT;
typedef struct CdxParserS CdxParserT;
//typedef struct ParserMetaDataS ParserMetaDataT;


#define MINOR_STREAM    0x0001 /*0 major stream, 1 minor stream*/
#define FIRST_PART      0x0002
#define LAST_PART       0x0004
#define KEY_FRAME       0x0008

struct CdxPacketS
{
    cdx_void *buf;
    cdx_void *ringBuf;
    cdx_int32 buflen;
    cdx_int32 ringBufLen;
    int64_t pts;   
    cdx_int64 duration;
    cdx_int32 type;
    cdx_int32 length;
    cdx_uint32 flags; /* MINOR_STREAM, FIRST_PART, LAST_PART, etc... */
    cdx_int32 streamIndex;
    cdx_int64 pcr;
    cdx_int32 infoVersion;
    void *info;//VideoInfo/AudioInfo/SubtitleInfo
};

//#define PARSER_METADATA_ORGINAL_URI     "parser.orginalUri"
//#define PARSER_METADATA_REDIRECT_URI    "parser.redirectUri"
//#define PARSER_METADATA_ACCESSIBLE_URI  "parser.accessibleUri"

struct ParserCacheStateS
{
    cdx_int32 nCacheCapacity;
    cdx_int32 nCacheSize;
    cdx_int32 nBandwidthKbps;
    cdx_int32 nPercentage; /* current_caching_positioin / duration */
};

struct StreamSeekPos
{
    cdx_int64 pos;
    cdx_int64 time;
    cdx_int64 startTime;
};

enum EPARSERNOTIFY  //* notify.
{
    PARSER_NOTIFY_VIDEO_STREAM_CHANGE = 0x100,
    PARSER_NOTIFY_AUDIO_STREAM_CHANGE,
};


enum CdxParserCommandE
{
    CDX_PSR_CMD_SWITCH_AUDIO,
    CDX_PSR_CMD_SWITCH_SUBTITLE,

    CDX_PSR_CMD_DISABLE_AUDIO,
    CDX_PSR_CMD_DISABLE_VIDEO,
    CDX_PSR_CMD_DISABLE_SUBTITLE,

    CDX_PSR_CMD_SET_DURATION,
    CDX_PSR_CMD_REPLACE_STREAM,
	CDX_PSR_CMD_SET_LASTSEGMENT_FLAG,
    CDX_PSR_CMD_CLR_INFO,
    
    CDX_PSR_CMD_STREAM_SEEK,
    CDX_PSR_CMD_GET_CACHESTATE,

    CDX_PSR_CMD_SET_FORCESTOP,
    CDX_PSR_CMD_CLR_FORCESTOP,
    CDX_PSR_CMD_UPDATE_PARSER,
    CDX_PSR_CMD_SET_HDCP,
    CDX_PSR_CMD_SET_SECURE_BUFFER_COUNT,
    CDX_PSR_CMD_SET_SECURE_BUFFERS,
	CDX_PSR_CMD_GET_STREAM_EXTRADATA,

    // get the shiftTimeUrl for cmcc livemode1&2 seekTo
    CDX_PSR_CMD_GET_REDIRECT_URL,

    // for cmcc get the stream url in LogRecorder
    CDX_PSR_CMD_GET_URL,
	// get the statusCode of http
    CDX_PSR_CMD_GET_STREAM_STATUSCODE,
	// for Ali YUNOS get TS info
	CDX_PSR_CMD_GET_TS_M3U_BANDWIDTH,
	CDX_PSR_CMD_GET_TS_SEQ_NUM,
	CDX_PSR_CMD_GET_TS_LENGTH,
	CDX_PSR_CMD_GET_TS_DURATION,
	CDX_PSR_CMD_SET_YUNOS_UUID,

    // parser and stream use the same setCallback cmd, 
    // the code below must be the end of  this structure
	CDX_PSR_CMD_SET_CALLBACK = STREAM_CMD_SET_CALLBACK,
};

enum CdxParserStatusE
{
    PSR_OK,
    PSR_INVALID,
    PSR_OPEN_FAIL,
    PSR_IO_ERR,
    PSR_USER_CANCEL,
    PSR_INVALID_OPERATION,
    PSR_UNKNOWN_ERR,
    PSR_EOS,
    /*other error info*/
};

#define AUDIO_STREAM_LIMIT 32
#define VIDEO_STREAM_LIMIT 1
#define SUBTITLE_STREAM_LIMIT 32


struct VideoInfo
{
    cdx_atomic_t ref;
    cdx_int32 videoNum;
    VideoStreamInfo    video[VIDEO_STREAM_LIMIT];
};
struct AudioInfo
{
    cdx_atomic_t ref;
    cdx_int32 audioNum;
    AudioStreamInfo    audio[AUDIO_STREAM_LIMIT];
};
struct SubtitleInfo
{
    cdx_atomic_t ref;
    cdx_int32 subtitleNum;
    SubtitleStreamInfo subtitle[SUBTITLE_STREAM_LIMIT];
};


struct CdxProgramS
{
    cdx_int32 id;
    cdx_uint32 flags;
    cdx_uint32 duration; //unit: ms
    cdx_int32 audioNum, audioIndex;
    cdx_int32 videoNum, videoIndex;
    cdx_int32 subtitleNum, subtitleIndex;
	cdx_int64 firstPts;
    
    AudioStreamInfo    audio[AUDIO_STREAM_LIMIT];
    VideoStreamInfo    video[VIDEO_STREAM_LIMIT];
    SubtitleStreamInfo subtitle[SUBTITLE_STREAM_LIMIT];
};

#define PROGRAM_LIMIT 1 //no switch program interface now, so limit 1

struct CdxMediaInfoS
{
    cdx_int64 fileSize;
    cdx_uint32 bitrate;
    cdx_bool bSeekable;
    cdx_int32 programNum, programIndex;
    
    struct CdxProgramS program[PROGRAM_LIMIT];
    
    cdx_void *privData;

    cdx_uint8   album[64];
	cdx_int32   albumsz;
	cdx_int32	albumCharEncode;

	cdx_uint8   author[64];
	cdx_int32   authorsz;
	cdx_int32   authorCharEncode;

	cdx_uint8   genre[64];
	cdx_int32   genresz;
	cdx_int32   genreCharEncode;
	
    cdx_uint8   title[64];
	cdx_int32   titlesz;
	cdx_int32   titleCharEncode;

	cdx_uint8   year[64];
	cdx_int32   yearsz;
	cdx_int32   yearCharEncode;
	
    cdx_uint8   composer[64];
    cdx_uint8   date[64];
    cdx_uint8   artist[64];	
    cdx_uint8   writer[64];    
    cdx_uint8   albumArtist[64];
    cdx_uint8   compilation[64];   
    cdx_uint8   location[64];
    cdx_uint8   rotate[4];
    cdx_int32   discNumber;
	cdx_uint8   *pAlbumArtBuf;
	cdx_int32	nAlbumArtBufSize;
};

#define MUTIL_AUDIO         0x0001U /*will disable switch audio*/
//#define MUTIL_VIDEO         0x0002U /*will disable switch video*/
#define MUTIL_SUBTITLE      0x0004U /*will disable switch subtitle*/

#define DISABLE_AUDIO       0x0008U
#define DISABLE_VIDEO       0x0010U
#define DISABLE_SUBTITLE    0x0020U
#define NO_NEED_DURATION    0x0040U

#define BD_BASE             0x0080U
#define BD_DEPENDENT        0x0100U
#define BD_TXET             0x0180U

// for DASH
#define SEGMENT_MP4         0x0200U

// for mms playlist
#define NO_NEED_CLOSE_STREAM 0x0400U
// for miracast
#define MIRACST             0x0800U

// for smooth streaming
#define SEGMENT_SMS         0x1000U

// for cmcc timeShift
#define CMCC_TIME_SHIFT     0x2000U

#define NOT_LASTSEGMENT     0x4000U

// for playready sstr
#define SEGMENT_PLAYREADY   0x8000U

#define MutilAudioStream(flags) (!!(flags & MUTIL_AUDIO))

typedef struct HdcpOpsS HdcpOps;
struct HdcpOpsS
{
	int (*init)(void **);
    void (*deinit)(void *);
    cdx_uint32 (*decrypt)(void *handle, const cdx_uint8 privateData[16], const cdx_uint8 *in, cdx_uint8 *out, cdx_uint32 len, int streamType);
};
typedef struct DownloadObjectS DownloadObject;
struct DownloadObjectS
{
	int seqNum;
    char *uri;
    int statusCode; //http statusCode
    cdx_int64 seqSize;    //segment size, byte
    cdx_int64 seqDuration;//segment duration, us
    cdx_int64 spendTime;  //ms
    cdx_int64 rate;       //bps
};

struct CdxParserCreatorS
{
	CdxParserT *(*create)(CdxStreamT *, cdx_uint32 /*flags*/);
	cdx_uint32 (*probe)(CdxStreamProbeDataT *);/*return score(0-100)*/
};

struct CdxParserOpsS
{
	cdx_int32 (*control)(CdxParserT *, cdx_int32 /* cmd */, void * /* param */);
	
	cdx_int32 (*prefetch)(CdxParserT *, CdxPacketT * /* pkt */);
	
	cdx_int32 (*read)(CdxParserT *, CdxPacketT * /* pkt */);
	
	cdx_int32 (*getMediaInfo)(CdxParserT *, CdxMediaInfoT * /* MediaInfo */);

	cdx_int32 (*seekTo)(CdxParserT *, cdx_int64 /* timeUs */);

    cdx_uint32 (*attribute)(CdxParserT *); /*return falgs define as open's falgs*/

    cdx_int32 (*getStatus)(CdxParserT *); /*return enum CdxPrserStatusE*/
    
	cdx_int32 (*close)(CdxParserT *);

    cdx_int32 (*init)(CdxParserT *);
};

struct CdxParserS
{
    enum CdxParserTypeE type;
    struct CdxParserOpsS *ops;
};

struct ParserUriKeyInfoS
{
    const char *comment;
    const char *scheme[10];
    const char *suffix[10];
    const char *attr[10];
};

#ifdef __cplusplus
extern "C"
{
#endif

int AwParserRegister(CdxParserCreatorT *creator, CdxParserTypeT type, 
                    struct ParserUriKeyInfoS *keyInfo);

int CdxParserPrepare(CdxDataSourceT *source, cdx_uint32 flags, pthread_mutex_t *mutex, cdx_bool *exit,
    CdxParserT **parser, CdxStreamT **stream, ContorlTask *parserTasks, ContorlTask *streamTasks);
CdxParserT *CdxParserCreate(CdxStreamT *, cdx_uint32 /*flags*/);
int CdxParserOpen(CdxStreamT *stream, cdx_uint32 flags, pthread_mutex_t *mutex, cdx_bool *exit,
    CdxParserT **parser, ContorlTask *parserTasks);
#define CdxParserForceStop(parser) \
    (CdxParserControl(parser, CDX_PSR_CMD_SET_FORCESTOP, NULL))

#define CdxParserClrForceStop(parser) \
    (CdxParserControl(parser, CDX_PSR_CMD_CLR_FORCESTOP, NULL))
    
static inline cdx_int32 CdxParserControl(CdxParserT *parser, cdx_int32 cmd, void *param)
{
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->control);
    return parser->ops->control(parser, cmd, param);
}

static inline cdx_int32 CdxParserPrefetch(CdxParserT *parser, CdxPacketT *pkt)
{
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->prefetch);
    return parser->ops->prefetch(parser, pkt);
}

static inline cdx_int32 CdxParserRead(CdxParserT *parser, CdxPacketT *pkt)
{
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->read);
    return parser->ops->read(parser, pkt);
}

static void PrintMediaInfo(CdxMediaInfoT *mediaInfo)
{
    CDX_LOGD("*********PrintMediaInfo begin*********");
    struct CdxProgramS *program = &mediaInfo->program[0];
	
	CDX_LOGD("fileSize = %lld, "
			"bSeekable = %d, "
			"duration = %d, "
			"audioNum = %d, "
			"videoNum = %d, "
			"subtitleNum = %d ",
			mediaInfo->fileSize,
			mediaInfo->bSeekable,
			program->duration,
			program->audioNum,
			program->videoNum,
			program->subtitleNum);
    int i;
    for (i = 0; i < VIDEO_STREAM_LIMIT && i < program->videoNum; i++)
    {
        VideoStreamInfo *video = program->video + i;
		CDX_LOGD("***Video[%d]*** "
				"eCodecFormat = 0x%x, "
				"nWidth = %d, "
				"nHeight = %d, "
				"nFrameRate = %d, "
				"nFrameDuration = %d, "
				"bIs3DStream = %d ",
				i,
				video->eCodecFormat,
				video->nWidth,
				video->nHeight,
				video->nFrameRate,
				video->nFrameDuration,
				video->bIs3DStream);

    }
    
    for (i = 0; i < AUDIO_STREAM_LIMIT && i < program->audioNum; i++)
    {
        AudioStreamInfo *audio = program->audio + i;
        CDX_LOGD("***Audio[%d]*** "
				"eCodecFormat = 0x%x, "
				"eSubCodecFormat = 0x%x, "
				"nChannelNum = %d, "
				"nBitsPerSample = %d, "
				"nSampleRate = %d ",
				i,
				audio->eCodecFormat,
				audio->eSubCodecFormat,
				audio->nChannelNum,
				audio->nBitsPerSample,
				audio->nSampleRate);

    }
    
    for (i = 0; i < SUBTITLE_STREAM_LIMIT && i < program->subtitleNum; i++)
    {
        SubtitleStreamInfo *subtitle = program->subtitle + i;

		CDX_LOGD("***Subtitle[%d]*** "
				"eCodecFormat = 0x%x, "
				"strLang = (%s) ",
				i,
				subtitle->eCodecFormat,
				subtitle->strLang);
    }

    CDX_LOGD("*********PrintMediaInfo end*********");
}

static inline cdx_int32 CdxParserGetMediaInfo(CdxParserT *parser, CdxMediaInfoT *mediaInfo)
{
    int ret;
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->getMediaInfo);
    ret = parser->ops->getMediaInfo(parser, mediaInfo);
    PrintMediaInfo(mediaInfo);
    return ret;
}

static inline cdx_int32 CdxParserSeekTo(CdxParserT *parser, cdx_int64 timeUs)
{
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->seekTo);
    return parser->ops->seekTo(parser, timeUs);
}

static inline cdx_uint32 CdxParserAttribute(CdxParserT *parser)
{
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->attribute);
    return parser->ops->attribute(parser);
}

static inline cdx_int32 CdxParserGetStatus(CdxParserT *parser)
{
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->getStatus);
    return parser->ops->getStatus(parser);
}

static inline cdx_int32 CdxParserClose(CdxParserT *parser)
{
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->close);
    return parser->ops->close(parser);
}

static inline cdx_int32 CdxParserInit(CdxParserT *parser)
{
    CDX_CHECK(parser);
    CDX_CHECK(parser->ops);
    CDX_CHECK(parser->ops->init);
    return parser->ops->init(parser);
}
enum
{
	PROBE_SPECIFIC_DATA_ERROR =-3,
	PROBE_SPECIFIC_DATA_NONE  =-2,
	PROBE_SPECIFIC_DATA_UNCOMPELETE = -1,
	PROBE_SPECIFIC_DATA_SUCCESS  = 1,
};
cdx_int32 ProbeVideoSpecificData(VideoStreamInfo* pVideoInfo, cdx_uint8* pDataBuf, 
    cdx_uint32 dataLen, cdx_uint32 eVideoCodecFormat, enum CdxParserTypeE type);
cdx_int32 probeH265RefPictureNumber(cdx_uint8* pDataBuf, cdx_uint32 nDataLen);

#ifdef __cplusplus
}
#endif

#endif

