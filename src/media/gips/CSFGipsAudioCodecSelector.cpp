/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _USE_CPVE

#include "CC_Common.h"
#include "CSFGipsLogging.h"

#include "CSFGipsAudioCodecSelector.h"
#include "GIPSVECodec.h"
#include "csf_common.h"

static const char* logTag = "AudioCodecSelector";

using namespace std;
using namespace CSF;

typedef struct _CSFCodecMapping
{
    AudioPayloadType csfAudioPayloadType;
    GipsAudioPayloadType gipsAudioPayloadType;
} CSFCodecMapping;

static CSFCodecMapping gipsMappingInfo[] =
{
    { AudioPayloadType_G711ALAW64K,       GipsAudioPayloadType_PCMA },
    { AudioPayloadType_G711ALAW56K,       GipsAudioPayloadType_PCMA },
    { AudioPayloadType_G711ULAW64K,       GipsAudioPayloadType_PCMU },
    { AudioPayloadType_G711ULAW56K,       GipsAudioPayloadType_PCMU },
    { AudioPayloadType_G729,              GipsAudioPayloadType_G729 },
    { AudioPayloadType_G729ANNEXA,        GipsAudioPayloadType_G729 },
    { AudioPayloadType_G729ANNEXB,        GipsAudioPayloadType_G729 },
    { AudioPayloadType_G729ANNEXAWANNEXB, GipsAudioPayloadType_G729 },
    { AudioPayloadType_G722_56K,          GipsAudioPayloadType_G722 },
    { AudioPayloadType_G722_64K,          GipsAudioPayloadType_G722 },
    { AudioPayloadType_G722_48K,          GipsAudioPayloadType_G722 },
    { AudioPayloadType_RFC2833,           GipsAudioPayloadType_TELEPHONE_EVENT }
};

CSFGipsAudioCodecSelector::CSFGipsAudioCodecSelector()
    : gipsCodec(NULL)
{
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::constructor" );
}

CSFGipsAudioCodecSelector::~CSFGipsAudioCodecSelector()
{
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::destructor" );
    release();
}

void CSFGipsAudioCodecSelector::release()
{
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::release" );

    // release the GIPS Codec sub-interface
    if (gipsCodec != NULL)
    {
        if (gipsCodec->Release() != 0)
        {
            LOG_GIPS_ERROR( logTag, "CSFGipsAudioCodecSelector::release gipsCodec->Release() failed" );
        }
        else
        {
            LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::release gipsCodec released" );
        }

        gipsCodec = NULL;

        std::map<int, GIPS_CodecInst*>::iterator iterGipsCodecs;
        for( iterGipsCodecs = codecMap.begin(); iterGipsCodecs != codecMap.end(); ++iterGipsCodecs ) {
            delete iterGipsCodecs->second;
        }

        codecMap.clear();
    }
}

int CSFGipsAudioCodecSelector::init( GIPSVoiceEngine* gipsVoice, bool useLowBandwidthCodecOnly, bool advertiseG722Codec )
{
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::init useLowBandwidthCodecOnly=%d, advertiseG722Codec=%d", useLowBandwidthCodecOnly, advertiseG722Codec );

    gipsCodec = GIPSVECodec::GetInterface( gipsVoice );

    if (gipsCodec == NULL)
    {
        LOG_GIPS_ERROR( logTag, "CSFGipsAudioCodecSelector::init cannot get reference to GIPS codec interface" );
        return -1;
    }

    // clear the existing codec map
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::init clearing map" );
    codecMap.clear();

    // get the number of codecs supported by GIPS
    int numOfSupportedCodecs = gipsCodec->GIPSVE_NumOfCodecs();

    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::init found %d supported codec(s)", numOfSupportedCodecs );

    // iterate over supported codecs
    for (int codecIndex = 0; codecIndex < numOfSupportedCodecs; codecIndex++)
    {
        GIPS_CodecInst supportedCodec;

        if (gipsCodec->GIPSVE_GetCodec(codecIndex, supportedCodec) == -1)
        {
            LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::init codecIndex=%d: cannot get supported codec information", codecIndex );
            continue;
        }

        LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::init codecIndex=%d: channels=%d, pacsize=%d, plfreq=%d, plname=%s, pltype=%d, rate=%d",
                codecIndex, supportedCodec.channels, supportedCodec.pacsize, supportedCodec.plfreq,
                supportedCodec.plname, supportedCodec.pltype, supportedCodec.rate );

        // iterate over the payload conversion table
        for (int i=0; i< (int) csf_countof(gipsMappingInfo); i++)
        {
            GipsAudioPayloadType gipsPayload = gipsMappingInfo[i].gipsAudioPayloadType;

            if (supportedCodec.pltype == gipsPayload)
            {
                bool addCodec = false;

                AudioPayloadType csfPayload = gipsMappingInfo[i].csfAudioPayloadType;

                switch (csfPayload)
                {
                case AudioPayloadType_G711ALAW64K:
                case AudioPayloadType_G711ULAW64K:
                    if (!useLowBandwidthCodecOnly)
                    {
                        addCodec = true;
                    }
                    break;

                case AudioPayloadType_G729:
                case AudioPayloadType_G729ANNEXA:
                case AudioPayloadType_G729ANNEXB:
                case AudioPayloadType_G729ANNEXAWANNEXB:
                    addCodec =  true;
                    break;

                case AudioPayloadType_G722_56K:
                case AudioPayloadType_G722_64K:
                    if (!useLowBandwidthCodecOnly &&
                        advertiseG722Codec)
                    {
                        addCodec =  true;
                    }
                    break;

                // iLBC and iSAC support is postponed
                //case AudioPayloadType_ILBC20:
                //case AudioPayloadType_ILBC30:
                //    addCodec =  true;
                //    break;
                //
                //case AudioPayloadType_ISAC:
                //    addCodec = true;
                //    break;

                case AudioPayloadType_RFC2833:
                    addCodec =  true;
                    break;

                case AudioPayloadType_G711ALAW56K:
                case AudioPayloadType_G711ULAW56K:
                case AudioPayloadType_G722_48K:
                case AudioPayloadType_ILBC20:
                case AudioPayloadType_ILBC30:
                case AudioPayloadType_ISAC:
                  break;
                    
                } // end of switch(csfPayload)

                if (addCodec)
                {
                    // add to codec map
                    GIPS_CodecInst* mappedCodec = new GIPS_CodecInst; // not sure when it should be deleted
                    memcpy(mappedCodec, &supportedCodec, sizeof(GIPS_CodecInst));

                    codecMap[csfPayload] = mappedCodec;

                    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::init added mapping payload %d to GIPS codec %s", csfPayload, mappedCodec->plname);
                }
                else
                {
                    LOG_GIPS_ERROR( logTag, "CSFGipsAudioCodecSelector::init no mapping found for GIPS codec %s (payload %d)", supportedCodec.plname, gipsPayload );
                }
            }
        } // end of iteration over the payload conversion table

    } // end of iteration over supported codecs

    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::init %d codec(s) added to map", (int)codecMap.size() );

    // return success
    return 0;
}

int  CSFGipsAudioCodecSelector::advertiseCodecs( CodecRequestType requestType )
{
    return AudioCodecMask_G711  |
           AudioCodecMask_G729A | AudioCodecMask_G729B |
           AudioCodecMask_G722;
}

int CSFGipsAudioCodecSelector::select( int payloadType, int dynamicPayloadType, int packPeriod, GIPS_CodecInst& selectedCodec )
{
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::select payloadType=%d, dynamicPayloadType=%d, packPeriod=%d", payloadType, dynamicPayloadType, packPeriod );

    // TO DO: calculate packet size ?
    // packPeriod "represents the number of milliseconds of audio encoded in a single packet" ?
    int packetSize = packPeriod;

    GIPS_CodecInst* supportedCodec = codecMap[payloadType];

    if (supportedCodec == NULL)
    {
        LOG_GIPS_ERROR( logTag, "CSFGipsAudioCodecSelector::select no GIPS codec found for payload %d", payloadType);
        return -1; // return failure
    }

    memcpy(&selectedCodec, supportedCodec, sizeof(GIPS_CodecInst));

    // adapt dynamic payload type if required
    if (dynamicPayloadType != -1)
    {
        selectedCodec.pltype = dynamicPayloadType;
        LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::select pltype = %d", selectedCodec.pltype);
    }

    // adapt packet size
    int pacsize;    // packet size in samples = sample rate * packet size

    switch ( payloadType )
    {
    case AudioPayloadType_G711ALAW64K:
    case AudioPayloadType_G711ULAW64K:
        // GIPS allowed packet sizes for G.711 u/a law: 10, 20, 30, 40, 50, 60 ms
        // (or 80, 160, 240, 320, 400, 480 samples)
        pacsize = ( 8 * packetSize );
        if (( pacsize == 80 ) || ( pacsize == 160 ) ||
            ( pacsize == 240) || ( pacsize == 320 ) ||
            ( pacsize == 400) || ( pacsize == 480 ))
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    case AudioPayloadType_ILBC20:
        // GIPS allowed packet sizes for iLBC: 20 30, 40, 60 ms
        pacsize = ( 8 * packetSize );
        if ( pacsize == 160 )
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    case AudioPayloadType_ILBC30:
        // GIPS allowed packet sizes for iLBC: 20 30, 40, 60 ms
        pacsize = ( 8 * packetSize );
        if ( pacsize == 240 )
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    case AudioPayloadType_G729:
    case AudioPayloadType_G729ANNEXA:
    case AudioPayloadType_G729ANNEXB:
    case AudioPayloadType_G729ANNEXAWANNEXB:
        // GIPS allowed packet sizes for G.729: 10, 20, 30, 40, 50, 60 ms
        // (or 80, 160, 240, 320, 400, 480 samples)
        pacsize = ( 8 * packetSize );
        if (( pacsize == 80 ) || ( pacsize == 160 ) ||
            ( pacsize == 240) || ( pacsize == 320 ) ||
            ( pacsize == 400) || ( pacsize == 480 ))
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    case AudioPayloadType_G722_56K:
    case AudioPayloadType_G722_64K:
        // GIPS allowed packet size for G.722: 20ms (or 320 samples).
        pacsize = ( 16 * packetSize );
        if ( pacsize == 320 )
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    default:
        break;
    }

    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::select found codec %s (payload=%d, packetSize=%d)",
            selectedCodec.plname, selectedCodec.pltype, selectedCodec.pacsize);

    // return success
    return 0;
}

int CSFGipsAudioCodecSelector::setSend(int channel, const GIPS_CodecInst& codec,int payloadType,bool vad)
{
    GIPS_PayloadFrequencies freq;
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::setSend channel=%d codec %s (payload=%d, packetSize=%d)",
            channel, codec.plname, codec.pltype, codec.pacsize);

    if (gipsCodec == NULL)
    {
        LOG_GIPS_ERROR( logTag, "CSFGipsAudioCodecSelector::setSend gipsCodec is null" );
        return -1;
    }
    bool vadEnable=vad;
    if ((payloadType== AudioPayloadType_G729) || (payloadType==AudioPayloadType_G729ANNEXA))
    {
        LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::setSend disable VAD for G729 or G729A on %d ", channel );
        vadEnable = false;
    }

    if (gipsCodec->GIPSVE_SetVADStatus( channel,  vadEnable,  VAD_CONVENTIONAL,false) != 0)
    {
        LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::GIPSVE_SetVADStatus cannot set VAD  to channel %d", channel );
        return -1;
    }
    
    // Check the codec's sampling  frequency and use appropriate enum 
    switch (codec.plfreq)
    {
        case SamplingFreq8000Hz:
            freq=FREQ_8000_HZ;
        break;
        case SamplingFreq16000Hz:
            freq=FREQ_16000_HZ;
            break;
#if GIPS_VER >= 3510    
    // GIPS 3.5 has  the FREQ_32000_HZ enum in GIPSVECodec.h whereas 3.4.8 does not.
    // Note the FREQ_32000_HZ enum is missing in the 3.5 docs 
    // This is only needed for ISAC which we don't support yet
    // I have logged a question with GIPS to see if it can be used with 3.4.8
        case SamplingFreq32000Hz:
            freq=FREQ_32000_HZ;
            break;            
#endif
        default:
            freq=FREQ_8000_HZ;

    }
    
    if (gipsCodec->GIPSVE_SetSendCNPayloadType(channel, ComfortNoisePayloadType,  freq) != 0)
    {
        LOG_GIPS_INFO( logTag, "GIPSVE_SetSendCNPayloadType cannot set CN payload type  to channel %d", channel );
        return -1;
    }
    // apply the codec to the channel
    if (gipsCodec->GIPSVE_SetSendCodec(channel, codec) != 0)
    {
        LOG_GIPS_ERROR( logTag, "CSFGipsAudioCodecSelector::setSend cannot set send codec to channel %d", channel );
        return -1;
    }

    
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::setSend applied codec %s (payload=%d, packetSize=%d) to channel %d",
            codec.plname, codec.pltype, codec.pacsize, channel);

    // return success
    return 0;
}

int CSFGipsAudioCodecSelector::setReceive(int channel, const GIPS_CodecInst& codec)
{
    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::setReceive channel=%d codec %s (payload=%d, packetSize=%d)",
            channel, codec.plname, codec.pltype, codec.pacsize);

    if (gipsCodec == NULL)
    {
        LOG_GIPS_ERROR( logTag, "CSFGipsAudioCodecSelector::setSend gipsCodec is null" );
        return -1;
    }

    if (gipsCodec->GIPSVE_SetRecPayloadType(channel, codec) != 0)
    {
        LOG_GIPS_ERROR( logTag, "CSFGipsAudioCodecSelector::setReceive cannot set receive codec to channel %d", channel );
        return -1;
    }

    LOG_GIPS_INFO( logTag, "CSFGipsAudioCodecSelector::setReceive applied codec %s (payload=%d, packetSize=%d) to channel %d",
            codec.plname, codec.pltype, codec.pacsize, channel);

    // return success
    return 0;
}

#endif
