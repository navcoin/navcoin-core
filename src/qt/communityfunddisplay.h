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
    CommunityFundDisplay(QWidget *parent = 0, const CFund::CProposal proposal = CFund::CProposal());
    ~CommunityFundDisplay();

private:
    Ui::CommunityFundDisplay *ui;
    const CFund::CProposal proposal;
};

#endif // COMMUNITYFUNDDISPLAY_H
