#ifndef COMMUNITYFUNDDISPLAY_H
#define COMMUNITYFUNDDISPLAY_H

#include <consensus/dao.h>
#include <wallet/wallet.h>
#include <QWidget>
#include <QAbstractButton>

namespace Ui {
class CommunityFundDisplay;
}

class CommunityFundDisplay : public QWidget
{
    Q_OBJECT

public:
    CommunityFundDisplay(QWidget *parent = 0, CProposal proposal = CProposal());
    ~CommunityFundDisplay();

private:
    Ui::CommunityFundDisplay *ui;
    CProposal proposal;
    CWallet *wallet;
    void refresh();

public Q_SLOTS:
    void click_buttonBoxVote(QAbstractButton *button);
    void click_pushButtonDetails();
};

#endif // COMMUNITYFUNDDISPLAY_H
