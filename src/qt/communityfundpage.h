#ifndef COMMUNITYFUNDPAGE_H
#define COMMUNITYFUNDPAGE_H

#include "communityfundpage.moc"
#include "consensus/cfund.h"

#include <QWidget>
#include <QPushButton>
#include <QListView>
#include <QPainter>

#define COLOR_VOTE_YES "background-color: #90ee90;"
#define COLOR_VOTE_NO "background-color: #f08080;"
#define COLOR_VOTE_NEUTRAL "background-color: #f3f4f6;"

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class PlatformStyle;
class WalletModel;

namespace Ui {
class CommunityFundPage;
}

class CommunityFundPage : public QWidget
{
    Q_OBJECT

public:
    explicit CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    void setWalletModel(WalletModel *walletModel);
    void refreshTab();
    ~CommunityFundPage();
    void Refresh(bool all, bool proposal);

private:
    Ui::CommunityFundPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CFund::flags flag;
    bool viewing_proposals;
    bool viewing_voted;

private Q_SLOTS:
    void on_click_pushButtonProposals();
    void on_click_pushButtonPaymentRequests();
    void on_click_radioButtonAll();
    void on_click_radioButtonYourVote();
    void on_click_radioButtonPending();
    void on_click_radioButtonAccepted();
    void on_click_radioButtonRejected();
    void on_click_radioButtonExpired();
    void on_click_pushButtonCreateProposal();
    void on_click_pushButtonCreatePaymentRequest();

};

#endif // COMMUNITYFUNDPAGE_H
