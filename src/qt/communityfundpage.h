#ifndef COMMUNITYFUNDPAGE_H
#define COMMUNITYFUNDPAGE_H

#include "communityfundpage.moc"

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
    //Q_OBJECT

public:
    explicit CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~CommunityFundPage();

private:
    Ui::CommunityFundPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
};

#endif // COMMUNITYFUNDPAGE_H
