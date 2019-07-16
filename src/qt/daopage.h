// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DAOPAGE_H
#define DAOPAGE_H

#include "communityfundcreateproposaldialog.h"
#include "communityfundcreatepaymentrequestdialog.h"
#include "communityfunddisplaydetailed.h"
#include "communityfunddisplaypaymentrequestdetailed.h"
#include "consensus/dao.h"
#include "consensus/params.h"
#include "clientmodel.h"
#include "daoconsultationcreate.h"
#include "daoconsultationvote.h"
#include "daoversionbit.h"
#include "main.h"
#include "navcoinunits.h"
#include "optionsmodel.h"
#include "txdb.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSignalMapper>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#define DEFAULT_UNIT 0

class ClientModel;
class PlatformStyle;
class WalletModel;

struct ProposalEntry {
    uint256 hash;
    QString color;
    QString title;
    QString requests;
    QString paid;
    QString deadline;
    int nVotesYes;
    int nVotesNo;
    int nVotesAbs;
    unsigned int nCycle;
    QString sState;
    bool fCanVote;
    uint64_t nMyVote;
    uint64_t ts;
};

struct PaymentRequestEntry {
    uint256 hash;
    QString title;
    QString parentTitle;
    QString color;
    QString amount;
    int nVotesYes;
    int nVotesNo;
    int nVotesAbs;
    unsigned int nCycle;
    QString sState;
    bool fCanVote;
    uint64_t nMyVote;
    uint64_t ts;
};

struct ConsultationAnswerEntry {
    uint256 hash;
    QString answer;
    int nVotes;
    QString sState;
    bool fCanVote;
    bool fCanSupport;
    QString sMyVote;
};

struct ConsultationEntry {
    uint256 hash;
    QString color;
    QString question;
    QVector<ConsultationAnswerEntry> answers;
    QString sDesc;
    unsigned int nCycle;
    QString sState;
    DAOFlags::flags fState;
    bool fCanVote;
    QStringList myVotes;
    uint64_t ts;
    bool fRange;
    uint64_t nMin;
    uint64_t nMax;
    bool fCanSupport;
};

struct DeploymentEntry {
    QString color;
    QString title;
    QString status;
    bool fCanVote;
    bool fMyVote;
    bool fDefaultVote;
    int64_t ts;
    int bit;
};

class DaoPage : public QWidget
{
    Q_OBJECT

public:
    explicit DaoPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    void setWalletModel(WalletModel *walletModel);
    void setClientModel(ClientModel *clientModel);
    void initialize(CProposalMap proposalMap, CPaymentRequestMap paymentRequestMap, CConsultationMap consultationMap, CConsultationAnswerMap consultationAnswerMap, int unit);
    void setActive(bool flag);
    void setView(int view);



    void setData(QVector<ProposalEntry>);
    void setData(QVector<PaymentRequestEntry>);
    void setData(QVector<ConsultationEntry>);
    void setData(QVector<DeploymentEntry>);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    QTableWidget* table;
    QVBoxLayout *layout;
    QVector<ProposalEntry> proposalModel;
    QVector<PaymentRequestEntry> paymentRequestModel;
    QVector<ConsultationEntry> consultationModel;
    QVector<DeploymentEntry> deploymentModel;

    QLabel* viewLbl;
    QPushButton* proposalsBtn;
    QPushButton* paymentRequestsBtn;
    QPushButton* consultationsBtn;
    QPushButton* deploymentsBtn;
    QLabel* filterLbl;
    QComboBox* filterCmb;
    QPushButton* createBtn;
    QMenu* contextMenu;
    QTableWidgetItem *contextItem = nullptr;

    bool fActive;
    int nView;
    int nCurrentView;
    int nCurrentUnit;
    int nFilter;
    int nCurrentFilter;

    int nBadgeProposals;
    int nBadgePaymentRequests;
    int nBadgeConsultations;
    int nBadgeDeployments;

    enum {
        P_COLUMN_HASH,
        P_COLUMN_COLOR,
        P_COLUMN_TITLE,
        P_COLUMN_REQUESTS,
        P_COLUMN_PAID,
        P_COLUMN_DURATION,
        P_COLUMN_VOTES,
        P_COLUMN_STATE,
        P_COLUMN_URL,
        P_COLUMN_PADDING2,
        P_COLUMN_VOTE,
        P_COLUMN_PADDING3
    };

    enum {
        PR_COLUMN_HASH,
        PR_COLUMN_COLOR,
        PR_COLUMN_PARENT_TITLE,
        PR_COLUMN_TITLE,
        PR_COLUMN_REQUESTS,
        PR_COLUMN_VOTES,
        PR_COLUMN_STATE,
        PR_COLUMN_URL,
        PR_COLUMN_PADDING2,
        PR_COLUMN_VOTE,
        PR_COLUMN_PADDING3
    };

    enum {
        C_COLUMN_HASH,
        C_COLUMN_COLOR,
        C_COLUMN_TITLE,
        C_COLUMN_ANSWERS,
        C_COLUMN_STATUS,
        C_COLUMN_URL,
        C_COLUMN_PADDING2,
        C_COLUMN_MY_VOTES,
        C_COLUMN_VOTE,
        C_COLUMN_PADDING3
    };

    enum {
        D_COLUMN_COLOR,
        D_COLUMN_TITLE,
        D_COLUMN_STATUS,
        D_COLUMN_PADDING2,
        D_COLUMN_MY_VOTES,
        D_COLUMN_VOTE,
        D_COLUMN_PADDING3
    };

    enum {
        VIEW_PROPOSALS,
        VIEW_PAYMENT_REQUESTS,
        VIEW_CONSULTATIONS,
        VIEW_DEPLOYMENTS
    };

    enum {
        FILTER_ALL,
        FILTER_NOT_VOTED,
        FILTER_VOTED,
        FILTER_IN_PROGRESS,
        FILTER_FINISHED,
        FILTER_LOOKING_FOR_SUPPORT
    };

private Q_SLOTS:
    void viewProposals();
    void viewPaymentRequests();
    void viewConsultations();
    void viewDeployments();

    void onCreate();
    void onVote();
    void onDetails();
    void onFilter(int index);
    void onProposeAnswer();

    void refresh(bool force = false);

    void showContextMenu(QPoint pt);
};

#endif // DAOPAGE_H
