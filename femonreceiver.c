/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <unistd.h>
#include "femontools.h"
#include "femoncfg.h"
#include "femonreceiver.h"

cFemonReceiver::cFemonReceiver(tChannelID ChannelID, int Ca, int Vpid, int Apid[], int Dpid[])
: cReceiver(ChannelID, -1, Vpid, Apid, Dpid, NULL),
  cThread("femon receiver"),
  m_Sleep(),
  m_Active(false),
  m_DetectH264(this),
  m_DetectMPEG(this, this),
  m_DetectAAC(this),
  m_DetectAC3(this),
  m_VideoPid(Vpid),
  m_VideoPacketCount(0),
  m_VideoBitrate(0.0),
  m_VideoValid(false),
  m_AudioPid(Apid[0]),
  m_AudioPacketCount(0),
  m_AudioBitrate(0.0),
  m_AudioValid(false),
  m_AC3Pid(Dpid[0]),
  m_AC3PacketCount(0), 
  m_AC3Bitrate(0),
  m_AC3Valid(false)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);

  m_VideoInfo.codec = VIDEO_CODEC_INVALID;
  m_VideoInfo.format = VIDEO_FORMAT_INVALID;
  m_VideoInfo.scan = VIDEO_SCAN_INVALID;
  m_VideoInfo.aspectRatio = VIDEO_ASPECT_RATIO_INVALID;
  m_VideoInfo.width = 0;
  m_VideoInfo.height = 0;
  m_VideoInfo.frameRate = 0;
  m_VideoInfo.bitrate = AUDIO_BITRATE_INVALID;
  m_AudioInfo.codec = AUDIO_CODEC_UNKNOWN;
  m_AudioInfo.bitrate = AUDIO_BITRATE_INVALID;
  m_AudioInfo.samplingFrequency = AUDIO_SAMPLING_FREQUENCY_INVALID;
  m_AudioInfo.channelMode = AUDIO_CHANNEL_MODE_INVALID;
  m_AC3Info.bitrate = AUDIO_BITRATE_INVALID;
  m_AC3Info.samplingFrequency = AUDIO_SAMPLING_FREQUENCY_INVALID;
  m_AC3Info.bitstreamMode = AUDIO_BITSTREAM_MODE_INVALID;
  m_AC3Info.audioCodingMode = AUDIO_CODING_MODE_INVALID;
  m_AC3Info.dolbySurroundMode = AUDIO_DOLBY_SURROUND_MODE_INVALID;
  m_AC3Info.centerMixLevel = AUDIO_CENTER_MIX_LEVEL_INVALID;
  m_AC3Info.surroundMixLevel = AUDIO_SURROUND_MIX_LEVEL_INVALID;
  m_AC3Info.dialogLevel = 0;
  m_AC3Info.lfe = false;
}

cFemonReceiver::~cFemonReceiver(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  Deactivate();
}

void cFemonReceiver::Deactivate(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  if (m_Active) {
     m_Active = false;
     m_Sleep.Signal();
     if (Running())
        Cancel(3);
     Detach();
     }
}

void cFemonReceiver::Activate(bool On)
{
  Dprintf("%s(%d)\n", __PRETTY_FUNCTION__, On);
  if (On)
     Start();
  else
     Deactivate();
}

void cFemonReceiver::Receive(uchar *Data, int Length)
{
  // TS packet length: TS_SIZE
  if (Length == TS_SIZE) {
     int len, pid = TsPid(Data);
     if (pid == m_VideoPid) {
        m_VideoPacketCount++;
        if (TsPayloadStart(Data)) {
           while (const uint8_t *p = m_VideoAssembler.GetPes(len)) {
             if (m_DetectMPEG.processVideo(p, len) || m_DetectH264.processVideo(p, len)) {
                m_VideoValid = true;
                break;
                }
             }
           m_VideoAssembler.Reset();
         }
         m_VideoAssembler.PutTs(Data, Length);
        }
     else if (pid == m_AudioPid) {
        m_AudioPacketCount++;
        if (const uint8_t *p = m_AudioAssembler.GetPes(len)) {
           if (m_DetectAAC.processAudio(p, len) || m_DetectMPEG.processAudio(p, len))
              m_AudioValid = true;
           m_AudioAssembler.Reset();
           }
        m_AudioAssembler.PutTs(Data, Length);
        }
     else if (pid == m_AC3Pid) {
        m_AC3PacketCount++;
        if (const uint8_t *p = m_AC3Assembler.GetPes(len)) {
           if (m_DetectAC3.processAudio(p, len))
              m_AC3Valid = true;
           m_AC3Assembler.Reset();
           }
        m_AC3Assembler.PutTs(Data, Length);
        }
     }
}

void cFemonReceiver::Action(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  cTimeMs t;
  m_Active = true;
  while (Running() && m_Active) {
        t.Set(0);
        // TS packet 188 bytes - 4 byte header; MPEG standard defines 1Mbit = 1000000bit
        m_VideoBitrate = (10.0 * 8.0 * 184.0 * m_VideoPacketCount) / femonConfig.calcinterval;
        m_VideoPacketCount = 0;
        m_AudioBitrate = (10.0 * 8.0 * 184.0 * m_AudioPacketCount) / femonConfig.calcinterval;
        m_AudioPacketCount = 0;
        m_AC3Bitrate   = (10.0 * 8.0 * 184.0 * m_AC3PacketCount)   / femonConfig.calcinterval;
        m_AC3PacketCount = 0;
        m_Sleep.Wait(max((int)(100 * femonConfig.calcinterval - t.Elapsed()), 3));
    }
}
