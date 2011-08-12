// GIPS_common_types.h

#ifndef GIPS_COMMON_TYPES_H
#define GIPS_COMMON_TYPES_H

#ifdef GIPS_WCECHAR
    #define GIPS_CHAR TCHAR     // Windows CE is UNICODE based (used in VE PPC)
#else
    #define GIPS_CHAR char
#endif

#ifndef NULL
    #define NULL    0
#endif

#define FILE_FORMAT_PCM_FILE 0
#define FILE_FORMAT_WAV_FILE 1
#define FILE_FORMAT_COMPRESSED_FILE 2
#define FILE_FORMAT_AVI_FILE 3
#define FILE_FORMAT_PREENCODED_FILE 4

// GIPS_encryption
// This is a class that should be overloaded to enable encryption

class GIPS_encryption
{
public:
    virtual void encrypt(int channel_no, unsigned char * in_data, 
        unsigned char * out_data, int bytes_in, int * bytes_out) = 0;

    virtual void decrypt(int channel_no, unsigned char * in_data, 
        unsigned char * out_data, int bytes_in, int * bytes_out) = 0;

    virtual void encrypt_rtcp(int channel_no, unsigned char * in_data, 
        unsigned char * out_data, int bytes_in, int * bytes_out) = 0;

    virtual void decrypt_rtcp(int channel_no, unsigned char * in_data, 
        unsigned char * out_data, int bytes_in, int * bytes_out) = 0;
    virtual ~GIPS_encryption() {}
protected:
   GIPS_encryption() {} 
};

// External transport protocol
// This is a class that should be implemented by the customer IF
// a different transport protocol than IP/UDP/RTP is wanted. The
// standard data transport protocol used by VoiceEngine is IP/UDP/RTP
// according to the SIP-standard.
class GIPS_transport
{
public:
    virtual int SendPacket(int channel, const void *data, int len) = 0;
    virtual int SendRTCPPacket(int channel, const void *data, int len) = 0;
    virtual ~GIPS_transport() {}
protected:
   GIPS_transport() {}
};

// Depricated, will be removed
enum GIPS_TraceFilter
{
    TR_NONE		    = 0x0000,	// no trace
    TR_STATE_INFO   = 0x0001,   
    TR_WARNING	    = 0x0002,
    TR_ERROR	    = 0x0004,
    TR_CRITICAL	    = 0x0008,
    TR_APICALL	    = 0x0010,	
    TR_MEMORY	= 0x0100,		// memory info
    TR_TIMER	= 0x0200,		// timing info
    TR_STREAM	= 0x0400,		// "continuous" stream of data
	

    // everything bellow will be encrypted
    // used for GIPS debug purposes
    TR_DEBUG	= 0x0800,		// debug
    TR_INFO	    = 0x1000,		// debug info 
    TR_CUSTOMER = 0x2000,		// customer debug info 

    TR_ALL      = 0xffff
};


namespace GIPS
{
    enum TraceModule
    {
        TR_ENGINE             = 0x0000,	// no module, old engine 
        TR_VOICE              = 0x0001,	// not a module
        TR_VIDEO              = 0x0002,	// not a module
        TR_UTILITY            = 0x0003,	// not a module
        TR_RTP_RTCP           = 0x0004,	
        TR_TRANSPORT          = 0x0005,	
        TR_SRTP               = 0x0006,	
        TR_AUDIO_CODING       = 0x0007,	
        TR_AUDIO_MIXER_SERVER = 0x0008,	
        TR_AUDIO_MIXER_CLIENT = 0x0009,
        TR_FILE               = 0x000a,
        TR_VQE                = 0x000b,
        TR_VIDEO_CODING       = 0x0010,	
        TR_VIDEO_MIXER        = 0x0011,
        TR_AUDIO_DEVICE       = 0x0012,
    };

    enum TraceLevel
    {
        TR_NONE		     = 0x0000,	// no trace
        TR_STATE_INFO    = 0x0001,   
        TR_WARNING	     = 0x0002,
        TR_ERROR	     = 0x0004,
        TR_CRITICAL	     = 0x0008,
        TR_APICALL	     = 0x0010,
        TR_DEFAULT       = 0x00ff,

        TR_MODULE_CALL   = 0x0020,	
        TR_MEMORY		 = 0x0100,		// memory info
        TR_TIMER		 = 0x0200,		// timing info
        TR_STREAM		 = 0x0400,		// "continuous" stream of data
    
        // everything bellow will be encrypted
        // used for GIPS debug purposes
        TR_DEBUG	= 0x0800,		// debug
        TR_INFO	    = 0x1000,		// debug info 
        TR_CUSTOMER = 0x2000,		// customer debug info 

        TR_ALL      = 0xffff
    };

    // Corresponds to GIPS platform-independent types.
    enum Type
    {
        TYPE_Word8,
        TYPE_UWord8,
        TYPE_Word16,
        TYPE_UWord16,
        TYPE_Word32,
        TYPE_UWord32,
        TYPE_Word64,
        TYPE_UWord64,
        TYPE_Float32,
        TYPE_Float64
    };
};

// External Trace API
class GIPSTraceCallback
{
public:
    virtual void Print(const GIPS::TraceLevel level, 
                       const char *traceString, 
                       const int length) = 0;
    virtual ~GIPSTraceCallback() {}
protected:
   GIPSTraceCallback() {}
};


//////////////////////////////////////////////////////////////////////
// GIPS_CodecInst
//
// Each codec supported by a GIPS Engine can be described by
// this structure.
//
// The following codecs are always supported:
//
// - G.711 u-Law
// - G.711 a-Law
// - GIPS Enhanced G.711 u-Law
// - GIPS Enhanced G.711 A-Law
// - GIPS iPCM-wb
// - GIPS iLBC
// - GIPS iSAC
// - GIPS iSAC_LC
//
//  Other standard codecs are available upon request
//
// Note that the GIPS NetEQ is included on the receiving side for all
// codecs. NetEQ is a patented GIPS technology that adaptively compen-,
// sates for jitter, and at the same time conceals errors due to lost
// packets. NetEQ delivers improvements in sound quality while mini-
// mizing buffering latency.
//////////////////////////////////////////////////////////////////////

struct GIPS_CodecInst
{
    int pltype;
    GIPS_CHAR plname[32];
    int plfreq;
    int pacsize;
    int channels;
    int rate;
};


// External read and write functions
// This is a class that should be implemented by the customer IF
// the functions GIPS***_ConvertWavToPCM() or GIPS***_PlayPCM() are used

class  InStream 
{
public:
    virtual int Read(void *buf,int len) = 0;
    // len - size in bytes that should be read
    // returns the size in bytes read (=len before end and =[0..len-1] at end similar to fread)

    // Called when a wav-file needs to be rewinded
    virtual int Rewind() {return -1;}

    // Destructor
    virtual ~InStream() {};
};

class  OutStream 
{
public:
    virtual bool Write(const void *buf,int len) = 0;
    // true for ok, false for fail

    // Only internal usage
    virtual int Rewind() {return -1;}

    // Destructor
    virtual ~OutStream() {};
};


#endif

