// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_SPLASHSCREEN_H
#define NAVCOIN_QT_SPLASHSCREEN_H

#include <QLabel>
#include <QHBoxLayout>

class NetworkStyle;

/** Class for the splashscreen with information of the running client.
 *
 * @note this is intentionally not a QSplashScreen. Navcoin Core initialization
 * can take a long time, and in that case a progress window that cannot be
 * moved around and minimized has turned out to be frustrating to the user.
 */
class SplashScreen : public QWidget
{
    Q_OBJECT

public:
    explicit SplashScreen(Qt::WindowFlags f, const NetworkStyle *networkStyle);
    ~SplashScreen();

protected:
    void closeEvent(QCloseEvent *event);

public Q_SLOTS:
    /** Slot to call finish() method as it's not defined as slot */
    void slotFinish(QWidget *mainWin);

    /** Get the screen scale, usefull for scaling UI elements */
    float scale();

    /** Show message and progress */
    void showMessage(const QString &message, const QColor &color);

    /** Update the progress */
    void updateProgress();

private:
    /** Connect core signals to splash screen */
    void subscribeToCoreSignals();
    /** Disconnect core signals to splash screen */
    void unsubscribeFromCoreSignals();

    QPixmap pixmap;
    QLabel* statusLabel;
    QWidget* splashBarInner;
    QHBoxLayout* splashBarLayout;
    QSize splashSize;
    QTimer* timerProgress;
};

#endif // NAVCOIN_QT_SPLASHSCREEN_H
