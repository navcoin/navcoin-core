// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletmodeltransaction.h>

#include <policy/policy.h>
#include <wallet/wallet.h>
#include <util.h>

WalletModelTransaction::WalletModelTransaction(const QList<SendCoinsRecipient> &recipients) :
    recipients(recipients),
    walletTransaction(0),
    keyChange(0),
    fee(0),
    fSpendsColdStaking(false)
{

}

WalletModelTransaction::~WalletModelTransaction()
{
    delete keyChange;
    delete walletTransaction;
}

QList<SendCoinsRecipient> WalletModelTransaction::getRecipients()
{
    return recipients;
}

CWalletTx *WalletModelTransaction::getTransaction()
{
    return walletTransaction;
}

unsigned int WalletModelTransaction::getTransactionSize()
{
    return (!walletTransaction ? 0 : ::GetVirtualTransactionSize(*walletTransaction));
}

CAmount WalletModelTransaction::getTransactionFee()
{
    return fee;
}

void WalletModelTransaction::setTransactionFee(const CAmount& newFee)
{
    fee = newFee;
}

void WalletModelTransaction::reassignAmounts(int nChangePosRet, CWalletTx* wTx, int index)
{
    int i = 0, j = 0;
    for (QList<SendCoinsRecipient>::iterator it = recipients.begin(); it != recipients.end(); ++it)
    {
        SendCoinsRecipient& rcp = (*it);

        if(j != index) continue;

        {
            if (i == nChangePosRet)
                i++;

            rcp.amount = wTx->vout[i].nValue;

            i++;
        }
        j++;
    }
}

CAmount WalletModelTransaction::getTotalTransactionAmount()
{
    CAmount totalTransactionAmount = 0;
    for(const SendCoinsRecipient &rcp: recipients)
    {
        totalTransactionAmount += rcp.amount;
    }
    return totalTransactionAmount;
}

void WalletModelTransaction::newPossibleKeyChange(CWallet *wallet)
{
    keyChange = new CReserveKey(wallet);
}

CReserveKey *WalletModelTransaction::getPossibleKeyChange()
{
    return keyChange;
}

void WalletModelTransaction::newPossibleBLSCTKeyChange(CWallet *wallet)
{
    blsctKeyChange = new std::vector<shared_ptr<CReserveBLSCTKey>>();

    for (unsigned int i = 0; i < 3; i++)
    {
        shared_ptr<CReserveBLSCTKey> rk(new CReserveBLSCTKey(wallet));
        blsctKeyChange->insert(blsctKeyChange->begin(), std::move(rk));
    }
}

std::vector<shared_ptr<CReserveBLSCTKey>> *WalletModelTransaction::getPossibleBLSCTKeyChange()
{
    return blsctKeyChange;
}
