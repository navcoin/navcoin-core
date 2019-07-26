// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daopage.h"

#include "timedata.h"

DaoPage::DaoPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    clientModel(0),
    walletModel(0),
    table(new QTableWidget),
    layout(new QVBoxLayout),
    contextMenu(new QMenu),
    fActive(false),
    fExclude(false),
    nView(VIEW_PROPOSALS),
    nCurrentView(-1),
    nCurrentUnit(DEFAULT_UNIT),
    nFilter(FILTER_ALL),
    nFilter2(FILTER_ALL),
    nCurrentFilter(FILTER_ALL),
    nCurrentFilter2(FILTER_ALL)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);

    auto *topBox = new QFrame;
    auto *topBoxLayout = new QHBoxLayout;
    topBoxLayout->setContentsMargins(QMargins());
    topBox->setLayout(topBoxLayout);

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    viewLbl = new QLabel(tr("View:"));
    proposalsBtn = new NavCoinPushButton(tr("Fund Proposals"));
    paymentRequestsBtn = new NavCoinPushButton(tr("Payment Requests"));
    consultationsBtn = new NavCoinPushButton(tr("Consultations"));
    deploymentsBtn = new NavCoinPushButton(tr("Deployments"));
    consensusBtn = new NavCoinPushButton(tr("Consensus Parameters"));
    filterLbl = new QLabel(tr("Filter:"));
    filterCmb = new QComboBox();
    filter2Cmb = new QComboBox();
    createBtn = new QPushButton(tr("Create New"));
    excludeBox = new QCheckBox(tr("Exclude finished and expired"));
    childFilterLbl = new QLabel();
    backToFilterBtn = new QPushButton(tr("Back"));
    warningLbl = new QLabel();

    warningLbl->setObjectName("warning");

    childFilterLbl->setVisible(false);
    backToFilterBtn->setVisible(false);
    warningLbl->setVisible(false);

    connect(proposalsBtn, SIGNAL(clicked()), this, SLOT(viewProposals()));
    connect(paymentRequestsBtn, SIGNAL(clicked()), this, SLOT(viewPaymentRequests()));
    connect(consultationsBtn, SIGNAL(clicked()), this, SLOT(viewConsultations()));
    connect(deploymentsBtn, SIGNAL(clicked()), this, SLOT(viewDeployments()));
    connect(consensusBtn, SIGNAL(clicked()), this, SLOT(viewConsensus()));
    connect(createBtn, SIGNAL(clicked()), this, SLOT(onCreate()));
    connect(filterCmb, SIGNAL(activated(int)), this, SLOT(onFilter(int)));
    connect(filter2Cmb, SIGNAL(activated(int)), this, SLOT(onFilter2(int)));
    connect(excludeBox, SIGNAL(toggled(bool)), this, SLOT(onExclude(bool)));
    connect(backToFilterBtn, SIGNAL(clicked()), this, SLOT(backToFilter()));

    excludeBox->setChecked(true);

    topBoxLayout->addSpacing(30);
    topBoxLayout->addWidget(viewLbl, 0, Qt::AlignLeft);
    topBoxLayout->addSpacing(30);
    topBoxLayout->addWidget(proposalsBtn);
    topBoxLayout->addWidget(paymentRequestsBtn);
    topBoxLayout->addWidget(consultationsBtn);
    topBoxLayout->addWidget(deploymentsBtn);
    topBoxLayout->addWidget(consensusBtn);
    topBoxLayout->addStretch();

    bottomBoxLayout->addSpacing(30);
    bottomBoxLayout->addWidget(filterLbl, 0, Qt::AlignLeft);
    bottomBoxLayout->addWidget(childFilterLbl);
    bottomBoxLayout->addWidget(backToFilterBtn);
    bottomBoxLayout->addSpacing(38);
    bottomBoxLayout->addWidget(filterCmb, 0, Qt::AlignLeft);
    bottomBoxLayout->addSpacing(30);
    bottomBoxLayout->addWidget(filter2Cmb, 0, Qt::AlignLeft);
    bottomBoxLayout->addSpacing(30);
    bottomBoxLayout->addWidget(excludeBox);
    bottomBoxLayout->addWidget(warningLbl);
    bottomBoxLayout->addStretch();
    bottomBoxLayout->addWidget(createBtn);

    layout->addWidget(topBox);
    layout->addSpacing(15);
    layout->addWidget(table);
    layout->addWidget(bottomBox);

    table->setContentsMargins(QMargins());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->verticalHeader()->setDefaultSectionSize(78);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->horizontalHeader()->setSortIndicatorShown(true);
    table->horizontalHeader()->setSectionsClickable(true);
    table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(table, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showContextMenu(const QPoint &)));

    copyHash = new QAction(tr("Copy hash"), this);
    openExplorerAction = new QAction(tr("Open in explorer"), this);
    proposeChange = new QAction(tr("Propose Change"), this);
    seeProposalAction = new QAction(tr("Go to parent proposal"), this);
    seePaymentRequestsAction = new QAction(tr("Show payment requests from this proposal"), this);
    openChart = new QAction(tr("View votes"), this);

    contextMenu->addAction(copyHash);
    contextMenu->addAction(openExplorerAction);
    contextMenu->addSeparator();
    contextMenu->addAction(openChart);
    contextMenu->addSeparator();

    connect(seeProposalAction, SIGNAL(triggered()), this, SLOT(onSeeProposal()));
    connect(proposeChange, SIGNAL(triggered()), this, SLOT(onCreate()));
    connect(seePaymentRequestsAction, SIGNAL(triggered()), this, SLOT(onSeePaymentRequests()));
    connect(copyHash, SIGNAL(triggered()), this, SLOT(onCopyHash()));
    connect(openExplorerAction, SIGNAL(triggered()), this, SLOT(onDetails()));
    connect(openChart, SIGNAL(triggered()), this, SLOT(onViewChart()));

    viewProposals();
}

void DaoPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    refresh(true);
}

void DaoPage::setWarning(QString text)
{
    warningLbl->setText(text);
    warningLbl->setVisible(!text.isEmpty());
}

void DaoPage::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    if (clientModel) {
        connect(clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(refreshForce()));
    }
}

void DaoPage::setActive(bool flag)
{
    fActive = flag;
}

void DaoPage::setView(int view)
{
    nView = view;

    refresh(true);
}

void DaoPage::refreshForce()
{
    if (GetAdjustedTime() - nLastUpdate > 15)
    {
        nLastUpdate = GetAdjustedTime();
        setWarning("");
        refresh(true);
    }
}

void DaoPage::refresh(bool force, bool updateFilterIfEmpty)
{
    int unit = DEFAULT_UNIT;

    {
    LOCK(cs_main);

    if (pcoinsTip == nullptr)
        return;

    if (!pcoinsTip->GetAllProposals(proposalMap) || !pcoinsTip->GetAllPaymentRequests(paymentRequestMap) ||
            !pcoinsTip->GetAllConsultations(consultationMap) || !pcoinsTip->GetAllConsultationAnswers(consultationAnswerMap))
        return;

    if (clientModel)
    {
        OptionsModel* optionsModel = clientModel->getOptionsModel();

        if(optionsModel)
        {
            unit = optionsModel->getDisplayUnit();
        }
    }
    }

    if (!force && unit == nCurrentUnit && nFilter == nCurrentFilter && nFilter2 == nCurrentFilter2 &&
            ((nCurrentView == VIEW_PROPOSALS && proposalMap.size() == proposalModel.size()) ||
             (nCurrentView == VIEW_PAYMENT_REQUESTS &&  paymentRequestMap.size() == paymentRequestModel.size()) ||
             (nCurrentView == VIEW_CONSULTATIONS && consultationMap.size() == consultationModel.size())))
        return;

    initialize(proposalMap, paymentRequestMap, consultationMap, consultationAnswerMap, unit, updateFilterIfEmpty);
}

void DaoPage::initialize(CProposalMap proposalMap, CPaymentRequestMap paymentRequestMap, CConsultationMap consultationMap, CConsultationAnswerMap consultationAnswerMap, int unit, bool updateFilterIfEmpty)
{

    LOCK(cs_main);

    nCurrentUnit = unit;

    if (nView != nCurrentView)
    {
        while (filterCmb->count() > 0)
            filterCmb->removeItem(0);

        while (filter2Cmb->count() > 0)
            filter2Cmb->removeItem(0);

        nCurrentView = nView;
        nCurrentFilter = nFilter;
        nCurrentFilter2 = nFilter2;

        table->setRowCount(0);

        if (nCurrentView == VIEW_PROPOSALS)
        {
            createBtn->setVisible(true);
            createBtn->setText(tr("Create new Proposal"));

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");

            if (filterHash != "")
            {
                filterCmb->setVisible(false);
                filter2Cmb->setVisible(false);
                excludeBox->setVisible(false);
                backToFilterBtn->setVisible(true);
                childFilterLbl->setVisible(true);
                childFilterLbl->setText(tr("Showing proposal %1").arg(filterHash));
            }
            else
            {
                filterCmb->setVisible(true);
                filter2Cmb->setVisible(true);
                excludeBox->setVisible(true);
                backToFilterBtn->setVisible(false);
                childFilterLbl->setVisible(false);
            }

            table->setColumnCount(P_COLUMN_PADDING3 + 1);
            table->setColumnHidden(P_COLUMN_HASH, true);
            table->setColumnHidden(P_COLUMN_COLOR, false);
            table->setHorizontalHeaderLabels({ "", "", tr("Name"), tr("Requests"), tr("Paid"), tr("Duration"), tr("Votes"), tr("State"), "", tr("My Votes"), "", "" });
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_TITLE, QHeaderView::Stretch);
            table->horizontalHeaderItem(P_COLUMN_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_REQUESTS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_PAID, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_DURATION, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_MY_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_STATE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_COLOR, QHeaderView::Fixed);
            table->setColumnWidth(P_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(P_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(P_COLUMN_PADDING3, 12);
        }
        else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
        {
            createBtn->setVisible(true);
            createBtn->setText(tr("Create new Payment Request"));

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");

            if (filterHash != "")
            {
                filterCmb->setVisible(false);
                filter2Cmb->setVisible(false);
                excludeBox->setVisible(false);
                backToFilterBtn->setVisible(true);
                childFilterLbl->setVisible(true);
                childFilterLbl->setText(tr("Showing payment requests of %1").arg(filterHash));
            }
            else
            {
                filterCmb->setVisible(true);
                filter2Cmb->setVisible(true);
                excludeBox->setVisible(true);
                backToFilterBtn->setVisible(false);
                childFilterLbl->setVisible(false);
            }

            table->setColumnCount(PR_COLUMN_PADDING3 + 1);
            table->setColumnHidden(PR_COLUMN_HASH, true);
            table->setColumnHidden(PR_COLUMN_COLOR, false);
            table->setHorizontalHeaderLabels({ "", "", tr("Proposal"), tr("Description"), tr("Requests"), tr("Votes"), tr("State"), "", tr("My Votes"), "", ""});
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_PARENT_TITLE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_TITLE, QHeaderView::Stretch);
            table->horizontalHeaderItem(PR_COLUMN_PARENT_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_REQUESTS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_MY_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_STATE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_COLOR, QHeaderView::Fixed);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->setColumnWidth(PR_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(PR_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(PR_COLUMN_PADDING3, 12);
        }
        else if (nCurrentView == VIEW_CONSULTATIONS)
        {
            createBtn->setVisible(true);
            createBtn->setText(tr("Create new Consultation"));

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");
            filter2Cmb->insertItem(FILTER2_LOOKING_FOR_SUPPORT, "Looking for support");

            table->setColumnCount(C_COLUMN_PADDING3 + 1);
            table->setColumnHidden(C_COLUMN_HASH, true);
            table->setColumnHidden(C_COLUMN_COLOR, false);
            table->setHorizontalHeaderLabels({ "", "", tr("Question"), tr("Possible Answers"), tr("Status"), "", tr("My Votes"), "", ""});
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_TITLE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_ANSWERS, QHeaderView::Stretch);
            table->horizontalHeaderItem(C_COLUMN_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_STATUS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_MY_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_COLOR, QHeaderView::Fixed);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->setColumnWidth(C_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(C_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(C_COLUMN_PADDING3, 12);
        }
        else if (nCurrentView == VIEW_DEPLOYMENTS)
        {
            createBtn->setVisible(false);

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");

            table->setColumnCount(D_COLUMN_PADDING3 + 1);
            table->setColumnHidden(D_COLUMN_COLOR, false);
            table->setColumnHidden(D_COLUMN_TITLE, false);
            table->setHorizontalHeaderLabels({"", tr("Name"), tr("Status"), "", tr("My Vote"), "", "" });
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_TITLE, QHeaderView::Stretch);
            table->horizontalHeaderItem(D_COLUMN_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeaderItem(D_COLUMN_STATUS)->setTextAlignment(Qt::AlignCenter);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_STATUS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_MY_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_COLOR, QHeaderView::Fixed);
            table->setColumnWidth(D_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(D_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(D_COLUMN_PADDING3, 12);
        }
        else if (nCurrentView == VIEW_CONSENSUS)
        {
            createBtn->setVisible(false);

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");

            table->setColumnCount(CP_COLUMN_PADDING3 + 1);
            table->setColumnHidden(CP_COLUMN_COLOR, false);
            table->setColumnHidden(CP_COLUMN_ID, true);
            table->setColumnHidden(CP_COLUMN_HASH, true);
            table->setHorizontalHeaderLabels({"", "", "", tr("Name"), tr("Current value"), tr("Status"), tr("Proposed values"), "", tr("My votes"), "", "" });
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_TITLE, QHeaderView::Stretch);
            table->horizontalHeaderItem(CP_COLUMN_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeaderItem(CP_COLUMN_STATUS)->setTextAlignment(Qt::AlignCenter);
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_STATUS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_CURRENT, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_PROPOSALS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_MY_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_COLOR, QHeaderView::Fixed);
            table->setColumnWidth(CP_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(CP_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(CP_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(CP_COLUMN_PADDING3, 12);
        }

        onFilter(FILTER_ALL);
    }

    CStateViewCache coins(pcoinsTip);

    nBadgeProposals = nBadgePaymentRequests = nBadgeConsultations = nBadgeDeployments = nBadgeConsensus = 0;

    proposalModel.clear();

    for (auto& it: proposalMap)
    {
        CProposal proposal = it.second;

        uint64_t nVote = -10000;
        auto v = mapAddedVotes.find(proposal.hash);

        if (v != mapAddedVotes.end())
        {
            nVote = v->second;
        }

        if (proposal.CanVote() && nVote == -10000)
            nBadgeProposals++;

        if (!filterHash.isEmpty())
        {
            if (proposal.hash != uint256S(filterHash.toStdString()))
                continue;
        }
        else
        {
            if (nFilter != FILTER_ALL)
            {
                if (nFilter == FILTER_VOTED && nVote == -10000)
                    continue;
                if (nFilter == FILTER_NOT_VOTED && nVote != -10000)
                    continue;
            }
            if (nFilter2 != FILTER2_ALL)
            {
                if (nFilter2 == FILTER2_IN_PROGRESS && !proposal.CanVote())
                    continue;
                if (nFilter2 == FILTER2_FINISHED && proposal.CanVote())
                    continue;
            }
        }

        if (fExclude && proposal.IsExpired(chainActive.Tip()->GetBlockTime()))
            continue;

        if (mapBlockIndex.count(proposal.txblockhash) == 0)
            continue;

        uint64_t deadline = proposal.nDeadline;
        uint64_t deadline_d = std::floor(deadline/86400);
        uint64_t deadline_h = std::floor((deadline-deadline_d*86400)/3600);
        uint64_t deadline_m = std::floor((deadline-(deadline_d*86400 + deadline_h*3600))/60);

        // Show appropriate amount of figures
        std::string s_deadline = "";
        if(deadline_d >= 14)
            s_deadline = std::to_string(deadline_d) + std::string(" Days");
        else
            s_deadline = std::to_string(deadline_d) + std::string(" Days ") + std::to_string(deadline_h) + std::string(" Hours ") + std::to_string(deadline_m) + std::string(" Minutes");

        ProposalEntry p = {
            it.first,
            "#6666ff",
            QString::fromStdString(proposal.strDZeel),
            NavCoinUnits::formatWithUnit(unit, proposal.nAmount, false, NavCoinUnits::separatorAlways),
            NavCoinUnits::formatWithUnit(unit, proposal.nAmount - proposal.GetAvailable(coins), false, NavCoinUnits::separatorAlways),
            QString::fromStdString(s_deadline),
            proposal.nVotesYes ? proposal.nVotesYes : 0,
            proposal.nVotesNo ? proposal.nVotesNo : 0,
            proposal.nVotesAbs ? proposal.nVotesAbs : 0,
            proposal.nVotingCycle,
            QString::fromStdString(proposal.GetState(chainActive.Tip()->GetBlockTime())),
            proposal.CanVote(),
            nVote,
            (uint64_t)mapBlockIndex[proposal.txblockhash]->GetBlockTime()
        };

        switch (proposal.fState)
        {
        case DAOFlags::NIL:
        {
            p.color = "#ffdd66";
            break;
        }
        case DAOFlags::ACCEPTED:
        {
            p.color = "#66ff66";
            break;
        }
        case DAOFlags::REJECTED:
        {
            p.color = "#ff6666";
            break;
        }
        case DAOFlags::EXPIRED:
        {
            p.color = "#ff6600";
            break;
        }
        }

        proposalModel << p;
    }

    std::sort(proposalModel.begin(), proposalModel.end(), [](const ProposalEntry &a, const ProposalEntry &b) {
        if (a.ts == b.ts)
            return a.hash > b.hash;
        else
            return a.ts > b.ts;
    });

    paymentRequestModel.clear();

    for (auto& it: paymentRequestMap)
    {
        CProposal proposal;
        CPaymentRequest prequest = it.second;

        uint64_t nVote = -10000;
        auto v = mapAddedVotes.find(prequest.hash);

        if (v != mapAddedVotes.end())
        {
            nVote = v->second;
        }

        if (prequest.CanVote(coins) && nVote == -10000)
            nBadgePaymentRequests++;

        if (!filterHash.isEmpty())
        {
            if (prequest.proposalhash != uint256S(filterHash.toStdString()))
                continue;
        }
        else
        {
            if (nFilter != FILTER_ALL)
            {
                if (nFilter == FILTER_VOTED && nVote == -10000)
                    continue;
                if (nFilter == FILTER_NOT_VOTED && nVote != -10000)
                    continue;
            }

            if (nFilter2 != FILTER2_ALL)
            {
                if (nFilter2 == FILTER2_IN_PROGRESS && !prequest.CanVote(coins))
                    continue;
                if (nFilter2 == FILTER2_FINISHED && prequest.CanVote(coins))
                    continue;
            }
        }

        if (fExclude && prequest.IsExpired())
            continue;

        if (mapBlockIndex.count(prequest.txblockhash) == 0)
            continue;

        if (!coins.GetProposal(prequest.proposalhash, proposal))
            continue;

        PaymentRequestEntry p = {
            it.first,
            QString::fromStdString(prequest.strDZeel),
            QString::fromStdString(proposal.strDZeel),
            "#6666ff",
            NavCoinUnits::formatWithUnit(unit, prequest.nAmount, false, NavCoinUnits::separatorAlways),
            prequest.nVotesYes ? prequest.nVotesYes : 0,
            prequest.nVotesNo ? prequest.nVotesNo : 0,
            prequest.nVotesAbs ? prequest.nVotesAbs : 0,
            proposal.nVotingCycle,
            QString::fromStdString(prequest.GetState()),
            prequest.CanVote(coins),
            nVote,
            (uint64_t)mapBlockIndex[prequest.txblockhash]->GetBlockTime()
        };

        switch (prequest.fState)
        {
        case DAOFlags::NIL:
        {
            p.color = "#ffdd66";
            break;
        }
        case DAOFlags::ACCEPTED:
        {
            p.color = "#66ff66";
            break;
        }
        case DAOFlags::REJECTED:
        {
            p.color = "#ff6666";
            break;
        }
        case DAOFlags::EXPIRED:
        {
            p.color = "#ff6600";
            break;
        }
        }

        paymentRequestModel << p;
    }

    std::sort(paymentRequestModel.begin(), paymentRequestModel.end(), [](const PaymentRequestEntry &a, const PaymentRequestEntry &b) {
        if (a.ts == b.ts)
            return a.hash > b.hash;
        else
            return a.ts > b.ts;
    });

    consultationModel.clear();

    std::map<int, ConsultationConsensusEntry> mapConsultationConsensus;

    for (auto& it: consultationMap)
    {
        CConsultation consultation = it.second;

        if (mapBlockIndex.count(consultation.txblockhash) == 0)
            continue;

        CBlockIndex* pindex = mapBlockIndex[consultation.txblockhash];

        uint64_t nVote = -10000;
        auto v = mapAddedVotes.find(consultation.hash);

        if (v != mapAddedVotes.end())
        {
            nVote = v->second;
        }

        QVector<ConsultationAnswerEntry> answers;
        QStringList myVotes;

        if (nVote == -1)
        {
            myVotes << tr("Abstain");
        }

        if (!consultation.IsRange())
        {
            for (auto& it2: consultationAnswerMap)
            {
                CConsultationAnswer answer;

                if (!pcoinsTip->GetConsultationAnswer(it2.first, answer))
                    continue;

                if (answer.parent != consultation.hash)
                    continue;

                if (!answer.IsSupported() && consultation.fState != DAOFlags::NIL)
                    continue;

                if (!consultation.IsRange())
                {
                    auto v = mapAddedVotes.find(answer.hash);

                    if (v != mapAddedVotes.end() && v->second == 1)
                    {
                        myVotes << QString::fromStdString(answer.sAnswer);
                    }
                }

                ConsultationAnswerEntry a = {
                    answer.hash,
                    QString::fromStdString(answer.sAnswer),
                    answer.nVotes,
                    QString::fromStdString(answer.GetState()),
                    answer.CanBeVoted(coins),
                    answer.CanBeSupported(coins),
                    answer.CanBeSupported(coins) && mapSupported.count(answer.hash) ? tr("Supported") : tr("")
                };

                answers << a;
            }
        }

        bool fSupported = mapSupported.count(consultation.hash);

        if (consultation.CanBeSupported())
        {
            if (fSupported)
                myVotes << tr("Supported");
        }
        else if (consultation.IsRange() && nVote != -10000)
        {
            myVotes << QString::number(nVote);
        }

        if (consultation.IsRange())
        {
            std::sort(answers.begin(), answers.end(), [](const ConsultationAnswerEntry &a, const ConsultationAnswerEntry &b) {
                return a.answer.toInt() < b.answer.toInt();
            });
        }
        else
        {
            std::sort(answers.begin(), answers.end(), [](const ConsultationAnswerEntry &a, const ConsultationAnswerEntry &b) {
                return a.answer.toInt() < b.answer.toInt();
            });
        }

        myVotes.sort();

        bool fCanVote = consultation.CanBeVoted();

        if (!consultation.IsRange())
        {
            fCanVote = true;

            for (ConsultationAnswerEntry &a: answers)
            {
                if (!a.fCanVote)
                {
                    fCanVote = false;
                    break;
                }
            }
        }

        if (consultation.IsAboutConsensusParameter())
        {
            if (mapConsultationConsensus.count(consultation.nMin) == 0)
            {
                mapConsultationConsensus[consultation.nMin].consultation = consultation;
                mapConsultationConsensus[consultation.nMin].answers = answers;
                mapConsultationConsensus[consultation.nMin].myVotes = myVotes;
                mapConsultationConsensus[consultation.nMin].fCanVote = fCanVote;
            }
            else if (mapBlockIndex.count(mapConsultationConsensus[consultation.nMin].consultation.txblockhash))
            {
                if (pindex->GetBlockTime() > mapBlockIndex[mapConsultationConsensus[consultation.nMin].consultation.txblockhash]->GetBlockTime())
                {
                    mapConsultationConsensus[consultation.nMin].consultation = consultation;
                    mapConsultationConsensus[consultation.nMin].answers = answers;
                    mapConsultationConsensus[consultation.nMin].myVotes = myVotes;
                    mapConsultationConsensus[consultation.nMin].fCanVote = fCanVote;
                }
            }
        }

        ConsultationEntry p = {
            it.first,
            "#6666ff",
            QString::fromStdString(consultation.strDZeel),
            answers,
            "",
            consultation.nVotingCycle,
            QString::fromStdString(consultation.GetState(chainActive.Tip())),
            consultation.fState,
            fCanVote,
            myVotes,
            (uint64_t)mapBlockIndex[consultation.txblockhash]->GetBlockTime(),
            consultation.IsRange(),
            consultation.nMin,
            consultation.nMax,
            consultation.CanBeSupported()
        };

        switch (consultation.fState)
        {
        case DAOFlags::NIL:
        {
            p.color = "#ffdd66";
            break;
        }
        case DAOFlags::ACCEPTED:
        {
            p.color = "#66ff66";
            break;
        }
        case DAOFlags::EXPIRED:
        {
            p.color = "#ff6666";
            break;
        }
        }

        if (consultation.IsAboutConsensusParameter())
            continue;

        if (consultation.fState == DAOFlags::ACCEPTED &&
          ((nVote == -10000 && consultation.IsRange()) || (!consultation.IsRange() && myVotes.size() == 0)))
            nBadgeConsultations++;

        if (nFilter != FILTER_ALL)
        {
            if (nFilter == FILTER_VOTED && nVote == -10000 && myVotes.size() == 0)
                continue;
            if (nFilter == FILTER_NOT_VOTED && (nVote != -10000 || myVotes.size() > 0))
                continue;
        }
        if (nFilter2 != FILTER2_ALL)
        {
            if (nFilter2 == FILTER2_IN_PROGRESS && consultation.fState != DAOFlags::ACCEPTED)
                continue;
            if (nFilter2 == FILTER2_FINISHED && consultation.fState != DAOFlags::EXPIRED)
                continue;
            if ((consultation.CanBeSupported() && nFilter2 != FILTER2_LOOKING_FOR_SUPPORT) ||
                (!consultation.CanBeSupported() && nFilter2 == FILTER2_LOOKING_FOR_SUPPORT))
                continue;
        }

        if (fExclude && consultation.IsExpired(chainActive.Tip()))
            continue;

        consultationModel << p;
    }

    std::sort(consultationModel.begin(), consultationModel.end(), [](const ConsultationEntry &a, const ConsultationEntry &b) {
        if (a.ts == b.ts)
            return a.hash > b.hash;
        else
            return a.ts > b.ts;
    });

    deploymentModel.clear();

    const Consensus::Params& consensusParams = Params().GetConsensus();

    for (unsigned int i = 0; i < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++)
    {
        QString status = "unknown";

        Consensus::DeploymentPos id = (Consensus::DeploymentPos)i;

        bool fFinished = false;
        bool fInProgress = false;
        bool nVoted = false;

        if (!consensusParams.vDeployments[id].nStartTime)
            continue;

        std::vector<std::string>& versionBitVotesRejected = mapMultiArgs["-rejectversionbit"];
        std::vector<std::string>& versionBitVotesAccepted = mapMultiArgs["-acceptversionbit"];

        for (auto& it: versionBitVotesRejected)
        {
            if (stoi(it) == consensusParams.vDeployments[id].bit)
            {
                nVoted = true;
                break;
            }
        }

        if (!nVoted)
        {
            for (auto& it: versionBitVotesAccepted)
            {
                if (stoi(it) == consensusParams.vDeployments[id].bit)
                {
                    nVoted = true;
                    break;
                }
            }
        }

        const ThresholdState thresholdState = VersionBitsTipState(consensusParams, id);
        switch (thresholdState) {
        case THRESHOLD_DEFINED: status =  "defined"; fFinished = false; fInProgress = false; break;
        case THRESHOLD_STARTED: status = "started"; fFinished = false; fInProgress = true; break;
        case THRESHOLD_LOCKED_IN: status = "locked in"; fFinished = true; fInProgress = false; break;
        case THRESHOLD_ACTIVE: status = "active"; fFinished = true; fInProgress = false; break;
        case THRESHOLD_FAILED: status = "failed"; fFinished = true; fInProgress = false; break;
        }

        if (fInProgress && !nVoted)
            nBadgeDeployments++;

        if (nFilter != FILTER_ALL)
        {
            if (nFilter == FILTER_VOTED && !nVoted)
                continue;
            if (nFilter == FILTER_NOT_VOTED && nVoted)
                continue;
        }
        if (nFilter2 != FILTER2_ALL)
        {
            if (nFilter2 == FILTER2_IN_PROGRESS && !fInProgress)
                continue;
            if (nFilter2 == FILTER2_FINISHED && !fFinished)
                continue;
        }

        bool nVote = !IsVersionBitRejected(consensusParams, id);

        DeploymentEntry e = {
            "#6666ff",
            QString::fromStdString(Consensus::sDeploymentsDesc[id]),
            status,
            status == "started",
            nVote,
            !nVoted,
            consensusParams.vDeployments[id].nStartTime,
            consensusParams.vDeployments[id].bit
        };

        if (status == "started")
        {
            e.color = "#ffdd66";
        }
        else if (status == "locked_in")
        {
            e.color = "#ff66ff";
        }
        else if (status == "active")
        {
            e.color = "#66ff66";
        }
        else if (status == "failed")
        {
            e.color = "#ff6666";
        }

        deploymentModel << e;
    }

    std::sort(deploymentModel.begin(), deploymentModel.end(), [](const DeploymentEntry &a, const DeploymentEntry &b) {
        if (a.ts == b.ts)
            return a.title > b.title;
        else
            return a.ts > b.ts;
    });

    consensusModel.clear();

    for (unsigned int i = 0; i < Consensus::MAX_CONSENSUS_PARAMS; i++)
    {
        Consensus::ConsensusParamsPos id = (Consensus::ConsensusParamsPos)i;

        QString status = "set";

        ConsultationConsensusEntry consultation;
        consultation.consultation.SetNull();

        if (mapConsultationConsensus.count(i))
            consultation = mapConsultationConsensus[i];

        bool fHasConsultation = !consultation.consultation.IsNull() && consultation.consultation.fState != DAOFlags::EXPIRED && consultation.consultation.fState != DAOFlags::PASSED;

        if (fHasConsultation && (consultation.consultation.CanBeSupported() || consultation.consultation.fState == DAOFlags::REFLECTION))
            status = tr("change proposed") + " " + QString::fromStdString(consultation.consultation.GetState(chainActive.Tip()));
        else if (fHasConsultation && consultation.consultation.fState == DAOFlags::ACCEPTED)
            status = tr("voting");

        if (fHasConsultation && consultation.myVotes.size() == 0 && consultation.consultation.fState == DAOFlags::ACCEPTED)
            nBadgeConsensus++;

        if (nFilter != FILTER_ALL)
        {
            if (nFilter == FILTER_VOTED && consultation.myVotes.size() == 0)
                continue;
            if (nFilter == FILTER_NOT_VOTED && consultation.myVotes.size() > 0)
                continue;
        }

        if (nFilter2 != FILTER2_ALL)
        {
            if (nFilter2 == FILTER2_IN_PROGRESS && !fHasConsultation)
                continue;
            if (nFilter2 == FILTER2_FINISHED && fHasConsultation)
                continue;
        }

        QVector<ConsultationAnswerEntry> answers;
        QStringList myVotes;

        ConsensusEntry e = {
            fHasConsultation ? consultation.consultation.hash : uint256(),
            "#6666ff",
            QString::fromStdString(Consensus::sConsensusParamsDesc[id]),
            GetConsensusParameter(id),
            consultation.answers,
            0,
            status,
            consultation.consultation.fState,
            fHasConsultation,
            consultation.myVotes,
            fHasConsultation && consultation.consultation.CanBeSupported(),
            Consensus::vConsensusParamsType[id],
            i
        };

        if (status == "started")
        {
            e.color = "#ffdd66";
        }
        else if (status == "locked_in")
        {
            e.color = "#ff66ff";
        }
        else if (status == "active")
        {
            e.color = "#66ff66";
        }
        else if (status == "failed")
        {
            e.color = "#ff6666";
        }

        consensusModel << e;
    }

    if (nCurrentView == VIEW_PROPOSALS)
    {
        if (updateFilterIfEmpty && proposalModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(proposalModel);
    }
    else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
    {
        if (updateFilterIfEmpty && paymentRequestModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(paymentRequestModel);
    }
    else if (nCurrentView == VIEW_CONSULTATIONS)
    {
        if (updateFilterIfEmpty && consultationModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(consultationModel);
    }
    else if (nCurrentView == VIEW_DEPLOYMENTS)
    {
        if (updateFilterIfEmpty && deploymentModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(deploymentModel);
    }
    else if (nCurrentView == VIEW_CONSENSUS)
    {
        if (updateFilterIfEmpty && consensusModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(consensusModel);
    }

    proposalsBtn->setBadge(nBadgeProposals);
    paymentRequestsBtn->setBadge(nBadgePaymentRequests);
    consultationsBtn->setBadge(nBadgeConsultations);
    deploymentsBtn->setBadge(nBadgeDeployments);
    consensusBtn->setBadge(nBadgeConsensus);
}

void DaoPage::setData(QVector<ProposalEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        auto &entry = data[i];

        // HASH
        auto *hashItem = new QTableWidgetItem;
        hashItem->setData(Qt::DisplayRole, QString::fromStdString(entry.hash.GetHex()));
        table->setItem(i, P_COLUMN_HASH, hashItem);

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, P_COLUMN_COLOR, indicatorBox);
        table->setItem(i, P_COLUMN_COLOR, colorItem);

        // TITLE
        auto *nameItem = new QTableWidgetItem;
        nameItem->setData(Qt::DisplayRole, entry.title);
        table->setItem(i, P_COLUMN_TITLE, nameItem);

        // AMOUNT
        auto *amountItem = new QTableWidgetItem;
        amountItem->setData(Qt::DisplayRole, entry.requests);
        amountItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_REQUESTS, amountItem);

        // PAID
        auto *paidItem = new QTableWidgetItem;
        paidItem->setData(Qt::DisplayRole, entry.paid);
        paidItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_PAID, paidItem);

        // DEADLINE
        auto *deadlineItem = new QTableWidgetItem;
        deadlineItem->setData(Qt::DisplayRole, entry.deadline);
        deadlineItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_DURATION, deadlineItem);

        // VOTES
        auto *votesItem = new QTableWidgetItem;
        votesItem->setData(Qt::DisplayRole, QString::number(entry.nVotesYes) + "/" + QString::number(entry.nVotesNo) + "/" + QString::number(entry.nVotesAbs));
        votesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_VOTES, votesItem);

        // STATE
        auto *stateItem = new QTableWidgetItem;
        stateItem->setData(Qt::DisplayRole, entry.sState);
        stateItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_STATE, stateItem);

        // MY VOTES
        auto *myVotesItem = new QTableWidgetItem;
        auto voteText = QString();

        if (entry.nMyVote == 0)
        {
            voteText = tr("Voted No.");
        }
        else if (entry.nMyVote == 1)
        {
            voteText = tr("Voted Yes.");
        }
        else if (entry.nMyVote == -1)
        {
            voteText = tr("Abstaining.");
        }
        else if (entry.fCanVote)
        {
            voteText = tr("Did not vote.");
        }
        else
        {
            voteText = tr("Voting finished.");
        }

        myVotesItem->setData(Qt::DisplayRole, voteText);
        myVotesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_MY_VOTES, myVotesItem);

        //VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote) {
            auto *button = new QPushButton;
            button->setText(voteText.isEmpty() ? tr("Vote") : tr("Change"));
            button->setFixedHeight(40);
            button->setProperty("id", QString::fromStdString(entry.hash.GetHex()));
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
        }

        table->setItem(i, P_COLUMN_VOTE, votedItem);
        table->setCellWidget(i, P_COLUMN_VOTE, widget);

    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) table->setColumnWidth(P_COLUMN_VOTE, 0);
    else table->setColumnWidth(P_COLUMN_VOTE, 150);
    table->setColumnHidden(P_COLUMN_VOTE, !fWeight);
    table->setSortingEnabled(true);

}

void DaoPage::setData(QVector<PaymentRequestEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        auto &entry = data[i];

        // HASH
        auto *hashItem = new QTableWidgetItem;
        hashItem->setData(Qt::DisplayRole, QString::fromStdString(entry.hash.GetHex()));
        table->setItem(i, PR_COLUMN_HASH, hashItem);

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, PR_COLUMN_COLOR, indicatorBox);
        table->setItem(i, PR_COLUMN_COLOR, colorItem);

        // PARENT TITLE
        auto *parentNameItem = new QTableWidgetItem;
        parentNameItem->setData(Qt::DisplayRole, entry.parentTitle);
        table->setItem(i, PR_COLUMN_PARENT_TITLE, parentNameItem);

        // TITLE
        auto *nameItem = new QTableWidgetItem;
        nameItem->setData(Qt::DisplayRole, entry.title);
        nameItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_TITLE, nameItem);

        // AMOUNT
        auto *amountItem = new QTableWidgetItem;
        amountItem->setData(Qt::DisplayRole, entry.amount);
        amountItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_REQUESTS, amountItem);

        // VOTES
        auto *votesItem = new QTableWidgetItem;
        votesItem->setData(Qt::DisplayRole, QString::number(entry.nVotesYes) + "/" + QString::number(entry.nVotesNo) + "/" + QString::number(entry.nVotesAbs));
        votesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_VOTES, votesItem);

        // STATE
        auto *stateItem = new QTableWidgetItem;
        stateItem->setData(Qt::DisplayRole, entry.sState);
        stateItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_STATE, stateItem);

        // MY VOTE
        auto *myVoteItem = new QTableWidgetItem;
        auto voteText = QString();

        if (entry.nMyVote == 0)
        {
            voteText = tr("Voted No.");
        }
        else if (entry.nMyVote == 1)
        {
            voteText = tr("Voted Yes.");
        }
        else if (entry.nMyVote == -1)
        {
            voteText = tr("Abstaining.");
        }
        else if (entry.fCanVote)
        {
            voteText = tr("Did not vote.");
        }
        else
        {
            voteText = tr("Voting finished.");
        }
        myVoteItem->setData(Qt::DisplayRole, voteText);
        myVoteItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_MY_VOTES, myVoteItem);

        //VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote) {
            auto *button = new QPushButton;
            button->setText(voteText.isEmpty() ? tr("Vote") : tr("Change"));
            button->setFixedHeight(40);
            button->setProperty("id", QString::fromStdString(entry.hash.GetHex()));
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
        }

        table->setCellWidget(i, PR_COLUMN_VOTE, widget);
        table->setItem(i, PR_COLUMN_VOTE, votedItem);
    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) table->setColumnWidth(PR_COLUMN_VOTE, 0);
    else table->setColumnWidth(PR_COLUMN_VOTE, 150);
    table->setColumnHidden(PR_COLUMN_VOTE, !fWeight);
    table->setSortingEnabled(true);
}

void DaoPage::setData(QVector<ConsultationEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        ConsultationEntry &entry = data[i];

        // HASH
        auto *hashItem = new QTableWidgetItem;
        hashItem->setData(Qt::DisplayRole, QString::fromStdString(entry.hash.GetHex()));
        table->setItem(i, C_COLUMN_HASH, hashItem);

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, C_COLUMN_COLOR, indicatorBox);
        table->setItem(i, C_COLUMN_COLOR, colorItem);

        // TITLE
        auto *titleItem = new QTableWidgetItem;
        titleItem->setData(Qt::DisplayRole, entry.question);
        table->setItem(i, C_COLUMN_TITLE, titleItem);

        // ANSWERS
        QString answers;
        if(entry.fRange)
        {
            answers = QString(tr("Between %1 and %2")).arg(QString::number(entry.nMin), QString::number(entry.nMax));
        }
        else
        {
            QStringList s;
            for (auto& it: entry.answers)
            {
                s << it.answer;
            }
            s.sort();
            answers = s.join(", ");
        }

        auto *answersItem = new QTableWidgetItem;
        answersItem->setData(Qt::DisplayRole, answers);
        answersItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, C_COLUMN_ANSWERS, answersItem);

        // STATE
        auto *statusItem = new QTableWidgetItem;
        statusItem->setData(Qt::DisplayRole, entry.sState);
        table->setItem(i, C_COLUMN_STATUS, statusItem);

        // MY VOTES
        auto *myvotesItem = new QTableWidgetItem;
        myvotesItem->setData(Qt::DisplayRole, entry.myVotes.join(", "));
        myvotesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, C_COLUMN_MY_VOTES, myvotesItem);

        //VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote || (entry.fState == DAOFlags::NIL && entry.fCanSupport)) {
            auto *button = new QPushButton;
            if (entry.fCanSupport)
                button->setText(entry.myVotes.size() == 0 ? tr("Support") : tr("Unsupport"));
            else if(entry.fCanVote)
                button->setText(entry.myVotes.size() == 0 ? tr("Vote") : tr("Change"));
            button->setFixedHeight(40);
            button->setProperty("id", QString::fromStdString(entry.hash.GetHex()));
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
        }

        table->setCellWidget(i, C_COLUMN_VOTE, widget);
        table->setItem(i, C_COLUMN_VOTE, votedItem);

    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) {
        table->setColumnWidth(C_COLUMN_VOTE, 0);
        table->setColumnWidth(C_COLUMN_MY_VOTES, 0);
    }
    else
    {
        table->setColumnWidth(C_COLUMN_VOTE, 150);
        table->setColumnWidth(C_COLUMN_MY_VOTES, 150);

    }
    table->setColumnHidden(C_COLUMN_VOTE, !fWeight);
    table->setColumnHidden(C_COLUMN_MY_VOTES, !fWeight);
    table->setSortingEnabled(true);
}

void DaoPage::setData(QVector<DeploymentEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        DeploymentEntry &entry = data[i];

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, D_COLUMN_COLOR, indicatorBox);
        table->setItem(i, D_COLUMN_COLOR, colorItem);

        // TITLE
        auto *titleItem = new QTableWidgetItem;
        titleItem->setData(Qt::DisplayRole, entry.title);
        table->setItem(i, D_COLUMN_TITLE, titleItem);

        // STATE
        auto *statusItem = new QTableWidgetItem;
        statusItem->setData(Qt::DisplayRole, entry.status);
        statusItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, D_COLUMN_STATUS, statusItem);

        // MY VOTES
        auto *myvotesItem = new QTableWidgetItem;
        myvotesItem->setData(Qt::DisplayRole, entry.fMyVote ? tr("Yes") : tr("No"));
        myvotesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, D_COLUMN_MY_VOTES, myvotesItem);

        // VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote) {
            auto *button = new QPushButton;
            button->setText(tr("Change"));
            button->setFixedHeight(40);
            button->setProperty("id", entry.bit);
            button->setProperty("vote", !entry.fMyVote);
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
            if (entry.fDefaultVote)
            {
                auto *buttonConfirm = new QPushButton;
                buttonConfirm->setText(tr("Confirm"));
                buttonConfirm->setFixedHeight(40);
                buttonConfirm->setProperty("id", entry.bit);
                buttonConfirm->setProperty("vote", entry.fMyVote);
                boxLayout->addWidget(buttonConfirm, 0, Qt::AlignCenter);
                connect(buttonConfirm, SIGNAL(clicked()), this, SLOT(onVote()));
            }
        }

        table->setCellWidget(i, D_COLUMN_VOTE, widget);
        table->setItem(i, D_COLUMN_VOTE, votedItem);
    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) {
        table->setColumnWidth(D_COLUMN_VOTE, 0);
        table->setColumnWidth(D_COLUMN_MY_VOTES, 0);
    }
    else
    {
        table->setColumnWidth(D_COLUMN_VOTE, 150);
        table->setColumnWidth(D_COLUMN_MY_VOTES, 150);

    }
    table->setColumnHidden(D_COLUMN_VOTE, !fWeight);
    table->setColumnHidden(D_COLUMN_MY_VOTES, !fWeight);
    table->setSortingEnabled(true);
}

void DaoPage::setData(QVector<ConsensusEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        ConsensusEntry &entry = data[i];

        // HASH
        auto *hashItem = new QTableWidgetItem;
        hashItem->setData(Qt::DisplayRole, QString::fromStdString(entry.hash.GetHex()));
        table->setItem(i, CP_COLUMN_HASH, hashItem);

        // HASH
        auto *idItem = new QTableWidgetItem;
        idItem->setData(Qt::DisplayRole, entry.id);
        table->setItem(i, CP_COLUMN_ID, idItem);

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, CP_COLUMN_COLOR, indicatorBox);
        table->setItem(i, CP_COLUMN_COLOR, colorItem);

        // TITLE
        auto *titleItem = new QTableWidgetItem;
        titleItem->setData(Qt::DisplayRole, entry.parameter);
        table->setItem(i, CP_COLUMN_TITLE, titleItem);

        // STATE
        auto *statusItem = new QTableWidgetItem;
        statusItem->setData(Qt::DisplayRole, entry.sState);
        statusItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, CP_COLUMN_STATUS, statusItem);

        // VALUE
        auto *valueItem = new QTableWidgetItem;
        QString sValue = QString::number(entry.value);
        valueItem->setData(Qt::DisplayRole, QString::fromStdString(FormatConsensusParameter((Consensus::ConsensusParamsPos)entry.id, sValue.toStdString())));
        valueItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, CP_COLUMN_CURRENT, valueItem);

        // MY VOTES
        auto *myvotesItem = new QTableWidgetItem;
        myvotesItem->setData(Qt::DisplayRole, entry.myVotes.join(", "));
        myvotesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, CP_COLUMN_MY_VOTES, myvotesItem);

        //VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote || (entry.fState == DAOFlags::NIL && entry.fCanSupport)) {
            auto *button = new QPushButton;
            if (entry.fCanSupport)
                button->setText(entry.myVotes.size() == 0 ? tr("Support") : tr("Unsupport"));
            else if(entry.fCanVote)
                button->setText(entry.myVotes.size() == 0 ? tr("Vote") : tr("Change"));
            button->setFixedHeight(40);
            button->setProperty("id", QString::fromStdString(entry.hash.GetHex()));
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
        }

        table->setCellWidget(i, CP_COLUMN_VOTE, widget);
        table->setItem(i, CP_COLUMN_VOTE, votedItem);
    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) {
        table->setColumnWidth(CP_COLUMN_VOTE, 0);
        table->setColumnWidth(CP_COLUMN_MY_VOTES, 0);
    }
    else
    {
        table->setColumnWidth(CP_COLUMN_VOTE, 150);
        table->setColumnWidth(CP_COLUMN_MY_VOTES, 150);
    }
    table->setColumnHidden(CP_COLUMN_VOTE, !fWeight);
    table->setColumnHidden(CP_COLUMN_MY_VOTES, !fWeight);
    table->setSortingEnabled(true);
}

void DaoPage::onVote() {
    LOCK(cs_main);

    QVariant idV = sender()->property("id");
    QVariant voteV = sender()->property("vote");
    CStateViewCache coins(pcoinsTip);

    if (idV.isValid() && voteV.isValid())
    {
        VoteVersionBit(idV.toInt(), voteV.toBool());
    }
    else if (idV.isValid())
    {
        CProposal proposal;
        CPaymentRequest prequest;
        CConsultation consultation;
        CConsultationAnswer answer;

        uint256 hash = uint256S(idV.toString().toStdString());

        if (coins.GetProposal(hash, proposal))
        {
            CommunityFundDisplayDetailed dlg(this, proposal);
            dlg.exec();
        }
        else if (coins.GetPaymentRequest(hash, prequest))
        {
            CommunityFundDisplayPaymentRequestDetailed dlg(this, prequest);
            dlg.exec();
        }
        else if (coins.GetConsultation(hash, consultation))
        {
            if (consultation.CanBeSupported())
            {
                if (consultation.IsRange())
                {
                    bool duplicate;
                    if (mapSupported.count(consultation.hash))
                        RemoveSupport(consultation.hash.ToString());
                    else
                        Support(consultation.hash, duplicate);
                }
                else
                {
                    contextHash = QString::fromStdString(consultation.hash.ToString());
                    onSupportAnswer();
                }
            }
            else
            {
                DaoConsultationVote dlg(this, consultation);
                dlg.exec();
            }
        }
        else if (coins.GetConsultationAnswer(hash, answer))
        {
            if (answer.CanBeSupported(coins))
            {
                bool duplicate;
                if (mapSupported.count(answer.hash))
                    RemoveSupport(answer.hash.ToString());
                else
                    Support(answer.hash, duplicate);
            }
        }
    }
    refresh(true, true);

}

void DaoPage::onDetails() {
    if (!contextHash.isEmpty())
    {
        QString type;
        if (nCurrentView == VIEW_PROPOSALS)
            type = "proposal";
        else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
            type = "payment-request";
        else if (nCurrentView == VIEW_CONSULTATIONS)
            type = "consultation";
        QString link = "https://www.navexplorer.com/community-fund/" + type + "/" + contextHash;
        QDesktopServices::openUrl(QUrl(link));
    }
}

void DaoPage::viewProposals() {
    setView(VIEW_PROPOSALS);
    if (filterHash.isEmpty())
    {
        if (nBadgeProposals > 0)
        {
            onFilter(FILTER_NOT_VOTED);
            onFilter2(FILTER2_IN_PROGRESS);
        }
        else
        {
            onFilter(FILTER_ALL);
            onFilter2(FILTER2_ALL);
        }
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consensusBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
}

void DaoPage::viewPaymentRequests() {
    setView(VIEW_PAYMENT_REQUESTS);
    if (filterHash.isEmpty())
    {
        if (nBadgePaymentRequests > 0)
        {
            onFilter(FILTER_NOT_VOTED);
            onFilter2(FILTER2_IN_PROGRESS);
        }
        else
        {
            onFilter(FILTER_ALL);
            onFilter2(FILTER2_ALL);
        }
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consensusBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
}

void DaoPage::viewConsultations() {
    setView(VIEW_CONSULTATIONS);
    if (nBadgeConsultations > 0)
    {
        onFilter(FILTER_NOT_VOTED);
        onFilter2(FILTER2_IN_PROGRESS);
    }
    else
    {
        onFilter(FILTER_ALL);
        onFilter2(FILTER2_ALL);
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consensusBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
}

void DaoPage::viewDeployments() {
    setView(VIEW_DEPLOYMENTS);
    if (nBadgeDeployments > 0)
    {
        onFilter(FILTER_NOT_VOTED);
        onFilter2(FILTER2_IN_PROGRESS);
    }
    else
    {
        onFilter(FILTER_ALL);
        onFilter2(FILTER2_ALL);
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    consensusBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
}

void DaoPage::viewConsensus() {
    setView(VIEW_CONSENSUS);
    if (filterHash.isEmpty())
    {
        if (nBadgeConsensus > 0)
        {
            onFilter(FILTER_NOT_VOTED);
            onFilter2(FILTER2_IN_PROGRESS);
        }
        else
        {
            onFilter(FILTER_ALL);
            onFilter2(FILTER2_ALL);
        }
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consensusBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
}

void DaoPage::backToFilter() {
    filterHash = "";
    if (nCurrentView == VIEW_PROPOSALS)
        viewPaymentRequests();
    else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
        viewProposals();
}

void DaoPage::onFilter(int index) {
    nFilter = index;
    filterCmb->setCurrentIndex(index);
    refresh(true);
}

void DaoPage::onFilter2(int index) {
    nFilter2 = index;
    filter2Cmb->setCurrentIndex(index);
    refresh(true);
}

void DaoPage::onExclude(bool fChecked) {
    fExclude = fChecked;
    refresh(true);
}

void DaoPage::onCreate() {
    if (nCurrentView == VIEW_PROPOSALS)
    {
        CommunityFundCreateProposalDialog dlg(this);
        dlg.setModel(walletModel);
        dlg.exec();
        refresh(true);
    }
    else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
    {
        CommunityFundCreatePaymentRequestDialog dlg(this);
        dlg.setModel(walletModel);
        dlg.exec();
        refresh(true);
    }
    else if (nCurrentView == VIEW_CONSULTATIONS)
    {
        DaoConsultationCreate dlg(this);
        dlg.setModel(walletModel);
        dlg.exec();
        refresh(true);
    }
    else if (nCurrentView == VIEW_CONSENSUS && contextId >= 0)
    {
        DaoConsultationCreate dlg(this, QString::fromStdString(Consensus::sConsensusParamsDesc[(Consensus::ConsensusParamsPos)contextId]), contextId);
        dlg.setModel(walletModel);
        dlg.exec();
        refresh(true);
    }
}

void DaoPage::onSupportAnswer() {
    LOCK(cs_main);

    CStateViewCache coins(pcoinsTip);
    CConsultation consultation;

    if (coins.GetConsultation(uint256S(contextHash.toStdString()), consultation))
    {
        if (!consultation.IsRange())
        {
            DaoSupport dlg(this, consultation);
            dlg.setModel(walletModel);
            dlg.exec();
            refresh(true);
        }
    }
}

void DaoPage::showContextMenu(const QPoint& pt) {
    if (nView == VIEW_DEPLOYMENTS)
        return;

    auto *item = table->itemAt(pt);
    contextHash = "";
    contextId = -1;
    if (!item) {
        contextItem = nullptr;
        return;
    }
    contextItem = item;
    auto *hashItem = table->item(contextItem->row(), P_COLUMN_HASH);
    if (hashItem) {
        LOCK(cs_main);

        CConsultation consultation;
        CProposal proposal;
        CPaymentRequest prequest;

        contextHash = hashItem->data(Qt::DisplayRole).toString();

        if (contextHash == QString::fromStdString(uint256().ToString()))
            contextHash = "";

        copyHash->setDisabled(contextHash == "");
        openExplorerAction->setDisabled(contextHash == "");
        openChart->setDisabled(contextHash == "");
        contextMenu->removeAction(seeProposalAction);
        contextMenu->removeAction(seePaymentRequestsAction);

        proposeChange->setDisabled(false);

        if (pcoinsTip->GetConsultation(uint256S(contextHash.toStdString()), consultation))
        {
            if (consultation.CanHaveNewAnswers())
            {
                proposeChange->setDisabled(true);
            }
            else
            {
                proposeChange->setDisabled(false);
            }
        }
        else if (pcoinsTip->GetProposal(uint256S(contextHash.toStdString()), proposal))
        {
            contextMenu->addAction(seePaymentRequestsAction);
            contextMenu->removeAction(seeProposalAction);

        }
        else if (pcoinsTip->GetPaymentRequest(uint256S(contextHash.toStdString()), prequest))
        {
            contextMenu->addAction(seeProposalAction);
            contextMenu->removeAction(seePaymentRequestsAction);
        }
        else if (nCurrentView == VIEW_CONSENSUS)
        {
            contextId = table->item(contextItem->row(), CP_COLUMN_ID)->data(Qt::DisplayRole).toInt();
            contextMenu->addAction(proposeChange);
        }
        else
        {
            contextMenu->removeAction(proposeChange);
        }
    }
    contextMenu->exec(QCursor::pos());
}

void DaoPage::onCopyHash() {
    if (contextHash.isEmpty())
        return;
    QApplication::clipboard()->setText(contextHash, QClipboard::Clipboard);
}

void DaoPage::onSeePaymentRequests() {
    if (contextHash.isEmpty())
        return;

    filterHash = contextHash;

    viewPaymentRequests();
}

void DaoPage::onSeeProposal() {
    if (contextHash.isEmpty())
        return;

    LOCK(cs_main);

    CPaymentRequest prequest;

    if (pcoinsTip->GetPaymentRequest(uint256S(contextHash.toStdString()), prequest))
    {
        filterHash = QString::fromStdString(prequest.proposalhash.ToString());
    }

    viewProposals();
}

void DaoPage::onViewChart() {
    if (contextHash.isEmpty())
        return;

    CConsultation consultation;
    CProposal proposal;
    CPaymentRequest prequest;
    std::map<QString, uint64_t> mapVotes;
    QString title;

    {
        LOCK(cs_main);
        if (pcoinsTip->GetConsultation(uint256S(contextHash.toStdString()), consultation))
        {
            title = QString::fromStdString(consultation.strDZeel);

            if (consultation.CanBeSupported())
                title += " - Showing Support";

            if (consultation.mapVotes.count((uint64_t)-5))
                mapVotes.insert(make_pair(QString(tr("Abstention") + " (" + QString::number(consultation.mapVotes.at((uint64_t)-5)) + ")"), consultation.mapVotes.at((uint64_t)-5)));

            if (consultation.IsRange())
            {
                for (auto&it: consultation.mapVotes)
                {
                    if (it.first == -1)
                        continue;
                    mapVotes.insert(make_pair(QString::number(it.first) + " (" + QString::number(it.second) + ")", it.second));
                }
            }
            else
            {
                for (auto&it: consultationAnswerMap)
                {
                    CConsultationAnswer answer = it.second;

                    if (answer.parent == consultation.hash)
                    {
                        uint64_t nAmount = consultation.CanBeSupported() ? answer.nSupport : answer.nVotes;
                        mapVotes.insert(make_pair(QString::fromStdString(answer.sAnswer) + " (" + QString::number(nAmount) + ")",nAmount));
                    }
                }
            }
        }
        else if (pcoinsTip->GetProposal(uint256S(contextHash.toStdString()), proposal))
        {
            title = QString::fromStdString(proposal.strDZeel);

            mapVotes.insert(make_pair(QString("Yes (" + QString::number(proposal.nVotesYes) + ")"), proposal.nVotesYes));
            mapVotes.insert(make_pair(QString("No (" + QString::number(proposal.nVotesNo) + ")"), proposal.nVotesNo));
            mapVotes.insert(make_pair(QString("Abstention (" + QString::number(proposal.nVotesAbs) + ")"), proposal.nVotesAbs));
        }
        else if (pcoinsTip->GetPaymentRequest(uint256S(contextHash.toStdString()), prequest))
        {
            title = QString::fromStdString(prequest.strDZeel);

            mapVotes.insert(make_pair(QString("Yes (" + QString::number(prequest.nVotesYes) + ")"), prequest.nVotesYes));
            mapVotes.insert(make_pair(QString("No (" + QString::number(prequest.nVotesNo) + ")"), prequest.nVotesNo));
            mapVotes.insert(make_pair(QString("Abstention (" + QString::number(prequest.nVotesAbs) + ")"), prequest.nVotesAbs));
        }
    }

    DaoChart dlg(this, title, mapVotes);
    dlg.exec();
}

DaoChart::DaoChart(QWidget *parent, QString title, std::map<QString, uint64_t> mapVotes) :
    layout(new QVBoxLayout)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);
    this->setStyleSheet(Skinize());
    this->resize(450, 300);

    QtCharts::QPieSeries *series = new QtCharts::QPieSeries();

    int i = 0;
    for (auto& it: mapVotes)
    {
        if (it.second == 0)
            continue;
        series->append(it.first, it.second);
        QtCharts::QPieSlice *slice = series->slices().at(i);
        slice->setLabelVisible();
        i++;
    }

    QtCharts::QChart *chart = new QtCharts::QChart();
    chart->addSeries(series);
    chart->setTitle(title);
    chart->legend()->hide();

    QtCharts::QChartView *chartView = new QtCharts::QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    layout->addWidget(chartView);
}
