#ifndef COMMUNITYFUNDPAGE_H
#define COMMUNITYFUNDPAGE_H

#include "communityfundpage.moc"
#include "consensus/cfund.h"
#include "wallet/wallet.h"

#include <QWidget>
#include <QPushButton>
#include <QListView>
#include <QPainter>
#include <QLayoutItem>

// Color constants
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
    void append(QWidget* widget);
    void refresh(bool all, bool proposal);
    void deleteChildWidgets(QLayoutItem *item);
    void reset();
    ~CommunityFundPage();

private:
    Ui::CommunityFundPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CFund::flags flag;
    CWallet *wallet;
    bool viewing_proposals;
    bool viewing_voted;
    bool viewing_unvoted;

private Q_SLOTS:
    void click_pushButtonProposals();
    void click_pushButtonPaymentRequests();
    void click_radioButtonAll();
    void click_radioButtonYourVote();
    void click_radioButtonPending();
    void click_radioButtonAccepted();
    void click_radioButtonRejected();
    void click_radioButtonExpired();
    void click_pushButtonCreateProposal();
    void click_pushButtonCreatePaymentRequest();
    void click_radioButtonNoVote();

};

#endif // COMMUNITYFUNDPAGE_H
