// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/navcoin-config.h>
#endif

#include <qt/optionsdialog.h>
#include <ui_optionsdialog.h>

#include <qt/guiutil.h>
#include <qt/navcoinunits.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>

#include <chainparams.h>
#include <main.h> // for DEFAULT_SCRIPTCHECK_THREADS and MAX_SCRIPTCHECK_THREADS
#include <miner.h>
#include <netbase.h>
#include <txdb.h> // for -dbcache defaults

#ifdef ENABLE_WALLET
#include <wallet/wallet.h> // for CWallet::GetRequiredFee()
#endif

#include <boost/thread.hpp>

#include <QDataWidgetMapper>
#include <QDir>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

OptionsDialog::OptionsDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog),
    model(0),
    mapper(0),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

    /* Main elements init */
    ui->databaseCache->setMinimum(nMinDbCache);
    ui->databaseCache->setMaximum(nMaxDbCache);
    ui->threadsScriptVerif->setMinimum(-GetNumCores());
    ui->threadsScriptVerif->setMaximum(MAX_SCRIPTCHECK_THREADS);

    /* Network elements init */
#ifndef USE_UPNP
    ui->mapPortUpnp->setEnabled(false);
#endif

    ui->proxyIp->setEnabled(false);
    ui->proxyPort->setEnabled(false);
    ui->proxyPort->setValidator(new QIntValidator(1, 65535, this));

    ui->proxyIpTor->setEnabled(false);
    ui->proxyPortTor->setEnabled(false);
    ui->proxyPortTor->setValidator(new QIntValidator(1, 65535, this));

    ui->mixingDefaultBox->addItem("Always ask");
    ui->mixingDefaultBox->addItem("Yes");
    ui->mixingDefaultBox->addItem("No");

    connect(ui->mixingDefaultBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=]( int ix ) {
        QSettings settings;

        settings.setValue("defaultPrivacy", ix == -1 ? 2 : (ix == 1 ? 1 : 0));
    });

    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyIp, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyPort, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), this, SLOT(updateProxyValidationState()));

    connect(ui->connectSocksTor, SIGNAL(toggled(bool)), ui->proxyIpTor, SLOT(setEnabled(bool)));
    connect(ui->connectSocksTor, SIGNAL(toggled(bool)), ui->proxyPortTor, SLOT(setEnabled(bool)));
    connect(ui->connectSocksTor, SIGNAL(toggled(bool)), this, SLOT(updateProxyValidationState()));
    connect(ui->voteTextField, SIGNAL(textChanged(QString)), this, SLOT(vote(QString)));

    bool showVoting = GetStaking();

    ui->voteLabel->setVisible(showVoting);
    ui->voteTextField->setVisible(showVoting);
    ui->voteTextField->setText(QString::fromStdString(GetArg("-stakervote","")));

    /* Window elements init */
#ifdef Q_OS_MAC
    /* remove Window tab on Mac */
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabWindow));
#endif

#ifdef ENABLE_WALLET
    /* remove Wallet tab in case of -disablewallet */
    if (GetBoolArg("-disablewallet", false)) {
#endif // ENABLE_WALLET
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabWallet));
#ifdef ENABLE_WALLET
    }
#endif

    /* Display elements init */
    QDir translations(":translations");

    ui->navcoinAtStartup->setToolTip(ui->navcoinAtStartup->toolTip().arg(tr(PACKAGE_NAME)));
    ui->navcoinAtStartup->setText(ui->navcoinAtStartup->text().arg(tr(PACKAGE_NAME)));

    // Add the themes that we support
    ui->theme->addItem(tr("Light") + "("+ tr("default") + ")", QVariant("light"));
    ui->theme->addItem(tr("Dark"), QVariant("dark"));

    ui->lang->setToolTip(ui->lang->toolTip().arg(tr(PACKAGE_NAME)));
    ui->lang->addItem(QString("(") + tr("default") + QString(")"), QVariant(""));
    for(const QString &langStr: translations.entryList())
    {
        QLocale locale(langStr);

        /** check if the locale name consists of 2 parts (language_country) */
        if(langStr.contains("_"))
        {
            /** display language strings as "native language - native country (locale name)", e.g. "Deutsch - Deutschland (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" - ") + locale.nativeCountryName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
        }
        else
        {
            /** display language strings as "native language (locale name)", e.g. "Deutsch (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
        }
    }

    ui->thirdPartyTxUrls->setPlaceholderText("https://explorer.navcoin.org/tx/%s");

    ui->unit->setModel(new NavcoinUnits(this));

    /* Widget-to-option mapper */
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    /* setup/change UI elements when proxy IPs are invalid/valid */
    ui->proxyIp->setCheckValidator(new ProxyAddressValidator(parent));
    ui->proxyIpTor->setCheckValidator(new ProxyAddressValidator(parent));
    connect(ui->proxyIp, SIGNAL(validationDidChange(QValidatedLineEdit *)), this, SLOT(updateProxyValidationState()));
    connect(ui->proxyIpTor, SIGNAL(validationDidChange(QValidatedLineEdit *)), this, SLOT(updateProxyValidationState()));
    connect(ui->proxyPort, SIGNAL(textChanged(const QString&)), this, SLOT(updateProxyValidationState()));
    connect(ui->proxyPortTor, SIGNAL(textChanged(const QString&)), this, SLOT(updateProxyValidationState()));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::setModel(OptionsModel *model)
{
    this->model = model;

    QSettings settings;

    ui->voteTextField->setText(QString::fromStdString(GetArg("-stakervote","")));
    ui->voteQuestionLabel->setText(settings.value("votingQuestion", "").toString());
    ui->maxmixfeeText->setDisplayUnit(model->getDisplayUnit());
    ui->maxmixfeeText->setValue(GetArg("-aggregationmaxfee", DEFAULT_MAX_MIX_FEE));
    ui->mixTimeout->setValue(settings.value("aggregationSessionWait", 15).toInt());

    int defaultPrivacy = settings.value("defaultPrivacy", 0).toInt();

    if (defaultPrivacy == 1)
    {
        ui->mixingDefaultBox->setCurrentIndex(1);
    }
    else if (defaultPrivacy == -1)
    {
        ui->mixingDefaultBox->setCurrentIndex(2);
    }
    else
    {
        ui->mixingDefaultBox->setCurrentIndex(0);
    }

    if(model)
    {
        /* check if client restart is needed and show persistent message */
        if (model->isRestartRequired())
            showRestartWarning(true);

        QString strLabel = model->getOverriddenByCommandLine();
        if (strLabel.isEmpty())
            strLabel = tr("none");
        ui->overriddenByCommandLineLabel->setText(strLabel);

        mapper->setModel(model);
        setMapper();
        mapper->toFirst();

        updateDefaultProxyNets();
    }

    /* warn when one of the following settings changes by user action (placed here so init via mapper doesn't trigger them) */

    /* Main */
    connect(ui->databaseCache, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    connect(ui->threadsScriptVerif, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    /* Wallet */
    connect(ui->spendZeroConfChange, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    /* Network */
    connect(ui->allowIncoming, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    connect(ui->connectSocks, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    connect(ui->connectSocksTor, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    /* Display */
    connect(ui->lang, SIGNAL(valueChanged()), this, SLOT(showRestartWarning()));
    connect(ui->theme, SIGNAL(valueChanged()), this, SLOT(showRestartWarning()));
    connect(ui->scaling, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    connect(ui->thirdPartyTxUrls, SIGNAL(textChanged(const QString &)), this, SLOT(showRestartWarning()));

    /* Main */
    connect(ui->navcoinAtStartup, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));
    connect(ui->threadsScriptVerif, SIGNAL(valueChanged(int)), this, SLOT(markModelDirty()));
    connect(ui->databaseCache, SIGNAL(valueChanged(int)), this, SLOT(markModelDirty()));

    /* Wallet */
    connect(ui->spendZeroConfChange, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));

    /* Network */
    connect(ui->mapPortUpnp, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));
    connect(ui->allowIncoming, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));

    connect(ui->connectSocks, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));
    connect(ui->proxyIp, SIGNAL(textChanged(const QString &)), this, SLOT(markModelDirty()));
    connect(ui->proxyPort, SIGNAL(textChanged(const QString &)), this, SLOT(markModelDirty()));

    connect(ui->connectSocksTor, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));
    connect(ui->proxyIpTor, SIGNAL(textChanged(const QString &)), this, SLOT(markModelDirty()));
    connect(ui->proxyPortTor, SIGNAL(textChanged(const QString &)), this, SLOT(markModelDirty()));

    /* Window */
#ifndef Q_OS_MAC
    connect(ui->hideTrayIcon, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));
    connect(ui->minimizeToTray, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));
    connect(ui->minimizeOnClose, SIGNAL(clicked(bool)), this, SLOT(markModelDirty()));
#endif

    /* Display */
    connect(ui->lang, SIGNAL(valueChanged()), this, SLOT(markModelDirty()));
    connect(ui->unit, SIGNAL(valueChanged()), this, SLOT(markModelDirty()));
    connect(ui->theme, SIGNAL(valueChanged()), this, SLOT(markModelDirty()));
    connect(ui->scaling, SIGNAL(valueChanged(int)), this, SLOT(markModelDirty()));
    connect(ui->thirdPartyTxUrls, SIGNAL(textChanged(const QString &)), this, SLOT(markModelDirty()));
}

void OptionsDialog::setMapper()
{
    /* Main */
    mapper->addMapping(ui->navcoinAtStartup, OptionsModel::StartAtStartup);
    mapper->addMapping(ui->threadsScriptVerif, OptionsModel::ThreadsScriptVerif);
    mapper->addMapping(ui->databaseCache, OptionsModel::DatabaseCache);

    /* Wallet */
    mapper->addMapping(ui->spendZeroConfChange, OptionsModel::SpendZeroConfChange);

    /* Network */
    mapper->addMapping(ui->mapPortUpnp, OptionsModel::MapPortUPnP);
    mapper->addMapping(ui->allowIncoming, OptionsModel::Listen);

    mapper->addMapping(ui->connectSocks, OptionsModel::ProxyUse);
    mapper->addMapping(ui->proxyIp, OptionsModel::ProxyIP);
    mapper->addMapping(ui->proxyPort, OptionsModel::ProxyPort);

    mapper->addMapping(ui->connectSocksTor, OptionsModel::ProxyUseTor);
    mapper->addMapping(ui->proxyIpTor, OptionsModel::ProxyIPTor);
    mapper->addMapping(ui->proxyPortTor, OptionsModel::ProxyPortTor);

    /* Window */
#ifndef Q_OS_MAC
    mapper->addMapping(ui->hideTrayIcon, OptionsModel::HideTrayIcon);
    mapper->addMapping(ui->minimizeToTray, OptionsModel::MinimizeToTray);
    mapper->addMapping(ui->minimizeOnClose, OptionsModel::MinimizeOnClose);
#endif

    /* Display */
    mapper->addMapping(ui->lang, OptionsModel::Language);
    mapper->addMapping(ui->unit, OptionsModel::DisplayUnit);
    mapper->addMapping(ui->theme, OptionsModel::Theme);
    mapper->addMapping(ui->scaling, OptionsModel::Scaling);
    mapper->addMapping(ui->thirdPartyTxUrls, OptionsModel::ThirdPartyTxUrls);
}

void OptionsDialog::markModelDirty()
{
    info("MODEL DIRTY");
    model->setDirty(true);
}

void OptionsDialog::setOkButtonState(bool fState)
{
    ui->okButton->setEnabled(fState);
}

void OptionsDialog::on_resetButton_clicked()
{
    if(model)
    {
        // confirmation dialog
        QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm options reset"),
                tr("Client restart required to activate changes.") + "<br><br>" + tr("Client will be shut down. Do you want to proceed?"),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if(btnRetVal == QMessageBox::Cancel)
            return;

        SoftSetArg("-defaultmixin", std::to_string(GetArg("-defaultmixin", DEFAULT_TX_MIXCOINS)), true);
        RemoveConfigFile("defaultmixin");
        WriteConfigFile("defaultmixin", std::to_string(GetArg("-defaultmixin", DEFAULT_TX_MIXCOINS)));

        QSettings settings;
        settings.setValue("aggregationSessionWait", settings.value("aggregationSessionWait", 15));

        SoftSetArg("-aggregationmaxfee", std::to_string(GetArg("-aggregationmaxfee", DEFAULT_MAX_MIX_FEE)), true);
        RemoveConfigFile("aggregationmaxfee");
        WriteConfigFile("aggregationmaxfee", std::to_string(GetArg("-aggregationmaxfee", DEFAULT_MAX_MIX_FEE)));

        /* reset all options and close GUI */
        model->Reset();
        QApplication::quit();
    }
}

void OptionsDialog::on_okButton_clicked()
{
    SoftSetArg("-aggregationmaxfee", std::to_string(ui->maxmixfeeText->value()), true);
    RemoveConfigFile("aggregationmaxfee");
    WriteConfigFile("aggregationmaxfee", std::to_string(ui->maxmixfeeText->value()));

    QSettings settings;
    settings.setValue("aggregationSessionWait", ui->mixTimeout->value());

    mapper->submit();
    model->setDirty(false);
    updateDefaultProxyNets();

    QMessageBox::information(this, tr("Changes saved"), tr("Changes have been saved!"));
}

void OptionsDialog::on_openNavcoinConfButton_clicked()
{
    QMessageBox::information(this, tr("Configuration options"),
            tr("The configuration is used to specify advanced user options less any command-line or Qt options. "
                "Any command-line options will override this configuration file."));
    GUIUtil::openNavcoinConf();
}

void OptionsDialog::on_cancelButton_clicked()
{
    reject();
}

void OptionsDialog::on_hideTrayIcon_stateChanged(int fState)
{
    if(fState)
    {
        ui->minimizeToTray->setChecked(false);
        ui->minimizeToTray->setEnabled(false);
    }
    else
    {
        ui->minimizeToTray->setEnabled(true);
    }
}

void OptionsDialog::showRestartWarning(bool fPersistent)
{
    ui->statusLabel->setStyleSheet("QLabel { color: red; }");

    if(fPersistent)
    {
        ui->statusLabel->setText(tr("Client restart required to activate changes."));
    }
    else
    {
        ui->statusLabel->setText(tr("This change would require a client restart."));
        // clear non-persistent status label after 10 seconds
        // Todo: should perhaps be a class attribute, if we extend the use of statusLabel
        QTimer::singleShot(10000, this, SLOT(clearStatusLabel()));
    }
}

void OptionsDialog::clearStatusLabel()
{
    ui->statusLabel->clear();
}

void OptionsDialog::vote(QString vote)
{
    SoftSetArg("-stakervote",vote.toStdString(),true);
    RemoveConfigFile("stakervote");
    WriteConfigFile("stakervote",vote.toStdString());
}

void OptionsDialog::updateProxyValidationState()
{
    QValidatedLineEdit *pUiProxyIp = ui->proxyIp;
    QValidatedLineEdit *otherProxyWidget = (pUiProxyIp == ui->proxyIpTor) ? ui->proxyIp : ui->proxyIpTor;
    if (pUiProxyIp->isValid() && (!ui->proxyPort->isEnabled() || ui->proxyPort->text().toInt() > 0) && (!ui->proxyPortTor->isEnabled() || ui->proxyPortTor->text().toInt() > 0))
    {
        setOkButtonState(otherProxyWidget->isValid()); //only enable ok button if both proxys are valid
        ui->statusLabel->clear();
    }
    else
    {
        setOkButtonState(false);
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("The supplied proxy address is invalid."));
    }
}

void OptionsDialog::updateDefaultProxyNets()
{
    proxyType proxy;
    std::string strProxy;
    QString strDefaultProxyGUI;

    GetProxy(NET_IPV4, proxy);
    strProxy = proxy.proxy.ToStringIP() + ":" + proxy.proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachIPv4->setChecked(true) : ui->proxyReachIPv4->setChecked(false);

    GetProxy(NET_IPV6, proxy);
    strProxy = proxy.proxy.ToStringIP() + ":" + proxy.proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachIPv6->setChecked(true) : ui->proxyReachIPv6->setChecked(false);

    GetProxy(NET_TOR, proxy);
    strProxy = proxy.proxy.ToStringIP() + ":" + proxy.proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachTor->setChecked(true) : ui->proxyReachTor->setChecked(false);
}

ProxyAddressValidator::ProxyAddressValidator(QObject *parent) :
    QValidator(parent)
{
}

QValidator::State ProxyAddressValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);
    // Validate the proxy
    proxyType addrProxy = proxyType(CService(input.toStdString(), GetArg("-torsocksport", 9044)), true);
    if (addrProxy.IsValid())
        return QValidator::Acceptable;

    return QValidator::Invalid;
}
