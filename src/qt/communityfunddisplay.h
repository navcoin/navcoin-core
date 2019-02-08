#ifndef COMMUNITYFUNDDISPLAY_H
#define COMMUNITYFUNDDISPLAY_H

#include <QWidget>

namespace Ui {
class CommunityFundDisplay;
}

class CommunityFundDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit CommunityFundDisplay(QWidget *parent = 0);
    ~CommunityFundDisplay();

private:
    Ui::CommunityFundDisplay *ui;
};

#endif // COMMUNITYFUNDDISPLAY_H
