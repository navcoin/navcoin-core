#include <qt/addresstablemodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <main.h>
#include <qt/walletmodel.h>
#include <wallet/wallet.h>
#include <base58.h>

#include <qt/coldstakingwizard.h>
#include <qt/getaddresstoreceive.h>
#include <qt/forms/ui_getaddresstoreceive.h>

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
    for(const PAIRTYPE(CTxDestination, CAddressBookData)& item: pwalletMain->mapAddressBook)
    {
        const CNavcoinAddress& addressbook = item.first;
        bool fMine = IsMine(*pwalletMain, addressbook.Get());
        if(fMine)
        {
            address = QString::fromStdString(addressbook.ToString());
            break;
        }
    }

    ui->textAddress->setMinimumWidth(260 * GUIUtil::scale());

    QPixmap p1(":/icons/mininav");
    QPixmap p2(":/icons/minixnav");

    ui->typeBox->insertItem(0,"Public NAV");
    ui->typeBox->insertItem(1,"Private xNAV");
    ui->typeBox->setItemData(0,p1,Qt::DecorationRole);
    ui->typeBox->setItemData(1,p2,Qt::DecorationRole);
    ui->typeBox->setIconSize(QSize(32,32));

    connect(ui->copyClipboardButton,SIGNAL(clicked()),this,SLOT(copyToClipboard()));
    connect(ui->newAddressButton,SIGNAL(clicked()),this,SLOT(getNewAddress()));
    connect(ui->coldStakingButton,SIGNAL(clicked()),this,SLOT(getColdStakingAddress()));
    connect(ui->requestNewAddressButton,SIGNAL(clicked()),this,SLOT(showAddressHistory()));
    connect(ui->typeBox,SIGNAL(currentIndexChanged(int)),this,SLOT(showPrivateAddress(int)));

    ui->textAddress->setFont(GUIUtil::fixedPitchFont());

    ui->textAddress->setText(address);

    QSize size = ui->textAddress->document()->size().toSize();
    ui->textAddress->setFixedHeight( size.height() + 3 );
    ui->textAddress->setAlignment(Qt::AlignCenter);
}

getAddressToReceive::~getAddressToReceive()
{
    delete ui;
}

void getAddressToReceive::showPrivateAddress(int what)
{
    if (!ui->typeBox->currentIndex())
    {
        LOCK(pwalletMain->cs_wallet);
        for(const PAIRTYPE(CTxDestination, CAddressBookData)& item: pwalletMain->mapAddressBook)
        {
            const CNavcoinAddress& addressbook = item.first;
            bool fMine = IsMine(*pwalletMain, addressbook.Get());
            if(fMine)
            {
                address = QString::fromStdString(addressbook.ToString());
                break;
            }
        }
        ui->requestNewAddressButton->show();
        ui->coldStakingButton->show();
        ui->newAddressButton->show();
    }
    else
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        blsctDoublePublicKey k;
        if (pwalletMain->GetBLSCTSubAddressPublicKeys(std::make_pair(0, 0), k))
            address = QString::fromStdString(CNavcoinAddress(k).ToString());
        else
            address = "Unavailable";

        ui->requestNewAddressButton->hide();
        ui->coldStakingButton->hide();
        ui->newAddressButton->hide();
    }

    showQR();
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
    if (!IsColdStakingEnabled(chainActive.Tip(),Params().GetConsensus()))
        QMessageBox::warning(this, tr("Action not available"),
                             "<qt>Cold Staking is not active yet.</qt>");
    else {
        ColdStakingWizard wizard;
        wizard.exec();
    }
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
            qrImage.fill(0xffffff);
            unsigned char *p = code->data;
            for (int y = 0; y < code->width; y++)
            {
                for (int x = 0; x < code->width; x++)
                {
                    qrImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                    p++;
                }
            }
            QRcode_free(code);

            // Size of the qr code scaled
            int qrSize = QR_IMAGE_SIZE * GUIUtil::scale();

            // Set the pixmap and the fixed size
            ui->lblQRCode->setPixmap(QPixmap::fromImage(qrImage.scaled(qrSize, qrSize)));
            ui->lblQRCode->setMinimumSize(qrSize, qrSize);

            // Add the address to the label
            ui->textAddress->setText(address);
        }
    }
#endif
    QSize size = ui->textAddress->document()->size().toSize();
    ui->textAddress->setFixedHeight( std::max(size.height() + 3, 22) );
    ui->textAddress->setAlignment(Qt::AlignCenter);
}
