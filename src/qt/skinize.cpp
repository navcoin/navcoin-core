#include "skinize.h"
#include <QString>

QString Skinize()
{
    return "QDialog { background: #e8ebf0; border: 0 }"
           "QStackedWidget { background: #eff1f4; border: 0 }"
           "QScrollArea { background: #eff1f4; border: 0 }"
           "QPushButton {  background: #f6f6f6; border: 1px solid #a9adc4; border-radius: 5px; padding: 6px; text-transform: uppercase; color: #616998; font-weight: bold }"
           "QTextBox { background: #fff; border-radius: 4px; border: 1px solid #000; padding:2px }";
}
