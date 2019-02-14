#include "communityfundcreatepaymentrequestdialog.h"
#include "ui_communityfundcreatepaymentrequestdialog.h"
#include "uint256.h"
#include "consensus/cfund.h"
#include "main.h"
#include "main.cpp"

CommunityFundCreatePaymentRequestDialog::CommunityFundCreatePaymentRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreatePaymentRequestDialog)
{
    ui->setupUi(this);

    //connect
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonSubmitPaymentRequest, SIGNAL(clicked()), SLOT(on_click_pushButtonSubmitPaymentRequest()));
}


bool CommunityFundCreatePaymentRequestDialog::validate()
{
    bool isValid = true;

    //uint256 hash;
    std::vector<CFund::CPaymentRequest> vec;
    if(pblocktree->GetPaymentRequestIndex(vec))
    {
        uint256 input_hash = uint256S(ui->lineEditProposalHash->text().toStdString());
        if(std::find_if(vec.begin(), vec.end(), [&input_hash](CFund::CPaymentRequest& obj) {return obj.hash == input_hash;}) != vec.end())
        {
            std::cout << "valid\n";
        }
        else {
            isValid = false;
            std::cout << "invalid\n";
        }

    }

    //amount
    if(!ui->lineEditRequestedAmount->validate())
    {
        isValid = false;
    }
    //desc
    //if(ui->plainTextEditDescription->text().)

    return isValid;
}

void CommunityFundCreatePaymentRequestDialog::on_click_pushButtonSubmitPaymentRequest()
{
    std::cout << "button clicked\n";
    this->validate();

}

CommunityFundCreatePaymentRequestDialog::~CommunityFundCreatePaymentRequestDialog()
{
    delete ui;
}
