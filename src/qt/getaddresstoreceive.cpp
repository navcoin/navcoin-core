#include "addresstablemodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "main.h"
#include "walletmodel.h"
#include "wallet/wallet.h"

#include "coldstakingwizard.h"
#include "getaddresstoreceive.h"
#include "ui_getaddresstoreceive.h"

#include <boost/foreach.hpp>

#include <stdint.h>

#include <QSettings>

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

getAddressToReceive::getAddressToReceive(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::getAddressToReceive)
{
    ui->setupUi(this);

    LOCK(pwalletMain->cs_wallet);
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, pwalletMain->mapAddressBook)
    {
        const CNavCoinAddress& addressbook = item.first;
        bool fMine = IsMine(*pwalletMain, addressbook.Get());
        if(fMine)
        {
          address = QString::fromStdString(addressbook.ToString());
          break;
        }
    }

    connect(ui->requestPaymentButton,SIGNAL(clicked()),this,SLOT(showRequestPayment()));
    connect(ui->copyClipboardButton,SIGNAL(clicked()),this,SLOT(copyToClipboard()));
    connect(ui->newAddressButton,SIGNAL(clicked()),this,SLOT(getNewAddress()));
    connect(ui->coldStakingButton,SIGNAL(clicked()),this,SLOT(getColdStakingAddress()));
    connect(ui->requestNewAddressButton,SIGNAL(clicked()),this,SLOT(showAddressHistory()));
    connect(ui->privateAddressButton,SIGNAL(clicked()),this,SLOT(showPrivateAddress()));
}

getAddressToReceive::~getAddressToReceive()
{
    delete ui;
}

void getAddressToReceive::showRequestPayment()
{
    Q_EMIT requestPayment();
}

void getAddressToReceive::showAddressHistory()
{
    Q_EMIT requestAddressHistory();
}

void getAddressToReceive::setModel(WalletModel *model)
{
    this->model = model;
}

void getAddressToReceive::copyToClipboard()
{
    GUIUtil::setClipboard(address);
}

void getAddressToReceive::getNewAddress()
{
    address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, "", "");
    showQR();
}

void getAddressToReceive::getColdStakingAddress()
{
    if (!IsCommunityFundEnabled(pindexBestHeader,Params().GetConsensus()))
        QMessageBox::warning(this, tr("Action not available"),
                             "<qt>Cold Staking is not active yet.</qt>");
    else {
        ColdStakingWizard wizard;
        wizard.exec();
    }
}

void getAddressToReceive::showPrivateAddress()
{
    if (address.length() > 400)
    {
        LOCK(pwalletMain->cs_wallet);
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, pwalletMain->mapAddressBook)
        {
            const CNavCoinAddress& addressbook = item.first;
            bool fMine = IsMine(*pwalletMain, addressbook.Get());
            if(fMine)
            {
              address = QString::fromStdString(addressbook.ToString());
              break;
            }
        }
        ui->privateAddressButton->setText(QString(tr("Show private address")));
        ui->requestNewAddressButton->show();
        ui->requestPaymentButton->show();
        ui->coldStakingButton->show();
        ui->newAddressButton->show();
    }
    else
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        CKey zk; libzeroct::BlindingCommitment bc;
        pwalletMain->GetBlindingCommitment(bc);
        pwalletMain->GetZeroKey(zk);

        libzeroct::CPrivateAddress pa(&Params().GetConsensus().ZeroCT_Params, bc, zk, "", 0);
        address = QString::fromStdString(CNavCoinAddress(pa).ToString());
        ui->privateAddressButton->setText(QString(tr("Show public address")));
        ui->requestNewAddressButton->hide();
        ui->requestPaymentButton->hide();
        ui->coldStakingButton->hide();
        ui->newAddressButton->hide();
    }
    showQR();
}

void getAddressToReceive::showQR()
{
#ifdef USE_QRCODE
    QString uri = "navcoin:" + address;
    ui->lblQRCode->setText("");
    if(!uri.isEmpty())
    {
        // limit URI length
        if (uri.length() > MAX_URI_LENGTH)
        {
            ui->lblQRCode->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        } else {
            QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
            if (!code)
            {
                ui->lblQRCode->setText(tr("Error encoding URI into QR Code."));
                return;
            }
            QImage qrImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
            qrImage.fill(0xE8EBF0);
            unsigned char *p = code->data;
            for (int y = 0; y < code->width; y++)
            {
                for (int x = 0; x < code->width; x++)
                {
                    qrImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xFFFFFF));
                    p++;
                }
            }
            QRcode_free(code);

            QImage qrAddrImage = QImage(QR_IMAGE_SIZE+(uri.length() > 400 ? 160 : 0), QR_IMAGE_SIZE+(uri.length() > 400 ? 140 : 20), QImage::Format_RGB32);
            qrAddrImage.fill(0xE8EBF0);
            QPainter painter(&qrAddrImage);
            painter.drawImage((uri.length() > 400 ? 80 : 0), 0, qrImage.scaled(QR_IMAGE_SIZE, QR_IMAGE_SIZE));
            QFont font = GUIUtil::fixedPitchFont();
            font.setPixelSize(12);
            painter.setFont(font);
            QRect paddedRect = qrAddrImage.rect();
            paddedRect.setHeight(QR_IMAGE_SIZE+(uri.length() > 400 ? 132     : 12));
            painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter|Qt::TextWrapAnywhere, address);
            painter.end();

            ui->lblQRCode->setPixmap(QPixmap::fromImage(qrAddrImage));
        }
    }
#endif
}
