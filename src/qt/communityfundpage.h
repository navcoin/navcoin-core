#ifndef COMMUNITYFUNDPAGE_H
#define COMMUNITYFUNDPAGE_H

#include "communityfundpage.moc"
#include "consensus/cfund.h"

#include <QWidget>
#include <QPushButton>
#include <QListView>
#include <QPainter>

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
    //find a fix for Q_OBJECT macro, vtable error
    Q_OBJECT

public:
    explicit CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~CommunityFundPage();
    void Refresh(bool all, bool proposal);

private:
    Ui::CommunityFundPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CFund::flags flag;

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
