#pragma once


#define CHANNELINPUT_TIMEOUT      1000
#define SVDRPPLUGIN               "svdrpservice"

#ifndef MINFONTSIZE
   #define MINFONTSIZE 10
#endif

#ifndef MAXFONTSIZE
   #define MAXFONTSIZE 64
#endif

#define OSDWIDTH                  osdWidthM                      // in pixels

#define OSDHEIGHT                 osdHeightM                     // in pixels

#define OSDROWHEIGHT              fontM->Height()                // in pixels

#define OSDSTATUSROWS             5                              //  5 rows

#define OSDINFOROWS               14                             // 14 rows

#define OSDINFOHEIGHT             (OSDROWHEIGHT * OSDINFOROWS)   // in pixels (14 rows)

#define OSDSTATUSHEIGHT           (OSDROWHEIGHT * OSDSTATUSROWS) // in pixels ( 5 rows)

#define OSDSYMBOL(id)             femonSymbols.Get(id)

#define OSDSPACING                femonSymbols.GetSpacing()

#define OSDROUNDING               femonSymbols.GetRounding()

#define IS_OSDROUNDING            (FemonConfig.GetSkin() == eFemonSkinElchi)

#define IS_OSDRESOLUTION(r1, r2)  (abs(r1 - r2) < 20)

#define OSDINFOWIN_Y(offset)      (FemonConfig.GetPosition() ? (OSDHEIGHT - OSDINFOHEIGHT + offset) : offset)

#define OSDINFOWIN_X(col)         ((col == 4) ? int(round(OSDWIDTH * 0.76)) :                                                                                                               \
                                   (col == 3) ? int(round(OSDWIDTH * 0.51)) :                                                                                                               \
                                   (col == 2) ? int(round(OSDWIDTH * 0.26)) :                                                                                                               \
                                                int(round(OSDWIDTH * 0.025)))

#define OSDSTATUSWIN_Y(offset)    (FemonConfig.GetPosition() ? offset : (OSDHEIGHT - OSDSTATUSHEIGHT + offset))

#define OSDSTATUSWIN_X(col)       ((col == 6) ? int(round(OSDWIDTH * 0.84)) :                                                                                                               \
                                   (col == 5) ? int(round(OSDWIDTH * 0.66)) :                                                                                                               \
                                   (col == 4) ? int(round(OSDWIDTH * 0.50)) :                                                                                                               \
                                   (col == 3) ? int(round(OSDWIDTH * 0.35)) :                                                                                                               \
                                   (col == 2) ? int(round(OSDWIDTH * 0.19)) :                                                                                                               \
                                                int(round(OSDWIDTH * 0.025)))

#define OSDSTATUSWIN_XSYMBOL(c,w) (c * ((OSDWIDTH - (5 * w)) / 6) + ((c - 1) * w))

#define OSDBARWIDTH(x)            (OSDWIDTH * x / 100)

#define OSDDRAWSTATUSBM(spacing)                                                                                                                                                            \
        do {                                                                                                                                                                                \
           if (bm) {                                                                                                                                                                        \
              x -= bm->Width() + spacing;                                                                                                                                                   \
              y = (OSDROWHEIGHT - bm->Height()) / 2;                                                                                                                                        \
              if (y < 0) y = 0;                                                                                                                                                             \
              osdM->DrawBitmap(x, OSDSTATUSWIN_Y(offset) + y, *bm, FemonTheme[FemonConfig.GetTheme()].clrTitleText, FemonTheme[FemonConfig.GetTheme()].clrTitleBackground);                 \
              }                                                                                                                                                                             \
           } while(0)

#define OSDDRAWSTATUSFRONTEND(column, bitmap, status)                                                                                                                                       \
        do {                                                                                                                                                                                \
           osdM->DrawBitmap(OSDSTATUSWIN_XSYMBOL(column, x), OSDSTATUSWIN_Y(offset) + y, bitmap,                                                                                            \
                            frontendStatusM & status ? FemonTheme[FemonConfig.GetTheme()].clrActiveText : FemonTheme[FemonConfig.GetTheme()].clrRed,                                        \
                            FemonTheme[FemonConfig.GetTheme()].clrBackground);                                                                                                              \
           /* inform other plugins about frontend status */                                                                                                                                 \
           switch(column) {                                                                                                                                                                 \
              case(1): if (frontendStatusM & DTV_STAT_HAS_LOCK)     fe_status += " LOCK";     break;                                                                                        \
              case(2): if (frontendStatusM & DTV_STAT_HAS_SIGNAL)   fe_status += " SIGNAL";   break;                                                                                        \
              case(3): if (frontendStatusM & DTV_STAT_HAS_CARRIER)  fe_status += " CARRIER";  break;                                                                                        \
              case(4): if (frontendStatusM & DTV_STAT_HAS_VITERBI)  fe_status += " VITERBI";  break;                                                                                        \
              case(5): if (frontendStatusM & DTV_STAT_HAS_SYNC)     fe_status += " SYNC";     break;                                                                                        \
              }                                                                                                                                                                             \
           if (column == 5) {                                                                                                                                                               \
              cStatus::MsgOsdItem(fe_status.c_str(), 3);                                                                                                                                    \
              }                                                                                                                                                                             \
           } while(0)


#define OSDDRAWSTATUSVALUES(label1, label2, label3, label4, label5, label6)                                                                                                                 \
        do {                                                                                                                                                                                \
           osdM->DrawText(OSDSTATUSWIN_X(1), OSDSTATUSWIN_Y(offset), label1, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);  \
           osdM->DrawText(OSDSTATUSWIN_X(2), OSDSTATUSWIN_Y(offset), label2, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);  \
           osdM->DrawText(OSDSTATUSWIN_X(3), OSDSTATUSWIN_Y(offset), label3, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);  \
           osdM->DrawText(OSDSTATUSWIN_X(4), OSDSTATUSWIN_Y(offset), label4, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);  \
           osdM->DrawText(OSDSTATUSWIN_X(5), OSDSTATUSWIN_Y(offset), label5, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);  \
           osdM->DrawText(OSDSTATUSWIN_X(6), OSDSTATUSWIN_Y(offset), label6, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);  \
           /* inform other plugins about current measurements */                                                                                                                            \
           std::string status(label1);                                                                                                                                                      \
           status += " ";                                                                                                                                                                   \
           status += label2;                                                                                                                                                                \
           status += " ";                                                                                                                                                                   \
           status += label3;                                                                                                                                                                \
           status += " ";                                                                                                                                                                   \
           status += label4;                                                                                                                                                                \
           status += " ";                                                                                                                                                                   \
           status += label5;                                                                                                                                                                \
           status += " ";                                                                                                                                                                   \
           status += label6;                                                                                                                                                                \
           cStatus::MsgOsdItem(status.c_str(), 2);                                                                                                                                          \
           } while(0)


#define OSDDRAWSTATUSBAR(value)                                                                                                                                                             \
        do {                                                                                                                                                                                \
           if (value > 0) {                                                                                                                                                                 \
              int barvalue = OSDBARWIDTH(value);                                                                                                                                            \
              osdM->DrawRectangle(0, OSDSTATUSWIN_Y(offset) + 3, min(OSDBARWIDTH(FemonConfig.GetRedLimit()), barvalue), OSDSTATUSWIN_Y(offset) + OSDROWHEIGHT - 3,                          \
                                  FemonTheme[FemonConfig.GetTheme()].clrRed);                                                                                                               \
              if (barvalue > OSDBARWIDTH(FemonConfig.GetRedLimit()))                                                                                                                        \
                 osdM->DrawRectangle(OSDBARWIDTH(FemonConfig.GetRedLimit()), OSDSTATUSWIN_Y(offset) + 3, min((OSDWIDTH * FemonConfig.GetGreenLimit() / 100), barvalue),                     \
                                     OSDSTATUSWIN_Y(offset) + OSDROWHEIGHT - 3, FemonTheme[FemonConfig.GetTheme()].clrYellow);                                                              \
              if (barvalue > OSDBARWIDTH(FemonConfig.GetGreenLimit()))                                                                                                                      \
                 osdM->DrawRectangle(OSDBARWIDTH(FemonConfig.GetGreenLimit()), OSDSTATUSWIN_Y(offset) + 3, barvalue,                                                                        \
                                     OSDSTATUSWIN_Y(offset) + OSDROWHEIGHT - 3, FemonTheme[FemonConfig.GetTheme()].clrGreen);                                                               \
              /* inform other plugins about strength */                                                                                                                                     \
              std::string len((int) (value * 0.7), '=');                                                                                                                                    \
              if      (n == 0)  cStatus::MsgOsdItem(("STR " + len).c_str(), 0);                                                                                                             \
              else if (n == 1)  cStatus::MsgOsdItem(("QUA " + len).c_str(), 1);                                                                                                             \
              }                                                                                                                                                                             \
           n++;                                                                                                                                                                             \
          } while(0)

#define OSDDRAWSTATUSTITLEBAR(title)                                                                                                                                                        \
        do {                                                                                                                                                                                \
           osdM->DrawRectangle(0, OSDSTATUSWIN_Y(offset), OSDWIDTH, OSDSTATUSWIN_Y(offset) + OSDROWHEIGHT - 1, FemonTheme[FemonConfig.GetTheme()].clrTitleBackground);                      \
           osdM->DrawText(OSDSTATUSWIN_X(1), OSDSTATUSWIN_Y(offset), title, FemonTheme[FemonConfig.GetTheme()].clrTitleText, FemonTheme[FemonConfig.GetTheme()].clrTitleBackground, fontM); \
           if (IS_OSDROUNDING) {                                                                                                                                                            \
              osdM->DrawEllipse(0, OSDSTATUSWIN_Y(0), OSDROUNDING, OSDSTATUSWIN_Y(OSDROUNDING), clrTransparent, -2);                                                                        \
              osdM->DrawEllipse(OSDWIDTH - OSDROUNDING, OSDSTATUSWIN_Y(0), OSDWIDTH, OSDSTATUSWIN_Y(OSDROUNDING), clrTransparent, -1);                                                      \
              }                                                                                                                                                                             \
           osdM->DrawRectangle(0, OSDSTATUSWIN_Y(offset) + OSDROWHEIGHT, OSDWIDTH, OSDSTATUSWIN_Y(offset) + OSDSTATUSHEIGHT - 1, FemonTheme[FemonConfig.GetTheme()].clrBackground);         \
           } while(0)

#define OSDDRAWSTATUSBOTTOMBAR()                                                                                                                                                            \
        do {                                                                                                                                                                                \
           if (IS_OSDROUNDING) {                                                                                                                                                            \
              osdM->DrawEllipse(0, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT) - OSDROUNDING, OSDROUNDING, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT), clrTransparent, -3);                                        \
              osdM->DrawEllipse(OSDWIDTH - OSDROUNDING, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT) - OSDROUNDING, OSDWIDTH, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT), clrTransparent, -4);                      \
              }                                                                                                                                                                             \
           } while(0)

#define OSDCLEARSTATUS()                                                                                                                                                                    \
        do {                                                                                                                                                                                \
           osdM->DrawRectangle(0, OSDSTATUSWIN_Y(0), OSDWIDTH, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT) - 1, clrTransparent);                                                                        \
           /* inform other plugins about clearing the info area */                                                                                                                          \
           for(int i=0; i<OSDSTATUSROWS; i++) cStatus::MsgOsdItem("", i);                                                                                                                   \
           } while(0)

#define OSDDRAWINFOLEFT(label, value)                                                                                                                                                       \
        do {                                                                                                                                                                                \
           osdM->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), label, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);       \
           osdM->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), value, FemonTheme[FemonConfig.GetTheme()].clrActiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);         \
           } while(0)

#define OSDDRAWINFORIGHT(label, value)                                                                                                                                                      \
        do {                                                                                                                                                                                \
           osdM->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), label, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);       \
           osdM->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), value, FemonTheme[FemonConfig.GetTheme()].clrActiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);         \
           } while(0)

#define OSDDRAWINFO(Llabel, Lvalue, Rlabel, Rvalue)                                                                                                                                         \
        do {                                                                                                                                                                                \
           OSDDRAWINFOLEFT(Llabel, Lvalue);                                                                                                                                                 \
           std::string msg(BackFill(Llabel, 16) + BackFill(Lvalue , 16));                                                                                                                   \
           if (*Rlabel) {                                                                                                                                                                   \
              OSDDRAWINFORIGHT(Rlabel, Rvalue);                                                                                                                                             \
              msg += BackFill(Rlabel, 16) + Rvalue;                                                                                                                                         \
              }                                                                                                                                                                             \
           cStatus::MsgOsdItem(msg.c_str(), n++);                                                                                                                                           \
           } while(0)

#define OSDDRAWINFOACTIVE(label, value)                                                                                                                                                     \
        do {                                                                                                                                                                                \
           osdM->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), label, FemonTheme[FemonConfig.GetTheme()].clrActiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);         \
           osdM->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), value, FemonTheme[FemonConfig.GetTheme()].clrActiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);         \
           std::string Info(BackFill(label, 32) + value);                                                                                                                                   \
           cStatus::MsgOsdItem(Info.c_str(), n++);                                                                                                                                          \
           } while(0)

#define OSDDRAWINFOINACTIVE(label, value)                                                                                                                                                   \
        do {                                                                                                                                                                                \
           osdM->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), label, FemonTheme[FemonConfig.GetTheme()].clrInactiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);       \
           osdM->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), value, FemonTheme[FemonConfig.GetTheme()].clrActiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);         \
           std::string Info(BackFill(label, 32) + value);                                                                                                                                   \
           cStatus::MsgOsdItem(Info.c_str(), n++);                                                                                                                                          \
           } while(0)

#define OSDDRAWINFOLINE(label)                                                                                                                                                              \
        do {                                                                                                                                                                                \
           osdM->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), label, FemonTheme[FemonConfig.GetTheme()].clrActiveText, FemonTheme[FemonConfig.GetTheme()].clrBackground, fontM);         \
           cStatus::MsgOsdItem(label, n++);                                                                                                                                                 \
           } while(0)

#define OSDDRAWINFOTITLEBAR(title)                                                                                                                                                          \
        do {                                                                                                                                                                                \
           osdM->DrawRectangle(0, OSDINFOWIN_Y(offset), OSDWIDTH, OSDINFOWIN_Y(offset) + OSDROWHEIGHT - 1, FemonTheme[FemonConfig.GetTheme()].clrTitleBackground);                          \
           osdM->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), title, FemonTheme[FemonConfig.GetTheme()].clrTitleText, FemonTheme[FemonConfig.GetTheme()].clrTitleBackground, fontM);     \
           if (IS_OSDROUNDING) {                                                                                                                                                            \
              osdM->DrawEllipse(0, OSDINFOWIN_Y(0), OSDROUNDING, OSDINFOWIN_Y(OSDROUNDING), clrTransparent, -2);                                                                            \
              osdM->DrawEllipse(OSDWIDTH - OSDROUNDING, OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDROUNDING), clrTransparent, -1);                                                          \
              }                                                                                                                                                                             \
           osdM->DrawRectangle(0, OSDINFOWIN_Y(offset) + OSDROWHEIGHT, OSDWIDTH, OSDINFOWIN_Y(offset) + OSDINFOHEIGHT - 1, FemonTheme[FemonConfig.GetTheme()].clrBackground);               \
           /* inform other plugins about title */                                                                                                                                           \
           std::string Title("--- ");                                                                                                                                                       \
           Title += title;                                                                                                                                                                  \
           Title += " ---";                                                                                                                                                                 \
           cStatus::MsgOsdItem(Title.c_str(), n++);                                                                                                                                         \
           } while(0)

#define OSDDRAWINFOBOTTOMBAR()                                                                                                                                                              \
        do {                                                                                                                                                                                \
           if (IS_OSDROUNDING) {                                                                                                                                                            \
              osdM->DrawEllipse(0, OSDINFOWIN_Y(OSDINFOHEIGHT) - OSDROUNDING, OSDROUNDING, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent, -3);                                                \
              osdM->DrawEllipse((OSDWIDTH - OSDROUNDING), OSDINFOWIN_Y(OSDINFOHEIGHT) - OSDROUNDING, OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent, -4);                            \
              }                                                                                                                                                                             \
           } while(0)

#define OSDCLEAR(from, to)                                                                                                                                                                  \
        do {                                                                                                                                                                                \
           for(int i=from; i<=to; i++) {                                                                                                                                                    \
              cStatus::MsgOsdItem("", i);                                                                                                                                                   \
              }                                                                                                                                                                             \
           } while(0)

#define OSDCLEARINFO()                                                                                                                                                                      \
        do {                                                                                                                                                                                \
           osdM->DrawRectangle(0, OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT) - 1, clrTransparent);                                                                              \
           /* inform other plugins about clearing the info area */                                                                                                                          \
           OSDCLEAR(5, 19);                                                                                                                                                                 \
           } while(0)

