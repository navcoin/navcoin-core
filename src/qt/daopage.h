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
#include "daoproposeanswer.h"
#include "daosupport.h"
#include "daoversionbit.h"
#include "main.h"
#include "navcoinpushbutton.h"
#include "navcoinunits.h"
#include "optionsmodel.h"
#include "txdb.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalMapper>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
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

struct ConsensusEntry {
    uint256 hash;
    QString color;
    QString parameter;
    uint64_t value;
    QVector<ConsultationAnswerEntry> answers;
    unsigned int nCycle;
    QString sState;
    DAOFlags::flags fState;
    bool fCanVote;
    QStringList myVotes;
    bool fCanSupport;
    Consensus::ConsensusParamType type;
    unsigned int id;
};

struct ConsultationConsensusEntry {
    CConsultation consultation;
    QVector<ConsultationAnswerEntry> answers;
    QStringList myVotes;
    bool fCanVote;
};

class DaoPage : public QWidget
{
    Q_OBJECT

public:
    explicit DaoPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    void setWalletModel(WalletModel *walletModel);
    void setClientModel(ClientModel *clientModel);
    void initialize(CProposalMap proposalMap, CPaymentRequestMap paymentRequestMap, CConsultationMap consultationMap, CConsultationAnswerMap consultationAnswerMap, int unit, bool updateFilterIfEmpty);
    void setActive(bool flag);
    void setView(int view);

    void setData(QVector<ProposalEntry>);
    void setData(QVector<PaymentRequestEntry>);
    void setData(QVector<ConsultationEntry>);
    void setData(QVector<DeploymentEntry>);
    void setData(QVector<ConsensusEntry>);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    QTableWidget* table;
    QVBoxLayout *layout;
    QVector<ProposalEntry> proposalModel;
    QVector<PaymentRequestEntry> paymentRequestModel;
    QVector<ConsultationEntry> consultationModel;
    QVector<DeploymentEntry> deploymentModel;
    QVector<ConsensusEntry> consensusModel;

    QLabel* viewLbl;
    NavCoinPushButton* proposalsBtn;
    NavCoinPushButton* paymentRequestsBtn;
    NavCoinPushButton* consultationsBtn;
    NavCoinPushButton* deploymentsBtn;
    NavCoinPushButton* consensusBtn;
    QLabel* filterLbl;
    QLabel* childFilterLbl;
    QComboBox* filterCmb;
    QComboBox* filter2Cmb;
    QCheckBox* excludeBox;
    QPushButton* createBtn;
    QPushButton* backToFilterBtn;
    QLabel* warningLbl;
    QProgressBar* cycleProgressBar;
    QMenu* contextMenu;
    QTableWidgetItem *contextItem = nullptr;
    QString contextHash;
    QString filterHash;
    int contextId;

    int64_t nLastUpdate;

    QAction* copyHash;
    QAction* openExplorerAction;
    QAction* seePaymentRequestsAction;
    QAction* seeProposalAction;
    QAction* openChart;
    QAction* proposeChange;

    CProposalMap proposalMap;
    CPaymentRequestMap paymentRequestMap;
    CConsultationMap consultationMap;
    CConsultationAnswerMap consultationAnswerMap;

    bool fActive;
    bool fExclude;
    int nView;
    int nCurrentView;
    int nCurrentUnit;
    int nFilter;
    int nFilter2;
    int nCurrentFilter;
    int nCurrentFilter2;

    int nBadgeProposals;
    int nBadgePaymentRequests;
    int nBadgeConsultations;
    int nBadgeDeployments;
    int nBadgeConsensus;

    enum {
        P_COLUMN_HASH,
        P_COLUMN_COLOR,
        P_COLUMN_TITLE,
        P_COLUMN_REQUESTS,
        P_COLUMN_PAID,
        P_COLUMN_DURATION,
        P_COLUMN_VOTES,
        P_COLUMN_STATE,
        P_COLUMN_PADDING2,
        P_COLUMN_MY_VOTES,
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
        PR_COLUMN_PADDING2,
        PR_COLUMN_MY_VOTES,
        PR_COLUMN_VOTE,
        PR_COLUMN_PADDING3
    };

    enum {
        C_COLUMN_HASH,
        C_COLUMN_COLOR,
        C_COLUMN_TITLE,
        C_COLUMN_ANSWERS,
        C_COLUMN_STATUS,
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
        CP_COLUMN_HASH,
        CP_COLUMN_ID,
        CP_COLUMN_COLOR,
        CP_COLUMN_TITLE,
        CP_COLUMN_CURRENT,
        CP_COLUMN_STATUS,
        CP_COLUMN_PROPOSALS,
        CP_COLUMN_PADDING2,
        CP_COLUMN_MY_VOTES,
        CP_COLUMN_VOTE,
        CP_COLUMN_PADDING3
    };

    enum {
        VIEW_PROPOSALS,
        VIEW_PAYMENT_REQUESTS,
        VIEW_CONSULTATIONS,
        VIEW_DEPLOYMENTS,
        VIEW_CONSENSUS
    };

    enum {
        FILTER_ALL,
        FILTER_NOT_VOTED,
        FILTER_VOTED,
    };

    enum {
        FILTER2_ALL,
        FILTER2_IN_PROGRESS,
        FILTER2_FINISHED,
        FILTER2_LOOKING_FOR_SUPPORT
    };

private Q_SLOTS:
    void viewProposals();
    void viewPaymentRequests();
    void viewConsultations();
    void viewDeployments();
    void viewConsensus();

    void onCreate();
    void onVote();
    void onDetails();
    void onFilter(int index);
    void onFilter2(int index);
    void onExclude(bool fChecked);
    void onSupportAnswer();
    void onCopyHash();
    void onSeeProposal();
    void onSeePaymentRequests();
    void onViewChart();

    void refresh(bool force = false, bool updateFilterIfEmpty = false);
    void refreshForce();

    void backToFilter();

    void setWarning(QString text);

    void showContextMenu(const QPoint& pt);
};

class DaoChart : public QDialog
{
    Q_OBJECT

public:
    DaoChart(QWidget *parent, QString title, std::map<QString, uint64_t> mapVotes);

private:
    QVBoxLayout *layout;
};

#endif // DAOPAGE_H
