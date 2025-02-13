
#ifndef CONFIG_H
#define CONFIG_H

// option for debug level.
#define OPTION_LOG_LEVEL_CLOSE      0
#define OPTION_LOG_LEVEL_ERROR      1
#define OPTION_LOG_LEVEL_WARNING    2
#define OPTION_LOG_LEVEL_DEFAULT    3
#define OPTION_LOG_LEVEL_DETAIL     4

// option for hls seek.
#define OPTION_HLS_SEEK_IN_SEGMENT  1
#define OPTION_HLS_NOT_SEEK_IN_SEGMENT 0

#ifndef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_WARNING
#endif

// <CONFIG_HLS_SEEK=1>: seek to the right request point, it may be slower a little.
// <CONFIG_HLS_SEEK=0>: seek to the beginning of this segment, it is faster a little, but not much accurat.
#define CONFIG_HLS_SEEK    OPTION_HLS_NOT_SEEK_IN_SEGMENT

// When hls live tv download ts slice failed, we will try to reconnect http server
// from the last read point. But the reconnection may fail with http 404 error.
// Then we will try to reconnect the same server with 30s timeout. If this
// reconnection is also timeout and (CONFIG_HLS_TRY_NEXT_TS == 1), we try to
// request next ts slice. (for yst icntv)
// If CONFIG_HLS_TRY_NEXT_TS == 0, after 30s timeout, we will return IO ERROR.
#define CONFIG_HLS_TRY_NEXT_TS     0

#define CONFIG_ENABLE_DEMUX_ASF         1
#define CONFIG_ENABLE_DEMUX_AVI         1
#define CONFIG_ENABLE_DEMUX_BLUERAYDISK 1
#define CONFIG_ENABLE_DEMUX_MPEGDASH    1
#define CONFIG_ENABLE_DEMUX_FLV         1
#define CONFIG_ENABLE_DEMUX_HLS         1
#define CONFIG_ENABLE_DEMUX_MKV         1
#define CONFIG_ENABLE_DEMUX_MMS         1
#define CONFIG_ENABLE_DEMUX_MOV         1
#define CONFIG_ENABLE_DEMUX_MPG         1
#define CONFIG_ENABLE_DEMUX_PMP         1
#define CONFIG_ENABLE_DEMUX_OGG         1
#define CONFIG_ENABLE_DEMUX_RX          1
#define CONFIG_ENABLE_DEMUX_TS          1
#define CONFIG_ENABLE_DEMUX_M3U9        1
#define CONFIG_ENABLE_DEMUX_PLAYLIST    1
#define CONFIG_ENABLE_DEMUX_APE         1
#define CONFIG_ENABLE_DEMUX_FLAC        1
#define CONFIG_ENABLE_DEMUX_AMR         1
#define CONFIG_ENABLE_DEMUX_ATRAC       1
#define CONFIG_ENABLE_DEMUX_MP3         1
#define CONFIG_ENABLE_DEMUX_DTS         1
#define CONFIG_ENABLE_DEMUX_AC3         1
#define CONFIG_ENABLE_DEMUX_AAC         1
#define CONFIG_ENABLE_DEMUX_WAV         1
#define CONFIG_ENABLE_DEMUX_REMUX       1
#define CONFIG_ENABLE_DEMUX_WVM         1
#define CONFIG_ENABLE_DEMUX_MMSHTTP     1
#define CONFIG_ENABLE_DEMUX_AWTS        1
#define CONFIG_ENABLE_DEMUX_SSTR        1
#define CONFIG_ENABLE_DEMUX_CAF         1


/*************** frame buffer config start ***********************/
#define NUM_OF_PICTURES_KEEP_IN_LIST 3
#define NUM_OF_PICTURES_KEEPPED_BY_DEINTERLACE 2
#define NUM_OF_PICTURES_KEEPPED_BY_ROTATE 0
#define NUM_OF_PICTURES_FOR_EXTRA_SMOOTH 1
/*************** frame buffer config end *************************/

/*************** for debug start ***********************/
#define ENABLE_SPECIAL_PARSER 0
/*************** for debug end *************************/


#endif
