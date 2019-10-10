// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "splitrewards.h"

SplitRewardsDialog::SplitRewardsDialog(QWidget *parent) :
    strDesc(new QLabel),
    tree(new QTreeView),
    comboAddress(new QComboBox)
{
    QVBoxLayout* layout(new QVBoxLayout);

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);
    this->setStyleSheet(Skinize());

    std::string sJson = GetArg("-stakingaddress", "") == "" ? "{\"all\":{}}" : GetArg("-stakingaddress", "");
    QJsonDocument jsonResponse = QJsonDocument::fromJson(QString::fromStdString(sJson).toUtf8());
    jsonObject = jsonResponse.object();

    availableAmount = 100;

    auto *topBox = new QFrame;
    auto *topBoxLayout = new QHBoxLayout;
    topBoxLayout->setContentsMargins(QMargins());
    topBox->setLayout(topBoxLayout);

    topBoxLayout->addWidget(new QLabel(tr("Configure where your staking rewards are forwarded.")));
    topBoxLayout->addStretch(1);
    topBoxLayout->addWidget(new QLabel(tr("Setup for staking address:")));
    topBoxLayout->addWidget(comboAddress);

    strDesc->setWordWrap(true);

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    QPushButton* addBtn = new QPushButton(tr("Add"));
    connect(addBtn, SIGNAL(clicked()), this, SLOT(onAdd()));

    QPushButton* editBtn = new QPushButton(tr("Edit"));
    connect(editBtn, SIGNAL(clicked()), this, SLOT(onEdit()));

    QPushButton* removeBtn = new QPushButton(tr("Remove"));
    connect(removeBtn, SIGNAL(clicked()), this, SLOT(onRemove()));

    QPushButton* saveBtn = new QPushButton(tr("Save"));
    connect(saveBtn, SIGNAL(clicked()), this, SLOT(onSave()));

    QPushButton* quitBtn = new QPushButton(tr("Cancel"));
    connect(quitBtn, SIGNAL(clicked()), this, SLOT(onQuit()));

    connect(comboAddress, QOverload<int>::of(&QComboBox::activated),
        [=](int index){ showFor(comboAddress->itemData(index).toString()); });

    bottomBoxLayout->addWidget(addBtn);
    bottomBoxLayout->addWidget(editBtn);
    bottomBoxLayout->addWidget(removeBtn);
    bottomBoxLayout->addStretch(1);
    bottomBoxLayout->addWidget(saveBtn);
    bottomBoxLayout->addWidget(quitBtn);

    layout->addWidget(topBox);
    layout->addWidget(tree);
    layout->addWidget(strDesc);
    layout->addWidget(bottomBox);

    std::map<QString, CAmount> mapAddressBalance;

    std::vector<COutput> vCoins;
    pwalletMain->AvailableCoinsForStaking(vCoins, GetTime());

    comboAddress->clear();
    comboAddress->insertItem(0, "Default", "all");

    for (const COutput& out: vCoins)
    {
        CTxDestination dest;
        if (ExtractDestination(out.tx->vout[out.i].scriptPubKey, dest)){
            CNavCoinAddress address(dest);
            if (address.IsColdStakingAddress(Params()))
                if (!address.GetStakingAddress(address))
                    continue;
            QString strAddress = QString::fromStdString(address.ToString());
            if (mapAddressBalance.count(strAddress) == 0)
                mapAddressBalance[strAddress] = 0;
            mapAddressBalance[strAddress] += out.tx->vout[out.i].nValue;
        }
    }

    int i = 1;

    for (const auto &it: mapAddressBalance)
    {
        comboAddress->insertItem(i++, it.first +" ("+ QString::fromStdString(FormatMoney(it.second)) +" NAV)",it.first);
    }

    showFor("all");
}

void SplitRewardsDialog::showFor(QString sin)
{
    currentAddress = sin;

    QJsonObject j;
    QJsonObject jDup;

    if (jsonObject.contains(currentAddress))
    {
        j  = jsonObject[currentAddress].toObject();
    }

    availableAmount = 100;
    CAmount nCFundContribution = GetFundContributionPerBlock(chainActive.Tip());

    QStringList descs;

    for (auto &key: j.keys())
    {
        CNavCoinAddress address(key.toStdString());
        double amount;

        if (address.IsValid())
        {
            amount = j[key].toInt();

            if (teamAddresses.count(key))
            {
                if (teamAddresses[key] == "Community Fund")
                    nCFundContribution += PercentageToNav(amount);
                else
                    descs << QString::fromStdString(FormatMoney(PercentageToNav(amount))) + " to " + teamAddresses[key];
                jDup[teamAddresses[key]] = amount;
            }
            else {
                descs << QString::fromStdString(FormatMoney(PercentageToNav(amount))) + " to " + key;
                jDup[key] = amount;
            }

            availableAmount -= amount;
        }
        else
        {
            j.erase(j.find(key));
        }

        if (availableAmount < 0)
        {
            j.erase(j.find(key));
            jDup.erase(jDup.find(key));
        }
    }

    jsonObject[currentAddress] = j;

    QJsonModel* jmodel = new QJsonModel("Address", "Percentage");
    QJsonDocument doc(jDup);

    tree->setModel(jmodel);

    tree->resizeColumnToContents(1);

    strDesc->setText(tr("For each block, %1 NAV will go to the Community Fund, %2 and %3 will be accumulated in your own address").arg(QString::fromStdString(FormatMoney(nCFundContribution)), descs.join(", "), QString::fromStdString(FormatMoney(PercentageToNav(availableAmount)))));

    jmodel->loadJson(doc.toJson());
}

void SplitRewardsDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void SplitRewardsDialog::onAdd()
{
    if (availableAmount <= 0)
    {
        QMessageBox msgBox(this);
        msgBox.setText(tr("You are already contributing 100% of your staking rewards."));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }

    QString address = "";

    bool ok;
    QString item = QInputDialog::getItem(this, tr("Add a new address"),
            tr("Choose address:"), teamAddresses.values(), 0, false, &ok);

    if (ok && !item.isEmpty())
    {
        if (item == "Custom")
        {
            QString item = QInputDialog::getText(this, tr("Add a new address"),
                    tr("Enter an address:"), QLineEdit::Normal, "", &ok);
            if (ok && !item.isEmpty())
            {
                address = item;
            }
            else
            {
                return;
            }

        }
        else
        {
            for (auto& key: teamAddresses.keys())
            {
                if (teamAddresses[key] == item)
                {
                    address = key;
                    break;
                }
            }
        }
    }
    else
    {
        return;
    }

    if (!CNavCoinAddress(address.toStdString()).IsValid())
    {
        QMessageBox msgBox(this);
        msgBox.setText(tr("Invalid NavCoin Address"));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }

    int amount = QInputDialog::getInt(this, tr("Add a new address"),
                                 tr("Percentage:"), availableAmount, 0, availableAmount, 1, &ok);
    if (!ok)
        return;

    QJsonObject j  = jsonObject[currentAddress].toObject();
    j[address] = amount;
    jsonObject[currentAddress] = j;

    showFor(currentAddress);
}

void SplitRewardsDialog::onRemove()
{
    QModelIndex index = tree->currentIndex();
    int row = index.row();
    QModelIndex indexrow = tree->model()->index(row, 0);
    QVariant data = tree->model()->data(indexrow);
    QString text = data.toString();
    QJsonObject j  = jsonObject[currentAddress].toObject();
    j.erase(j.find(text));
    jsonObject[currentAddress] = j;
    showFor(currentAddress);
}

void SplitRewardsDialog::onEdit()
{
    QModelIndex index = tree->currentIndex();
    int row = index.row();
    QModelIndex indexrow = tree->model()->index(row, 0);
    QVariant data = tree->model()->data(indexrow);
    QString text = data.toString();
    QString address = text;
    for (auto& key: teamAddresses.keys())
    {
        if (teamAddresses[key] == text)
        {
            address = key;
            break;
        }
    }
    QJsonObject j  = jsonObject[currentAddress].toObject();
    bool ok;
    int amount = QInputDialog::getInt(this, tr("Edit address %1").arg(text),
                                 tr("Percentage:"), j[address].toInt(), 0, availableAmount+j[address].toInt(), 1, &ok);
    if (!ok)
        return;
    j[address] = amount;
    jsonObject[currentAddress] = j;
    showFor(currentAddress);
}

void SplitRewardsDialog::onSave()
{
    QJsonDocument doc(jsonObject);
    QString strJson(doc.toJson(QJsonDocument::Compact));
    SoftSetArg("-stakingaddress", strJson.toStdString(), true);
    RemoveConfigFile("stakingaddress");
    WriteConfigFile("stakingaddress", strJson.toStdString());
    QDialog::accept();
}

void SplitRewardsDialog::onQuit()
{
    QDialog::accept();
}

CAmount PercentageToNav(int percentage)
{
    return GetStakingRewardPerBlock(chainActive.Tip()) * percentage / 100;
}
