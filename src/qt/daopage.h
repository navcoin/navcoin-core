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
#include "clientmodel.h"
#include "main.h"
#include "navcoinunits.h"
#include "optionsmodel.h"
#include "txdb.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
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

    struct ConsultationEntry {
        uint256 hash;
        QString question;
    };

    struct ConsultationAnswerEntry {
        uint256 hash;
        QString answer;
    };

    struct DeploymentEntry {
        uint256 hash;
        QString answer;
    };

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
    QVector<ConsultationAnswerEntry> consultationAnswerModel;
    QVector<DeploymentEntry> deploymentModel;

    QLabel* viewLbl;
    QPushButton* proposalsBtn;
    QPushButton* paymentRequestsBtn;
    QPushButton* consultationsBtn;
    QPushButton* deploymentsBtn;
    QLabel* filterLbl;
    QComboBox* filterCmb;
    QPushButton* createBtn;

    bool fActive;
    int nView;
    int nCurrentView;
    int nCurrentUnit;
    int nFilter;
    int nCurrentFilter;

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
        C_COLUMN_PADDING1,
        C_COLUMN_TITLE,
        C_COLUMN_REQUESTS,
        C_COLUMN_PAID,
        C_COLUMN_DURATION,
        C_COLUMN_VOTES,
        C_COLUMN_YES,
        C_COLUMN_NO,
        C_COLUMN_ABS,
        C_COLUMN_CYCLE,
        C_COLUMN_STATE,
        C_COLUMN_URL,
        C_COLUMN_VOTE,
        C_COLUMN_PADDING2
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
        FILTER_FINISHED
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

    void refresh(bool force = false);
};

#endif // DAOPAGE_H
