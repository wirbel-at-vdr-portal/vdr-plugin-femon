/*
 * osd.c: Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */
#include <string>
#include <iostream>
#include <cctype>
#include <cmath>
#include <repfunc.h>
#include "config.h"
#include "iptvservice.h"
#include "log.h"
#include "receiver.h"
#include "symbol.h"
#include "tools.h"
#include "osd.h"
#include "osd-defines.h"


class cFemonDummyFont : public cFont {
public:
  virtual int Width(void) const { return 10; }
  virtual int Width(uint cP) const { return 10; }
  virtual int Width(const char *sP) const { return 50; }
  virtual int Height(void) const { return 20; }
  virtual void DrawText(cBitmap *bitmapP, int xP, int yP, const char *sP, tColor colorFgP, tColor colorBgP, int widthP) const {}
  virtual void DrawText(cPixmap *pixmapP, int xP, int yP, const char *sP, tColor colorFgP, tColor colorBgP, int widthP) const {}
};

cFemonOsd *cFemonOsd::pInstanceS = NULL;

cFemonOsd *cFemonOsd::Instance(bool createP)
{
  debug1("%s (%d)", __PRETTY_FUNCTION__, createP);
  if ((pInstanceS == NULL) && createP)
  {
     pInstanceS = new cFemonOsd();
  }
  return (pInstanceS);
}

cFemonOsd::cFemonOsd()
: cOsdObject(true), cThread("femon osd"),
  osdM(NULL),
  receiverM(NULL),
  svdrpFrontendM(-1),
  svdrpVideoBitRateM(-1),
  svdrpAudioBitRateM(-1),
  svdrpPluginM(NULL),
  numberM(0),
  oldNumberM(0),
  qualityM(0),
  qualityValidM(false),
  strengthM(0),
  strengthValidM(false),
  cnrM(0),
  cnrValidM(false),
  signalM(0),
  signalValidM(false),
  berM(0),
  berValidM(false),
  perM(0),
  perValidM(false),
  frontendNameM(""),
  frontendTypeM(""),
  frontendStatusM(DTV_STAT_HAS_NONE),
  frontendStatusValidM(false),
  deviceSourceM(DEVICESOURCE_DVBAPI),
  displayModeM(FemonConfig.GetDisplayMode()),
  osdWidthM(cOsd::OsdWidth() * (100 - FemonConfig.GetDownscale()) / 100),
  osdHeightM(cOsd::OsdHeight() * (100 - FemonConfig.GetDownscale()) / 100),
  osdLeftM(cOsd::OsdLeft() + (cOsd::OsdWidth() * FemonConfig.GetDownscale() / 200)),
  osdTopM(cOsd::OsdTop() + (cOsd::OsdHeight() * FemonConfig.GetDownscale() / 200)),
  inputTimeM(0),
  sleepM(),
  mutexM()
{
  int tmp;
  debug1("%s", __PRETTY_FUNCTION__);
  svdrpConnectionM.handle = -1;
  femonSymbols.Refresh();
  fontM = cFont::CreateFont(Setup.FontSml, constrain(Setup.FontSmlSize, MINFONTSIZE, MAXFONTSIZE));
  if (!fontM || !fontM->Height()) {
     fontM = new cFemonDummyFont;
     error("%s Cannot create required font", __PRETTY_FUNCTION__);
     }
  tmp = 5 * OSDSYMBOL(SYMBOL_LOCK).Width() + 6 * OSDSPACING;
  if (OSDWIDTH < tmp) {
     error("%s OSD width (%d) smaller than required (%d).", __PRETTY_FUNCTION__, OSDWIDTH, tmp);
     OSDWIDTH = tmp;
     }
  tmp = OSDINFOHEIGHT + OSDROWHEIGHT + OSDSTATUSHEIGHT;
  if (OSDHEIGHT < tmp) {
     error("%s OSD height (%d) smaller than required (%d).", __PRETTY_FUNCTION__, OSDHEIGHT, tmp);
     OSDHEIGHT = tmp;
     }
}

cFemonOsd::~cFemonOsd(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  sleepM.Signal();
  if (Running())
     Cancel(3);
  if (svdrpConnectionM.handle >= 0) {
     svdrpPluginM = cPluginManager::GetPlugin(SVDRPPLUGIN);
     if (svdrpPluginM)
        svdrpPluginM->Service("SvdrpConnection-v1.0", &svdrpConnectionM);
     }
  if (receiverM) {
     receiverM->Deactivate();
     DELETENULL(receiverM);
     }
  if (osdM)
     DELETENULL(osdM);
  if (fontM)
     DELETENULL(fontM);
  pInstanceS = NULL;
}

void cFemonOsd::DrawStatusWindow(void)
{
  cMutexLock lock(&mutexM);
  LOCK_CHANNELS_READ;
  const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());

  if (osdM && channel) {
     cBitmap *bm = NULL;
     int offset = 0;
     int x = OSDWIDTH - OSDROUNDING;
     int y = 0;
     eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
     std::string ch(*cString::sprintf("%d%s %s", numberM ? numberM : channel->Number(), numberM ? "-" : "", channel->ShortName(true)));
     OSDDRAWSTATUSTITLEBAR(ch.c_str());
     if (svdrpFrontendM >= 0) {
        bm = &OSDSYMBOL(SYMBOL_SVDRP);
        OSDDRAWSTATUSBM(OSDSPACING);
        }
     switch (cDevice::ActualDevice()->CardIndex()) {
       case 1:  bm = &OSDSYMBOL(SYMBOL_ONE);   break;
       case 2:  bm = &OSDSYMBOL(SYMBOL_TWO);   break;
       case 3:  bm = &OSDSYMBOL(SYMBOL_THREE); break;
       case 4:  bm = &OSDSYMBOL(SYMBOL_FOUR);  break;
       case 5:  bm = &OSDSYMBOL(SYMBOL_FIVE);  break;
       case 6:  bm = &OSDSYMBOL(SYMBOL_SIX);   break;
       case 7:  bm = &OSDSYMBOL(SYMBOL_SEVEN); break;
       case 8:  bm = &OSDSYMBOL(SYMBOL_EIGHT); break;
       default: bm = &OSDSYMBOL(SYMBOL_ZERO);  break;
       }
     OSDDRAWSTATUSBM(OSDSPACING);
     bm = &OSDSYMBOL(SYMBOL_DEVICE);
     OSDDRAWSTATUSBM(0);
     if (IS_AUDIO_TRACK(track)) {
        switch (int(track - ttAudioFirst)) {
           case 1:  bm = &OSDSYMBOL(SYMBOL_ONE);   break;
           case 2:  bm = &OSDSYMBOL(SYMBOL_TWO);   break;
           case 3:  bm = &OSDSYMBOL(SYMBOL_THREE); break;
           case 4:  bm = &OSDSYMBOL(SYMBOL_FOUR);  break;
           case 5:  bm = &OSDSYMBOL(SYMBOL_FIVE);  break;
           case 6:  bm = &OSDSYMBOL(SYMBOL_SIX);   break;
           case 7:  bm = &OSDSYMBOL(SYMBOL_SEVEN); break;
           case 8:  bm = &OSDSYMBOL(SYMBOL_EIGHT); break;
           default: bm = &OSDSYMBOL(SYMBOL_ZERO);  break;
           }
        OSDDRAWSTATUSBM(OSDSPACING);
        switch (cDevice::PrimaryDevice()->GetAudioChannel()) {
           case 1:  bm = &OSDSYMBOL(SYMBOL_MONO_LEFT);  break;
           case 2:  bm = &OSDSYMBOL(SYMBOL_MONO_RIGHT); break;
           default: bm = &OSDSYMBOL(SYMBOL_STEREO);     break;
           }
        OSDDRAWSTATUSBM(0);
        }
     else if (receiverM && receiverM->AC3Valid() && IS_DOLBY_TRACK(track)) {
        if      (receiverM->AC3_5_1()) bm = &OSDSYMBOL(SYMBOL_DD51);
        else if (receiverM->AC3_2_0()) bm = &OSDSYMBOL(SYMBOL_DD20);
        else                            bm = &OSDSYMBOL(SYMBOL_DD);
        OSDDRAWSTATUSBM(OSDSPACING);
        }
     if (receiverM) {
        if (IS_OSDRESOLUTION(receiverM->VideoVerticalSize(), 2160)) {
           switch (receiverM->VideoScan()) {
              case VIDEO_SCAN_INTERLACED:  bm = &OSDSYMBOL(SYMBOL_FORMAT_2160i); break;
              case VIDEO_SCAN_PROGRESSIVE: bm = &OSDSYMBOL(SYMBOL_FORMAT_2160p); break;
              default:                     bm = &OSDSYMBOL(SYMBOL_FORMAT_2160);  break;
              }
           }
        else if (IS_OSDRESOLUTION(receiverM->VideoVerticalSize(), 1080)) {
           switch (receiverM->VideoScan()) {
              case VIDEO_SCAN_INTERLACED:  bm = &OSDSYMBOL(SYMBOL_FORMAT_1080i); break;
              case VIDEO_SCAN_PROGRESSIVE: bm = &OSDSYMBOL(SYMBOL_FORMAT_1080p); break;
              default:                     bm = &OSDSYMBOL(SYMBOL_FORMAT_1080);  break;
              }
           }
        else if (IS_OSDRESOLUTION(receiverM->VideoVerticalSize(), 720)) {
           switch (receiverM->VideoScan()) {
              case VIDEO_SCAN_INTERLACED:  bm = &OSDSYMBOL(SYMBOL_FORMAT_720i); break;
              case VIDEO_SCAN_PROGRESSIVE: bm = &OSDSYMBOL(SYMBOL_FORMAT_720p); break;
              default:                     bm = &OSDSYMBOL(SYMBOL_FORMAT_720);  break;
              }
           }
        else if (IS_OSDRESOLUTION(receiverM->VideoVerticalSize(), 576)) {
           switch (receiverM->VideoScan()) {
              case VIDEO_SCAN_INTERLACED:  bm = &OSDSYMBOL(SYMBOL_FORMAT_576i); break;
              case VIDEO_SCAN_PROGRESSIVE: bm = &OSDSYMBOL(SYMBOL_FORMAT_576p); break;
              default:                     bm = &OSDSYMBOL(SYMBOL_FORMAT_576);  break;
              }
           }
        else if (IS_OSDRESOLUTION(receiverM->VideoVerticalSize(), 480)) {
           switch (receiverM->VideoScan()) {
              case VIDEO_SCAN_INTERLACED:  bm = &OSDSYMBOL(SYMBOL_FORMAT_480i); break;
              case VIDEO_SCAN_PROGRESSIVE: bm = &OSDSYMBOL(SYMBOL_FORMAT_480p); break;
              default:                     bm = &OSDSYMBOL(SYMBOL_FORMAT_480);  break;
              }
           }
        else
           bm = NULL;
        OSDDRAWSTATUSBM(OSDSPACING);
        switch (receiverM->VideoCodec()) {
           case VIDEO_CODEC_MPEG2: bm = &OSDSYMBOL(SYMBOL_MPEG2); break;
           case VIDEO_CODEC_H264:  bm = &OSDSYMBOL(SYMBOL_H264);  break;
           case VIDEO_CODEC_H265:  bm = &OSDSYMBOL(SYMBOL_H265);  break;
           default:                bm = NULL;                     break;
           }
        OSDDRAWSTATUSBM(OSDSPACING);
        switch (receiverM->VideoFormat()) {
           case VIDEO_FORMAT_PAL:  bm = &OSDSYMBOL(SYMBOL_PAL);  break;
           case VIDEO_FORMAT_NTSC: bm = &OSDSYMBOL(SYMBOL_NTSC); break;
           default:                bm = NULL;                    break;
           }
        OSDDRAWSTATUSBM(OSDSPACING);
        switch (receiverM->VideoAspectRatio()) {
           case VIDEO_ASPECT_RATIO_1_1:    bm = &OSDSYMBOL(SYMBOL_AR_1_1);    break;
           case VIDEO_ASPECT_RATIO_4_3:    bm = &OSDSYMBOL(SYMBOL_AR_4_3);    break;
           case VIDEO_ASPECT_RATIO_16_9:   bm = &OSDSYMBOL(SYMBOL_AR_16_9);   break;
           case VIDEO_ASPECT_RATIO_2_21_1: bm = &OSDSYMBOL(SYMBOL_AR_2_21_1); break;
           default:                        bm = NULL;                         break;
           }
        OSDDRAWSTATUSBM(OSDSPACING);
        }
     if (channel->Ca() > 0xFF) {
        bm = &OSDSYMBOL(SYMBOL_ENCRYPTED);
        OSDDRAWSTATUSBM(OSDSPACING);
        }
     offset += OSDROWHEIGHT;

     {
     // inform other plugins about menu title
     std::string details;
     if (channel->Ca() > 0xFF)
        details = "CA";

     if (receiverM) {
        int i = receiverM->VideoAspectRatio();
        if (i > VIDEO_ASPECT_RATIO_EXTENDED) {
           details += ' ';
           details += *getAspectRatio(i);
           }

        i = receiverM->VideoFormat();
        if (i > VIDEO_FORMAT_COMPONENT) {
           details += ' ';
           details += *getVideoFormat(i);
           }

        i = receiverM->VideoCodec();
        if (i > VIDEO_CODEC_UNKNOWN) {
           details += ' ';
           details += *getVideoCodec(i);
           }

        bool found = true;
        i = receiverM->VideoVerticalSize();
        if (IS_OSDRESOLUTION(i, 2160))      i = 2160;
        else if (IS_OSDRESOLUTION(i, 1080)) i = 1080;
        else if (IS_OSDRESOLUTION(i, 720))  i = 720;
        else if (IS_OSDRESOLUTION(i, 576))  i = 576;
        else if (IS_OSDRESOLUTION(i, 480))  i = 480;
        else found = false;
        if (found) {
           details += " " + IntToStr(i);
           switch(receiverM->VideoScan()) {
              case VIDEO_SCAN_INTERLACED:  details += 'i'; break;
              case VIDEO_SCAN_PROGRESSIVE: details += 'p'; break;
              default:;
              }
           }
        }

     if (IS_AUDIO_TRACK(track)) {
        switch(cDevice::PrimaryDevice()->GetAudioChannel()) {
           case 1:  details += " left";   break;
           case 2:  details += " right";  break;
           default: details += " stereo"; break;
           }
        }
     else if (receiverM && receiverM->AC3Valid() && IS_DOLBY_TRACK(track)) {
        details += " DD";
        if (receiverM->AC3_5_1())
           details += "-5.1";
        else if (receiverM->AC3_2_0())
           details += "-2.0";
        }

     details += " #" + IntToStr(cDevice::ActualDevice()->CardIndex());

     if (svdrpFrontendM >= 0)
        details += " SVDRP";

     cStatus::MsgOsdTitle((BackFill(ch,40) + FrontFill(details,40)).c_str());
     }

     int n = 0;
     if (strengthValidM)
        OSDDRAWSTATUSBAR(strengthM);
     offset += OSDROWHEIGHT;
     if (qualityValidM)
        OSDDRAWSTATUSBAR(qualityM);
     offset += OSDROWHEIGHT;
     OSDDRAWSTATUSVALUES(signalValidM ? *cString::sprintf("STR: %s", *getSignalStrength(signalM)) : "STR: ---",
                         cnrValidM ? *cString::sprintf("CNR: %.2f dB", cnrM) : "CNR: ---",
                         berValidM ? *cString::sprintf("BER: %.0f", berM) : "BER: ---",
                         perValidM ? *cString::sprintf("PER: %.0f", perM) : "PER: ---",
                         *cString::sprintf("%s: %s", tr("Video"), *getBitrateMbits(receiverM ? receiverM->VideoBitrate() : (svdrpFrontendM >= 0 ? svdrpVideoBitRateM : -1.0))),
                         *cString::sprintf("%s: %s", (receiverM && receiverM->AC3Valid() && IS_DOLBY_TRACK(track)) ? tr("AC-3") : tr("Audio"), *getBitrateKbits(receiverM ? ((receiverM->AC3Valid() && IS_DOLBY_TRACK(track)) ? receiverM->AC3Bitrate() : receiverM->AudioBitrate()) : (svdrpFrontendM >= 0 ? svdrpAudioBitRateM : -1.0)))
                        );
     offset += OSDROWHEIGHT;
     x = OSDSYMBOL(SYMBOL_LOCK).Width();
     y = (OSDROWHEIGHT - OSDSYMBOL(SYMBOL_LOCK).Height()) / 2;
     if (frontendStatusValidM) {
        std::string fe_status;
        OSDDRAWSTATUSFRONTEND(1, OSDSYMBOL(SYMBOL_LOCK),    DTV_STAT_HAS_LOCK);
        OSDDRAWSTATUSFRONTEND(2, OSDSYMBOL(SYMBOL_SIGNAL),  DTV_STAT_HAS_SIGNAL);
        OSDDRAWSTATUSFRONTEND(3, OSDSYMBOL(SYMBOL_CARRIER), DTV_STAT_HAS_CARRIER);
        OSDDRAWSTATUSFRONTEND(4, OSDSYMBOL(SYMBOL_VITERBI), DTV_STAT_HAS_VITERBI);
        OSDDRAWSTATUSFRONTEND(5, OSDSYMBOL(SYMBOL_SYNC),    DTV_STAT_HAS_SYNC);
        }
     OSDDRAWSTATUSBOTTOMBAR();
     osdM->Flush();
     }
}

void cFemonOsd::DrawInfoWindow(void)
{
  cMutexLock lock(&mutexM);
  LOCK_CHANNELS_READ;
  const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());

  if (osdM && channel) {
     int offset = 0;
     int n = OSDSTATUSROWS;
     eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();

     switch (displayModeM) {
       case eFemonModeTransponder: {
            OSDDRAWINFOTITLEBAR(tr("Transponder Information"));
            offset += OSDROWHEIGHT;
            OSDDRAWINFO(trVDR("Vpid"), *cString::sprintf("%d", channel->Vpid()), \
                        trVDR("Ppid"), *cString::sprintf("%d", channel->Ppid()));
            offset += OSDROWHEIGHT;
            OSDDRAWINFO(   tr("Apid"), *getApids(channel), \
                           tr("Dpid"), *getDpids(channel));
            offset += OSDROWHEIGHT;
            OSDDRAWINFO(   tr("Spid"), *getSpids(channel), \
                        trVDR("Tpid"), *cString::sprintf("%d", channel->Tpid()));
            offset += OSDROWHEIGHT;
            OSDDRAWINFO( trVDR("Sid"),  *cString::sprintf("%d", channel->Sid()), \
                            tr("Nid"),  *cString::sprintf("%d", channel->Nid()));
            offset += OSDROWHEIGHT;
            OSDDRAWINFO(    tr("Tid"),  *cString::sprintf("%d", channel->Tid()), \
                            tr("Rid"),  *cString::sprintf("%d", channel->Rid()));
            offset += OSDROWHEIGHT;
            OSDDRAWINFO( trVDR("CA"),   *getCAids(channel), "", "");
            offset += OSDROWHEIGHT;
            switch (channel->Source() & cSource::st_Mask) {
              case cSource::stSat: {
                   cDvbTransponderParameters dtp(channel->Parameters());
                   OSDDRAWINFOLINE(*cString::sprintf("%s #%d - %s", *getSatelliteSystem(dtp.System()), (svdrpFrontendM >= 0) ? svdrpFrontendM : cDevice::ActualDevice()->CardIndex(), *frontendNameM));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Frequency"),    *getFrequencyMHz(channel->Frequency()), \
                                trVDR("Source"),       *cSource::ToString(channel->Source()));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Srate"),        *cString::sprintf("%d", channel->Srate()), \
                                trVDR("Polarization"), *cString::sprintf("%c", toupper(dtp.Polarization())));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Inversion"),    *getInversion(dtp.Inversion()), \
                                trVDR("CoderateH"),    *getCoderate(dtp.CoderateH()));
                   offset += OSDROWHEIGHT;
                   if (dtp.System()) {
                      OSDDRAWINFO( trVDR("System"),    *getSatelliteSystem(dtp.System()), \
                                   trVDR("RollOff"),   *getRollOff(dtp.RollOff()));
                      offset += OSDROWHEIGHT;
                      OSDDRAWINFO( trVDR("Pilot"),     *getPilot(dtp.Pilot()), "", "");
                      OSDCLEAR(18, 19);
                      }
                   else {
                      OSDDRAWINFO( trVDR("System"),    *getSatelliteSystem(dtp.System()), "", "");
                      OSDCLEAR(17, 19);
                      }
                   }
                   break;

              case cSource::stCable: {
                   cDvbTransponderParameters dtp(channel->Parameters());
                   OSDDRAWINFOLINE(*cString::sprintf("DVB-C #%d - %s", (svdrpFrontendM >= 0) ? svdrpFrontendM : cDevice::ActualDevice()->CardIndex(), *frontendNameM));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Frequency"),    *getFrequencyMHz(channel->Frequency()), \
                                trVDR("Source"),       *cSource::ToString(channel->Source()));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Srate"),        *cString::sprintf("%d", channel->Srate()), \
                                trVDR("Modulation"),   *getModulation(dtp.Modulation()));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Inversion"),    *getInversion(dtp.Inversion()), \
                                trVDR("CoderateH"),    *getCoderate(dtp.CoderateH()));
                   OSDCLEAR(16, 19);
                   }
                   break;

              case cSource::stTerr: {
                   cDvbTransponderParameters dtp(channel->Parameters());
                   OSDDRAWINFOLINE(*cString::sprintf("%s #%d - %s", *getTerrestrialSystem(dtp.System()), (svdrpFrontendM >= 0) ? svdrpFrontendM : cDevice::ActualDevice()->CardIndex(), *frontendNameM));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Frequency"),    *getFrequencyMHz(channel->Frequency()), \
                                trVDR("Transmission"), *getTransmission(dtp.Transmission()));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Bandwidth"),    *getBandwidth(dtp.Bandwidth()), \
                                trVDR("Modulation"),   *getModulation(dtp.Modulation()));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Inversion"),    *getInversion(dtp.Inversion()), \
                                tr   ("Coderate"),     *getCoderate(dtp.CoderateH()));
                   offset += OSDROWHEIGHT;
                   OSDDRAWINFO( trVDR("Hierarchy"),    *getHierarchy(dtp.Hierarchy()), \
                                trVDR("Guard"),        *getGuard(dtp.Guard()));
                   offset += OSDROWHEIGHT;
                   if (dtp.System()) {
                      OSDDRAWINFO( trVDR("System"),    *getTerrestrialSystem(dtp.System()), \
                                   trVDR("StreamId"),  *cString::sprintf("%d", dtp.StreamId()));
                      offset += OSDROWHEIGHT;
                      OSDDRAWINFO( trVDR("T2SystemId"),*cString::sprintf("%d", dtp.T2SystemId()), \
                                   trVDR("SISO/MISO"), *cString::sprintf("%d", dtp.SisoMiso()));
                      OSDCLEAR(19, 19);
                      }
                   else {
                      OSDDRAWINFO( trVDR("System"),    *getTerrestrialSystem(dtp.System()), "", "");
                      OSDCLEAR(18, 19);
                      }
                   }
                   break;

              case stIptv: {
                   OSDDRAWINFOLINE(*cString::sprintf("IPTV #%d - %s", (svdrpFrontendM >= 0) ? svdrpFrontendM : cDevice::ActualDevice()->CardIndex(), *frontendNameM));
                   offset += OSDROWHEIGHT;
                   if (svdrpFrontendM < 0) {
                      cPlugin *p;
                      IptvService_v1_0 data;
                      data.cardIndex = cDevice::ActualDevice()->CardIndex();
                      p = cPluginManager::CallFirstService("IptvService-v1.0", &data);
                      if (p) {
                         OSDDRAWINFO(tr("Protocol"),   *data.protocol, "", "");
                         offset += OSDROWHEIGHT;
                         OSDDRAWINFO(tr("Bitrate"),    *data.bitrate, "", "");
                         OSDCLEAR(15, 19);
                         }
                      else
                         OSDCLEAR(13, 19);
                      }
                   else
                      OSDCLEAR(13, 19);
                   }
                   break;

              default:
                   break;
              }
            OSDDRAWINFOBOTTOMBAR();
            break;
            }
       case eFemonModeStream: {
            OSDDRAWINFOTITLEBAR(tr("Stream Information"));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOACTIVE(  tr("Video Stream"),       *getVideoStream(channel->Vpid()));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Codec"),              *getVideoCodec(receiverM ? receiverM->VideoCodec() : VIDEO_CODEC_INVALID));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Bitrate"),            *getVideoBitrate(receiverM ? receiverM->VideoBitrate() : 0, receiverM ? receiverM->VideoStreamBitrate() : 0));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Aspect Ratio"),       *getAspectRatio(receiverM ? receiverM->VideoAspectRatio() : VIDEO_ASPECT_RATIO_INVALID));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Frame Rate"),         *getFrameRate(receiverM ? receiverM->VideoFrameRate() : 0));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Video Format"),       *getVideoFormat(receiverM ? receiverM->VideoFormat() : VIDEO_CODEC_INVALID));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Resolution"),         *getResolution(receiverM ? receiverM->VideoHorizontalSize() : 0, receiverM ? receiverM->VideoVerticalSize() : 0, receiverM ? receiverM->VideoScan() : VIDEO_SCAN_INVALID));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOACTIVE(  tr("Audio Stream"),       *getAudioStream(track, channel));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Codec"),              *getAudioCodec(receiverM ? receiverM->AudioCodec() : AUDIO_CODEC_INVALID));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Channel Mode"),       *getAudioChannelMode(receiverM ? receiverM->AudioChannelMode() : AUDIO_CHANNEL_MODE_INVALID));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Bitrate"),            *getAudioBitrate(receiverM ? receiverM->AudioBitrate() : 0, receiverM ? receiverM->AudioStreamBitrate() : 0));
            offset += OSDROWHEIGHT;
            OSDDRAWINFOINACTIVE(tr("Sampling Frequency"), *getAudioSamplingFreq(receiverM ? receiverM->AudioSamplingFreq() : AUDIO_SAMPLING_FREQUENCY_INVALID));
            OSDCLEAR(18, 19);
            OSDDRAWINFOBOTTOMBAR();
            break;
            }
       case eFemonModeAC3: {
            OSDDRAWINFOTITLEBAR(tr("Stream Information"));
            offset += OSDROWHEIGHT;
            if (receiverM && receiverM->AC3Valid() && IS_DOLBY_TRACK(track)) {
               OSDDRAWINFOACTIVE(  tr("AC-3 Stream"),            *getAC3Stream(track, channel));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Bitrate"),                *getAudioBitrate(receiverM->AC3Bitrate(), receiverM->AC3StreamBitrate()));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Sampling Frequency"),     *getAudioSamplingFreq(receiverM->AC3SamplingFreq()));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Bit Stream Mode"),        *getAC3BitStreamMode(receiverM->AC3BitStreamMode(), receiverM->AC3AudioCodingMode()));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Audio Coding Mode"),      *getAC3AudioCodingMode(receiverM->AC3AudioCodingMode(), receiverM->AC3BitStreamMode()));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Center Mix Level"),       *getAC3CenterMixLevel(receiverM->AC3CenterMixLevel()));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Surround Mix Level"),     *getAC3SurroundMixLevel(receiverM->AC3SurroundMixLevel()));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Dolby Surround Mode"),    *getAC3DolbySurroundMode(receiverM->AC3DolbySurroundMode()));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Low Frequency Effects"),  *cString::sprintf("%s", receiverM->AC3Lfe() ? trVDR("on") : trVDR("off")));
               offset += OSDROWHEIGHT;
               OSDDRAWINFOINACTIVE(tr("Dialogue Normalization"), *getAC3DialogLevel(receiverM->AC3DialogLevel()));
               OSDCLEAR(16, 19);
               }
            else
               OSDCLEAR(6, 19);
            OSDDRAWINFOBOTTOMBAR();
            break;
            }
       default: {
            OSDCLEARINFO();
            break;
            }
       }
     osdM->Flush();
     }
}

void cFemonOsd::Action(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  cTimeMs t;
  SvdrpCommand_v1_0 cmd;
  cmd.command = cString::sprintf("PLUG %s INFO\r\n", PLUGIN_NAME_I18N);
  while (Running()) {
    t.Set(0);
    svdrpFrontendM = -1;
    svdrpVideoBitRateM = -1.0;
    svdrpAudioBitRateM = -1.0;
    switch (deviceSourceM) {
      case DEVICESOURCE_PVRINPUT:
           qualityM = cDevice::ActualDevice()->SignalStrength();
           qualityValidM = (qualityM >= 0);
           strengthM = cDevice::ActualDevice()->SignalStrength();
           strengthValidM = (strengthM >= 0);
           frontendNameM = cDevice::ActualDevice()->DeviceName();
           frontendStatusM = strengthValidM ? (DTV_STAT_HAS_SIGNAL | DTV_STAT_HAS_CARRIER | DTV_STAT_HAS_VITERBI | DTV_STAT_HAS_SYNC | DTV_STAT_HAS_LOCK) : DTV_STAT_HAS_NONE;
           frontendStatusValidM = strengthValidM;
           signalM = strengthM;
           signalValidM = strengthValidM;
           cnrM = 0;
           cnrValidM = false;
           berM = 0;
           berValidM = false;
           perM = 0;
           perValidM = false;
           break;
      case DEVICESOURCE_IPTV:
           qualityM = cDevice::ActualDevice()->SignalQuality();
           qualityValidM = (qualityM >= 0);
           strengthM = cDevice::ActualDevice()->SignalStrength();
           strengthValidM = (strengthM >= 0);
           frontendNameM = cDevice::ActualDevice()->DeviceName();
           frontendStatusM = strengthValidM ? (DTV_STAT_HAS_SIGNAL | DTV_STAT_HAS_CARRIER | DTV_STAT_HAS_VITERBI | DTV_STAT_HAS_SYNC | DTV_STAT_HAS_LOCK) : DTV_STAT_HAS_NONE;
           frontendStatusValidM = strengthValidM;
           signalM = strengthM;
           signalValidM = strengthValidM;
           cnrM = qualityM;
           cnrValidM = qualityValidM;
           berM = 0;
           berValidM = false;
           perM = 0;
           perValidM = false;
           break;
      default:
      case DEVICESOURCE_DVBAPI:
           if (svdrpConnectionM.handle >= 0) {
              cmd.handle = svdrpConnectionM.handle;
              svdrpPluginM->Service("SvdrpCommand-v1.0", &cmd);
              if (cmd.responseCode == 900) {
                 strengthValidM = false;
                 qualityValidM = false;
                 frontendStatusValidM = false;
                 signalValidM = false;
                 cnrValidM = false;
                 berValidM = false;
                 perValidM = false;
                 for (cLine *line = cmd.reply.First(); line; line = cmd.reply.Next(line)) {
                     const char *s = line->Text();
	             if (!strncasecmp(s, "CARD:", 5))
                        svdrpFrontendM = (int)strtol(s + 5, NULL, 10);
                     else if (!strncasecmp(s, "STRG:", 5)) {
                        strengthM = (int)strtol(s + 5, NULL, 10);
                        strengthValidM = (strengthM >= 0);
                        }
                     else if (!strncasecmp(s, "QUAL:", 5)) {
                        qualityM = (int)strtol(s + 5, NULL, 10);
                        qualityValidM = (qualityM >= 0);
                        }
                     else if (!strncasecmp(s, "TYPE:", 5)) {
                        frontendTypeM = s + 5;
                        }
                     else if (!strncasecmp(s, "NAME:", 5)) {
                        frontendNameM = s + 5;
                        }
                     else if (!strncasecmp(s, "STAT:", 5)) {
                        frontendStatusM = strtol(s + 5, NULL, 16);
                        frontendStatusValidM = true;
                        }
                     else if (!strncasecmp(s, "SGNL:", 5)) {
                        signalM = atod(s + 5);
                        signalValidM = true;
                        }
                     else if (!strncasecmp(s, "CNRA:", 5)) {
                        cnrM = atod(s + 5);
                        cnrValidM = true;
                        }
                     else if (!strncasecmp(s, "BERA:", 5)) {
                        berM = atod(s + 5);
                        berValidM = true;
                        }
                     else if (!strncasecmp(s, "PERA:", 5)) {
                        perM = atod(s + 5);
                        perValidM = true;
                        }
                     else if (!strncasecmp(s, "VIBR:", 5))
                        svdrpVideoBitRateM = atod(s + 5);
                     else if (!strncasecmp(s, "AUBR:", 5))
                        svdrpAudioBitRateM = atod(s + 5);
                     }
                 }
              }
           else {
              int valid;
              qualityM = cDevice::ActualDevice()->SignalQuality();
              qualityValidM = (qualityM >= 0);
              strengthM = cDevice::ActualDevice()->SignalStrength();
              strengthValidM = (strengthM >= 0);
              frontendNameM = cDevice::ActualDevice()->DeviceName();
              if (cDevice::ActualDevice()->SignalStats(valid, &signalM, &cnrM, NULL, &berM, &perM, &frontendStatusM)) {
                 frontendStatusValidM = valid & DTV_STAT_VALID_STATUS;
                 signalValidM = valid & DTV_STAT_VALID_STRENGTH;
                 cnrValidM = valid & DTV_STAT_VALID_CNR;
                 berValidM = valid & DTV_STAT_VALID_BERPOST;
                 perValidM = valid & DTV_STAT_VALID_PER;
                 }
              else {
                 frontendStatusValidM = false;
                 signalValidM = false;
                 cnrValidM = false;
                 berValidM = false;
                 perValidM = false;
                 }
              }
           break;
      }
    DrawInfoWindow();
    DrawStatusWindow();
    sleepM.Wait(max((int)(100 * FemonConfig.GetUpdateInterval() - t.Elapsed()), 3));
    }
}

void cFemonOsd::Show(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
  LOCK_CHANNELS_READ;
  const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());

  AttachFrontend();
  
  osdM = cOsdProvider::NewOsd(osdLeftM, osdTopM);
  if (osdM) {
     tArea Areas1[] = { { 0, 0, OSDWIDTH - 1, OSDHEIGHT - 1, 8 } };
     if (Setup.AntiAlias && osdM->CanHandleAreas(Areas1, sizeof(Areas1) / sizeof(tArea)) == oeOk) {
        osdM->SetAreas(Areas1, sizeof(Areas1) / sizeof(tArea));
        }
     else {
        tArea Areas2[] = { { 0, OSDSTATUSWIN_Y(0),          OSDWIDTH - 1, OSDSTATUSWIN_Y(0) + OSDSTATUSHEIGHT - 1, FemonTheme[FemonConfig.GetTheme()].bpp },
                           { 0, OSDINFOWIN_Y(0),            OSDWIDTH - 1, OSDINFOWIN_Y(0)   + OSDROWHEIGHT    - 1, FemonTheme[FemonConfig.GetTheme()].bpp },
                           { 0, OSDINFOWIN_Y(OSDROWHEIGHT), OSDWIDTH - 1, OSDINFOWIN_Y(0)   + OSDINFOHEIGHT   - 1, 2                                 } };
        osdM->SetAreas(Areas2, sizeof(Areas2) / sizeof(tArea));
        }
     OSDCLEARSTATUS();
     OSDCLEARINFO();
     osdM->Flush();
     if (receiverM) {
        receiverM->Deactivate();
        DELETENULL(receiverM);
        }
     if (FemonConfig.GetAnalyzeStream() && channel) {
        receiverM = new cFemonReceiver(channel, IS_AUDIO_TRACK(track) ? int(track - ttAudioFirst) : 0, IS_DOLBY_TRACK(track) ? int(track - ttDolbyFirst) : 0);
        cDevice::ActualDevice()->AttachReceiver(receiverM);
        }
     Start();
     }
}

bool cFemonOsd::AttachFrontend(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  LOCK_CHANNELS_READ;
  const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());

  deviceSourceM = DEVICESOURCE_DVBAPI;
  if (channel) {
     if (channel->IsSourceType('I'))
        deviceSourceM = DEVICESOURCE_IPTV;
     else if (channel->IsSourceType('V'))
        deviceSourceM = DEVICESOURCE_PVRINPUT;
     }

  if (deviceSourceM == DEVICESOURCE_DVBAPI) {
     if (!strstr(*cDevice::ActualDevice()->DeviceType(), SATIP_DEVICE)) {
        if (FemonConfig.GetUseSvdrp()) {
           if (!SvdrpConnect() || !SvdrpTune())
              return false;
           }
        }
     }

  return true;
}

void cFemonOsd::ChannelSwitch(const cDevice * deviceP, int channelNumberP, bool liveViewP)
{
  debug1("%s (%d, %d, %d)", __PRETTY_FUNCTION__, deviceP->DeviceNumber(), channelNumberP, liveViewP);
  eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
  LOCK_CHANNELS_READ;
  const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());

  if (!deviceP || !liveViewP)
     return;

  if (!channelNumberP) {
     if (receiverM) {
        receiverM->Deactivate();
        DELETENULL(receiverM);
        }
     return;
     }

  if (channel && FemonConfig.GetAnalyzeStream() && AttachFrontend()) {
     if (receiverM) {
        receiverM->Deactivate();
        DELETENULL(receiverM);
        }
     receiverM = new cFemonReceiver(channel, IS_AUDIO_TRACK(track) ? int(track - ttAudioFirst) : 0, IS_DOLBY_TRACK(track) ? int(track - ttDolbyFirst) : 0);
     cDevice::ActualDevice()->AttachReceiver(receiverM);
     }
}

void cFemonOsd::SetAudioTrack(int indexP, const char * const *tracksP)
{
  debug1("%s (%d, )", __PRETTY_FUNCTION__, indexP);
  eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
  if (receiverM) {
     receiverM->Deactivate();
     DELETENULL(receiverM);
     }
  if (FemonConfig.GetAnalyzeStream()) {
     LOCK_CHANNELS_READ;
     const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());
     if (channel) {
        receiverM = new cFemonReceiver(channel, IS_AUDIO_TRACK(track) ? int(track - ttAudioFirst) : 0, IS_DOLBY_TRACK(track) ? int(track - ttDolbyFirst) : 0);
        cDevice::ActualDevice()->AttachReceiver(receiverM);
        }
     }
}

bool cFemonOsd::DeviceSwitch(int directionP)
{
  debug1("%s (%d)", __PRETTY_FUNCTION__, directionP);
  int device = cDevice::ActualDevice()->DeviceNumber();
  int direction = sgn(directionP);
  if (device >= 0) {
     LOCK_CHANNELS_READ;
     const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());
     if (channel) {
        for (int i = 0; i < cDevice::NumDevices() - 1; i++) {
            if (direction >= 0) {
               if (++device >= cDevice::NumDevices())
                  device = 0;
               }
            else {
               if (--device < 0)
                  device = cDevice::NumDevices() - 1;
               }
            // Collect the current priorities of all CAM slots that can decrypt the channel:
            int NumCamSlots = CamSlots.Count();
            int SlotPriority[NumCamSlots];
            int NumUsableSlots = 0;
            bool NeedsDetachAllReceivers = false;
            bool InternalCamNeeded = false;
            bool ValidDevice = false;
            cCamSlot *s = NULL;
            cDevice *d = cDevice::GetDevice(device);
            if (channel->Ca() >= CA_ENCRYPTED_MIN) {
               for (cCamSlot *CamSlot = CamSlots.First(); CamSlot; CamSlot = CamSlots.Next(CamSlot)) {
                   SlotPriority[CamSlot->Index()] = MAXPRIORITY + 1; // assumes it can't be used
                   if (CamSlot->ModuleStatus() == msReady) {
                      if (CamSlot->ProvidesCa(channel->Caids())) {
                         if (!ChannelCamRelations.CamChecked(channel->GetChannelID(), CamSlot->SlotNumber())) {
                            SlotPriority[CamSlot->Index()] = CamSlot->Priority();
                            NumUsableSlots++;
                            }
                         }
                      }
                  }
               if (!NumUsableSlots)
                  InternalCamNeeded = true; // no CAM is able to decrypt this channel
               }
            for (int j = 0; j < NumCamSlots || !NumUsableSlots; ++j) {
                if (NumUsableSlots && SlotPriority[j] > MAXPRIORITY)
                   continue; // there is no CAM available in this slot
                bool HasInternalCam = d->HasInternalCam();
                if (InternalCamNeeded && !HasInternalCam)
                   continue; // no CAM is able to decrypt this channel and the device uses vdr handled CAMs
                if (NumUsableSlots && !HasInternalCam && !CamSlots.Get(j)->Assign(d, true))
                   continue; // CAM slot can't be used with this device
                if (d->ProvidesChannel(channel, 0, &NeedsDetachAllReceivers)) { // this device is basically able to do the job
                   debug1("%s (%d) device=%d", __PRETTY_FUNCTION__, direction, device);
                   if (NumUsableSlots && !HasInternalCam && d->CamSlot() && d->CamSlot() != CamSlots.Get(j))
                      NeedsDetachAllReceivers = true; // using a different CAM slot requires detaching receivers
                   if (NumUsableSlots && !HasInternalCam)
                      s = CamSlots.Get(j);
                   ValidDevice = true;
                   break;
                   }
                if (!NumUsableSlots)
                   break; // no CAM necessary, so just one loop over the devices
                }
            // Do the actual switch if valid device found
            if (d && ValidDevice) {
               cControl::Shutdown();
               if (NeedsDetachAllReceivers)
                  d->DetachAllReceivers();
               if (s) {
                  if (s->Device() != d) {
                     if (s->Device())
                        s->Device()->DetachAllReceivers();
                     if (d->CamSlot())
                        d->CamSlot()->Assign(NULL);
                     s->Assign(d);
                     }
                  }
               else if (d->CamSlot() && !d->CamSlot()->IsDecrypting())
                  d->CamSlot()->Assign(NULL);
               d->SwitchChannel(channel, false);
               cControl::Launch(new cTransferControl(d, channel));
               AttachFrontend();
               return true;
               }
            }
        }
     }
   return false;
}

bool cFemonOsd::SvdrpConnect(void)
{
   if (svdrpConnectionM.handle < 0) {
      svdrpPluginM = cPluginManager::GetPlugin(SVDRPPLUGIN);
      if (svdrpPluginM) {
         svdrpConnectionM.serverIp = FemonConfig.GetSvdrpIp();
         svdrpConnectionM.serverPort = (unsigned short)FemonConfig.GetSvdrpPort();
         svdrpConnectionM.shared = true;
         svdrpPluginM->Service("SvdrpConnection-v1.0", &svdrpConnectionM);
         if (svdrpConnectionM.handle >= 0) {
            SvdrpCommand_v1_0 cmd;
            cmd.handle = svdrpConnectionM.handle;
            cmd.command = cString::sprintf("PLUG %s\r\n", PLUGIN_NAME_I18N);
            svdrpPluginM->Service("SvdrpCommand-v1.0", &cmd);
            if (cmd.responseCode != 214) {
               svdrpPluginM->Service("SvdrpConnection-v1.0", &svdrpConnectionM); // close connection
               error("%s Cannot find plugin '%s' on server %s", __PRETTY_FUNCTION__, PLUGIN_NAME_I18N, *svdrpConnectionM.serverIp);
               }
            }
         else
            error("%s Cannot connect to SVDRP server", __PRETTY_FUNCTION__);
         }
      else
         error("%s Cannot find plugin '%s'", __PRETTY_FUNCTION__, SVDRPPLUGIN);
      }
   return svdrpConnectionM.handle >= 0;
}

bool cFemonOsd::SvdrpTune(void)
{
   if (svdrpPluginM && svdrpConnectionM.handle >= 0) {
      LOCK_CHANNELS_READ;
      const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());
      if (channel) {
         SvdrpCommand_v1_0 cmd;
         cmd.handle = svdrpConnectionM.handle;
         cmd.command = cString::sprintf("CHAN %s\r\n", *channel->GetChannelID().ToString());
         svdrpPluginM->Service("SvdrpCommand-v1.0", &cmd);
         if (cmd.responseCode == 250)
            return true;
         error("%s Cannot tune server channel", __PRETTY_FUNCTION__);
         }
      else
         error("%s Invalid channel", __PRETTY_FUNCTION__);
      }
   else
      error("%s Unexpected connection state", __PRETTY_FUNCTION__);
   return false;
}

double cFemonOsd::GetVideoBitrate(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  double value = 0.0;

  if (receiverM)
     value = receiverM->VideoBitrate();

  return (value);
}

double cFemonOsd::GetAudioBitrate(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  double value = 0.0;

  if (receiverM)
     value = receiverM->AudioBitrate();

  return (value);
}

double cFemonOsd::GetDolbyBitrate(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  double value = 0.0;

  if (receiverM)
     value = receiverM->AC3Bitrate();

  return (value);
}

eOSState cFemonOsd::ProcessKey(eKeys keyP)
{
  eOSState state = cOsdObject::ProcessKey(keyP);
  if (state == osUnknown) {
     switch (int(keyP)) {
       case k0:
            if ((numberM == 0) && (oldNumberM != 0)) {
               numberM = oldNumberM;
               oldNumberM = cDevice::CurrentChannel();
               LOCK_CHANNELS_READ;
               Channels->SwitchTo(numberM);
               numberM = 0;
               return osContinue;
               }
       case k1 ... k9:
            if (numberM >= 0) {
               numberM = numberM * 10 + keyP - k0;
               if (numberM > 0) {
                  DrawStatusWindow();
                  LOCK_CHANNELS_READ;
                  const cChannel *ch = Channels->GetByNumber(numberM);
                  inputTimeM.Set(0);
                  // Lets see if there can be any useful further input:
                  int n = ch ? numberM * 10 : 0;
                  while (ch && (ch = Channels->Next(ch)) != NULL) {
                        if (!ch->GroupSep()) {
                           if (n <= ch->Number() && ch->Number() <= n + 9) {
                              n = 0;
                              break;
                              }
                           if (ch->Number() > n)
                              n *= 10;
                           }
                        }
                  if (n > 0) {
                     // This channel is the only one that fits the input, so let's take it right away:
                     oldNumberM = cDevice::CurrentChannel();
                     Channels->SwitchTo(numberM);
                     numberM = 0;
                     }
                  }
               }
            break;
       case kBack:
            return osEnd;
       case kGreen:
            {
            eTrackType types[ttMaxTrackTypes];
            eTrackType CurrentAudioTrack = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
            int numTracks = 0;
            int oldTrack = 0;
            int track = 0;
            for (int i = ttAudioFirst; i <= ttDolbyLast; i++) {
                const tTrackId *TrackId = cDevice::PrimaryDevice()->GetTrack(eTrackType(i));
                if (TrackId && TrackId->id) {
                   types[numTracks] = eTrackType(i);
                   if (i == CurrentAudioTrack)
                      track = numTracks;
                   numTracks++;
                   }
                }
            oldTrack = track;
            if (++track >= numTracks)
               track = 0;
            if (track != oldTrack) {
               cDevice::PrimaryDevice()->SetCurrentAudioTrack(types[track]);
               Setup.CurrentDolby = IS_DOLBY_TRACK(types[track]);
               }
            }
            break;
       case kYellow:
            if (IS_AUDIO_TRACK(cDevice::PrimaryDevice()->GetCurrentAudioTrack())) {
               int audioChannel = cDevice::PrimaryDevice()->GetAudioChannel();
               int oldAudioChannel = audioChannel;
               if (++audioChannel > 2)
                  audioChannel = 0;
               if (audioChannel != oldAudioChannel) {
                  cDevice::PrimaryDevice()->SetAudioChannel(audioChannel);
                  }
               }
            break;
       case kRight:
            DeviceSwitch(1);
            break;
       case kLeft:
            DeviceSwitch(-1);
            break;
       case kUp|k_Repeat:
       case kUp:
       case kDown|k_Repeat:
       case kDown:
            oldNumberM = cDevice::CurrentChannel();
            cDevice::SwitchChannel(NORMALKEY(keyP) == kUp ? 1 : -1);
            numberM = 0;
            break;
       case kNone:
            if (numberM && (inputTimeM.Elapsed() > CHANNELINPUT_TIMEOUT)) {
               LOCK_CHANNELS_READ;
               if (Channels->GetByNumber(numberM)) {
                  oldNumberM = cDevice::CurrentChannel();
                  Channels->SwitchTo(numberM);
                  numberM = 0;
                  }
               else {
                  inputTimeM.Set(0);
                  numberM = 0;
                  }
               }
            break;
       case kOk:
            {
            // toggle between display modes
            LOCK_CHANNELS_READ;
            const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());
            if (++displayModeM == eFemonModeAC3 && channel && !channel->Dpid(0)) displayModeM++;
            if (displayModeM >= eFemonModeMaxNumber) displayModeM = 0;
            DrawInfoWindow();
            }
            break;
       default:
            break;
       }
     state = osContinue;
     }
  return state;
}
