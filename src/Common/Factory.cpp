﻿//
// Created by xzl on 2018/10/24.
//

#include "Factory.h"
#include "RtmpMuxer/H264RtmpCodec.h"
#include "RtmpMuxer/AACRtmpCodec.h"
#include "RtspMuxer/H264RtpCodec.h"
#include "RtspMuxer/AACRtpCodec.h"

namespace mediakit{

Sdp::Ptr Factory::getSdpByTrack(const Track::Ptr &track) {
    switch (track->getCodecId()){
        case CodecH264:{
            H264Track::Ptr h264Track = dynamic_pointer_cast<H264Track>(track);
            if(!h264Track){
                WarnL << "该Track不是H264Track类型";
                return nullptr;
            }
            if(!h264Track->ready()){
                WarnL << "该Track未准备好";
                return nullptr;
            }
            return std::make_shared<H264Sdp>(h264Track->getSps(),h264Track->getPps());
        }

        case CodecAAC:{
            AACTrack::Ptr aacTrack = dynamic_pointer_cast<AACTrack>(track);
            if(!aacTrack){
                WarnL << "该Track不是AACTrack类型";
                return nullptr;
            }
            if(!aacTrack->ready()){
                WarnL << "该Track未准备好";
                return nullptr;
            }
            return std::make_shared<AACSdp>(aacTrack->getAacCfg(),aacTrack->getAudioSampleRate());
        }
        default:
            WarnL << "暂不支持的CodecId:" << track->getCodecId();
            return nullptr;
    }
}


Track::Ptr Factory::getTrackBySdp(const string &sdp) {
    if (strcasestr(sdp.data(), "mpeg4-generic") != nullptr) {
        string aac_cfg_str = FindField(sdp.c_str(), "config=", "\r\n");
        if (aac_cfg_str.size() != 4) {
            aac_cfg_str = FindField(sdp.c_str(), "config=", ";");
        }
        if (aac_cfg_str.size() != 4) {
            //延后获取adts头
            return std::make_shared<AACTrack>();
        }
        string aac_cfg;

        unsigned int cfg1;
        sscanf(aac_cfg_str.substr(0, 2).c_str(), "%02X", &cfg1);
        cfg1 &= 0x00FF;
        aac_cfg.push_back(cfg1);

        unsigned int cfg2;
        sscanf(aac_cfg_str.substr(2, 2).c_str(), "%02X", &cfg2);
        cfg2 &= 0x00FF;
        aac_cfg.push_back(cfg2);

        return std::make_shared<AACTrack>(aac_cfg);
    }

    if (strcasestr(sdp.data(), "h264") != nullptr) {
        string sps_pps = FindField(sdp.c_str(), "sprop-parameter-sets=", "\r\n");
        if(sps_pps.empty()){
            return std::make_shared<H264Track>();
        }
        string base64_SPS = FindField(sps_pps.c_str(), NULL, ",");
        string base64_PPS = FindField(sps_pps.c_str(), ",", NULL);
        if(base64_PPS.back() == ';'){
            base64_PPS.pop_back();
        }

        auto sps = decodeBase64(base64_SPS);
        auto pps = decodeBase64(base64_PPS);
        return std::make_shared<H264Track>(sps,pps,0,0);
    }

    WarnL << "暂不支持该sdp:" << sdp;
    return nullptr;
}


CodecId Factory::getCodecIdByAmf(const AMFValue &val){
    if (val.type() == AMF_STRING){
        auto str = val.as_string();
        if(str == "avc1"){
            return CodecH264;
        }
        if(str == "mp4a"){
            return CodecAAC;
        }
        return CodecInvalid;
    }

    if (val.type() != AMF_NULL){
        auto type_id = val.as_integer();
        switch (type_id){
            case 7:{
                return CodecH264;
            }
            case 10:{
                return CodecAAC;
            }
            default:
                return CodecInvalid;
        }
    }
    return CodecInvalid;
}

Track::Ptr Factory::getTrackByCodecId(CodecId codecId) {
    switch (codecId){
        case CodecH264:{
            return std::make_shared<H264Track>();
        }
        case CodecAAC:{
            return std::make_shared<AACTrack>();
        }
        default:
            WarnL << "暂不支持该CodecId:" << codecId;
            return nullptr;
    }
}


Track::Ptr Factory::getTrackByAmf(const AMFValue &amf) {
    CodecId codecId = getCodecIdByAmf(amf);
    if(codecId == CodecInvalid){
        WarnL << "暂不支持该Amf:" << amf.type();
        return nullptr;
    }
    return getTrackByCodecId(codecId);
}


RtpCodec::Ptr Factory::getRtpEncoderById(CodecId codecId,
                                          uint32_t ui32Ssrc,
                                          uint32_t ui32MtuSize,
                                          uint32_t ui32SampleRate,
                                          uint8_t ui8PlayloadType,
                                          uint8_t ui8Interleaved) {
    switch (codecId){
        case CodecH264:
            return std::make_shared<H264RtpEncoder>(ui32Ssrc,ui32MtuSize,ui32SampleRate,ui8PlayloadType,ui8Interleaved);
        case CodecAAC:
            return std::make_shared<AACRtpEncoder>(ui32Ssrc,ui32MtuSize,ui32SampleRate,ui8PlayloadType,ui8Interleaved);
        default:
            WarnL << "暂不支持该CodecId:" << codecId;
            return nullptr;
    }
}

RtpCodec::Ptr Factory::getRtpDecoderById(CodecId codecId, uint32_t ui32SampleRate) {
    switch (codecId){
        case CodecH264:
            return std::make_shared<H264RtpDecoder>();
        case CodecAAC:
            return std::make_shared<AACRtpDecoder>(ui32SampleRate);
        default:
            WarnL << "暂不支持该CodecId:" << codecId;
            return nullptr;
    }
}

RtmpCodec::Ptr Factory::getRtmpCodecById(CodecId codecId) {
    switch (codecId){
        case CodecH264:
            return std::make_shared<H264RtmpEncoder>();
        case CodecAAC:
            return std::make_shared<AACRtmpEncoder>();
        default:
            WarnL << "暂不支持该CodecId:" << codecId;
            return nullptr;
    }
}


}//namespace mediakit
