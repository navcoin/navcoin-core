#ifndef COMMUNITYFUNDDISPLAYDETAILED_H
#define COMMUNITYFUNDDISPLAYDETAILED_H

#include "consensus/cfund.h"
#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class CommunityFundDisplayDetailed;
}

class CommunityFundDisplayDetailed : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundDisplayDetailed(QWidget *parent = 0, CFund::CProposal proposal = CFund::CProposal());
    ~CommunityFundDisplayDetailed();

private:
    Ui::CommunityFundDisplayDetailed *ui;
    CFund::CProposal proposal;
    void setProposalLabels() const;

public Q_SLOTS:
    void on_click_buttonBoxYesNoVote(QAbstractButton *button);
};

#endif // COMMUNITYFUNDDISPLAYDETAILED_H
