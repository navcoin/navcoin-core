#ifndef COMMUNITYFUNDDISPLAY_H
#define COMMUNITYFUNDDISPLAY_H

#include "consensus/cfund.h"
#include <QWidget>

namespace Ui {
class CommunityFundDisplay;
}

class CommunityFundDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit CommunityFundDisplay(QWidget *parent = 0, const CFund::CProposal* proposal);
    ~CommunityFundDisplay();

private:
    Ui::CommunityFundDisplay *ui;
    CFund::CProposal proposal;
};

#endif // COMMUNITYFUNDDISPLAY_H
