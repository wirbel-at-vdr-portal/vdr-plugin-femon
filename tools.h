/*
 * tools.h: Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __FEMON_COMMON_H
#define __FEMON_COMMON_H

#include <stdint.h>
#include <vdr/channels.h>
#include <vdr/dvbdevice.h>
#include <vdr/remux.h>
#include <vdr/tools.h>

#define  ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define FRONTEND_DEVICE "/dev/dvb/adapter%d/frontend%d"

#define SATIP_DEVICE "SAT>IP"

cString getFrontendInfo(cDevice *deviceP);
cString getFrontendName(cDevice *deviceP);
cString getFrontendStatus(cDevice *deviceP);

double getCNR(cDevice *deviceP);
double getSignal(cDevice *deviceP);
double getBER(cDevice *deviceP);
double getPER(cDevice *deviceP);

cString getSignalStrength(double strengthP);
cString getApids(const cChannel *channelP);
cString getDpids(const cChannel *channelP);
cString getSpids(const cChannel *channelP);
cString getCAids(const cChannel *channelP);
cString getVideoStream(int valueP);
cString getVideoCodec(int valueP);
cString getAudioStream(int valueP, const cChannel *channelP);
cString getAudioCodec(int valueP);
cString getAudioChannelMode(int valueP);
cString getCoderate(int valueP);
cString getTransmission(int valueP);
cString getBandwidth(int valueP);
cString getInversion(int valueP);
cString getHierarchy(int valueP);
cString getGuard(int valueP);
cString getModulation(int valueP);
cString getTerrestrialSystem(int valueP);
cString getSatelliteSystem(int valueP);
cString getRollOff(int valueP);
cString getPilot(int valueP);
cString getResolution(int widthP, int heightP, int scanP);
cString getAspectRatio(int valueP);
cString getVideoFormat(int valueP);
cString getFrameRate(double valueP);
cString getAC3Stream(int valueP, const cChannel *channelP);
cString getAC3BitStreamMode(int valueP, int codingP);
cString getAC3AudioCodingMode(int valueP, int streamP);
cString getAC3CenterMixLevel(int valueP);
cString getAC3SurroundMixLevel(int valueP);
cString getAC3DolbySurroundMode(int valueP);
cString getAC3DialogLevel(int valueP);
cString getFrequencyMHz(int valueP);
cString getAudioSamplingFreq(int valueP);
cString getAudioBitrate(double valueP, double streamP);
cString getVideoBitrate(double valueP, double streamP);
cString getBitrateMbits(double valueP);
cString getBitrateKbits(double valueP);

class cFemonBitStream : public cBitStream {
public:
  cFemonBitStream(const uint8_t *dataP, const int lengthP) : cBitStream(dataP, lengthP) {}
  uint32_t       GetUeGolomb();
  int32_t        GetSeGolomb();
  void           SkipGolomb();
  void           SkipUeGolomb() { SkipGolomb(); }
  void           SkipSeGolomb() { SkipGolomb(); }
  };

#endif // __FEMON_COMMON_H
