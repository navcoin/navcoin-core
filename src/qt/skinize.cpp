#include "skinize.h"
#include <QString>

QString Skinize()
{
    return "QDialog { background: #e8ebf0; border: 0; text-transform: uppercase }"
           "QStackedWidget { background: #e8ebf0; border: 0 }"
           "QScrollArea { background: #e8ebf0; border: 0 }"
           "#BG1_1 { background: #eff1f4; border-radius: 2px; }"
           "#BG1_2 { background: #eff1f4; border-radius: 2px; }"
           "#BG1_3 { background: #eff1f4; border-radius: 2px; }"
           "QPushButton {  background: transparent; border: 1px solid #a9adc4; border-radius: 3px; padding: 6px; margin: 3px; text-transform: uppercase; color: #616998; font-weight: bold }"
           "QPushButton {  background: rgba(255,255,255,0.1); border: 1px solid #a9adc4; border-radius: 3px; padding: 6px; margin: 3px; text-transform: uppercase; color: #616998; font-weight: bold }"
           "QLineEdit { background: #fff; border-radius: 4px; border: 1px solid #7d7d7d; padding:5px }"
           "QSpingBox { background: #fff; border-radius: 4px; border: 1px solid #7d7d7d; padding:5px; margin-left: 20px }"
           "#unlockStakingButton { border-image: url(:/icons/lock_closed)  0 0 0 0 stretch stretch; border: 0px; }"
           "QLabel { color: #616998 }";
}
