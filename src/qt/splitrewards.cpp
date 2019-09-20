// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "splitrewards.h"

SplitRewardsDialog::SplitRewardsDialog(QWidget *parent) :
    strDesc(new QLabel),
    strLabel(new QLabel),
    tree(new QTreeView)
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

    QPushButton* changeBtn = new QPushButton(tr("Change"));
    connect(changeBtn, SIGNAL(clicked()), this, SLOT(onChange()));

    topBoxLayout->addWidget(new QLabel(tr("<b>Staking Rewards Setup</b>")));
    topBoxLayout->addStretch(1);
    topBoxLayout->addWidget(strLabel);
    topBoxLayout->addWidget(changeBtn);


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
    CAmount nCFundContribution =  Params().GetConsensus().nCommunityFundAmountV2;

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

    strLabel->setText(currentAddress == "all" ? tr("Default settings") : tr("Settings for ") + currentAddress);

    strDesc->setText(tr("For each block, %1 NAV will go to the Community Fund, %2 and %3 will be accumulated in your own address").arg(QString::fromStdString(FormatMoney(nCFundContribution)), descs.join(", "), QString::fromStdString(FormatMoney(PercentageToNav(availableAmount)))));

    jmodel->loadJson(doc.toJson());
}

void SplitRewardsDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void SplitRewardsDialog::onAdd()
{
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

void SplitRewardsDialog::onChange()
{
    bool ok;
    QString item = QInputDialog::getText(this, tr("Set up a concrete address"),
            tr("Enter an address (\"all\" for default settings):"), QLineEdit::Normal, "", &ok);
    if (ok && !item.isEmpty() && (CNavCoinAddress(item.toStdString()).IsValid() || item == "all"))
    {
        showFor(item);
    }
    else
    {
        return;
    }
}

CAmount PercentageToNav(int percentage)
{
    return Params().GetConsensus().nStaticReward * percentage / 100;
}
