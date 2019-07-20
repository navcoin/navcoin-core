#include "skinize.h"
#include <QString>
#include <QTextStream>
#include <QFile>

QString Skinize()
{
    // Where to load the styles
    static QString styles = "";

    // Check if we've already loaded the styles
    if (styles != "")
        return styles;

    // Load the style sheet
    QFile qss(":/stylesheets/light");

    // Check if we can access it
    if (qss.open(QIODevice::ReadOnly))
    {
        // Create a text stream
        QTextStream qssStream(&qss);

        // Load the whole stylesheet
        styles = qssStream.readAll();

        // Close the stream
        qss.close();
    }

    // Give back the styles to the caller
    return styles;
}
