#ifndef COMMUNITYFUNDDISPLAYDETAILED_H
#define COMMUNITYFUNDDISPLAYDETAILED_H

#include <QWidget>
#include <consensus/dao.h>
#include <wallet/wallet.h>
#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class CommunityFundDisplayDetailed;
}

class CommunityFundDisplayDetailed : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundDisplayDetailed(QWidget *parent = 0, CProposal proposal = CProposal());
    ~CommunityFundDisplayDetailed();

private:
    Ui::CommunityFundDisplayDetailed *ui;
    CProposal proposal;
    CWallet *wallet;
    void setProposalLabels();

public Q_SLOTS:
    void click_buttonBoxYesNoVote(QAbstractButton *button);
    void onDetails();
};

#endif // COMMUNITYFUNDDISPLAYDETAILED_H
