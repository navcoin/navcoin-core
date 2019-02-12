#ifndef COMMUNITYFUNDDISPLAY_H
#define COMMUNITYFUNDDISPLAY_H

#include "consensus/cfund.h"
#include <QWidget>
#include <QAbstractButton>

namespace Ui {
class CommunityFundDisplay;
}

class CommunityFundDisplay : public QWidget
{
    Q_OBJECT

public:
    CommunityFundDisplay(QWidget *parent = 0, CFund::CProposal proposal = CFund::CProposal());
    ~CommunityFundDisplay();

private:
    Ui::CommunityFundDisplay *ui;
    CFund::CProposal proposal;

public Q_SLOTS:
    void on_click_buttonBoxVote(QAbstractButton *button);
    void on_click_pushButtonDetails();
};

#endif // COMMUNITYFUNDDISPLAY_H
