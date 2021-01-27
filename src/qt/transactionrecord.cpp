// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <base58.h>
#include <consensus/consensus.h>
#include <main.h>
#include <timedata.h>
#include <wallet/wallet.h>

#include <stdint.h>

#include <boost/algorithm/string/join.hpp>

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {
        // Ensures we show generated coins / mined transactions at depth 1
        if (!wtx.IsInMainChain())
        {
            return false;
        }
    }

    if (wtx.IsBLSCT())
    {
        if (wtx.isAbandoned())
        {
            return false;
        }
    }

    return true;
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();
    isminefilter dCFilter = wtx.IsCoinStake() ? wallet->IsMine(wtx.vout[1]) : ISMINE_ALL;
    CAmount nCredit = wtx.GetCredit(dCFilter, false);
    CAmount nPrivateCredit = wtx.GetCredit(ISMINE_SPENDABLE_PRIVATE);
    CAmount nPublicCredit = wtx.GetCredit(ISMINE_SPENDABLE);
    CAmount nDebit = wtx.GetDebit(dCFilter);
    CAmount nPrivateDebit = wtx.GetDebit(ISMINE_SPENDABLE_PRIVATE);
    CAmount nPublicDebit = wtx.GetDebit(ISMINE_SPENDABLE);
    CAmount nCFundCredit = wtx.GetDebit(ISMINE_ALL);
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;
    std::vector<std::string> vMemos;

    for (auto &s:wtx.vMemos)
    {
        if (s != "" && s != "Fee" && s != "Change")
        {
            vMemos.push_back(s);
        }
    }

    bool involvesWatchAddress = false;
    isminetype fAllFromMe = ISMINE_SPENDABLE;
    for(const CTxIn& txin: wtx.vin)
    {
        isminetype mine = wallet->IsMine(txin);
        if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
        if(fAllFromMe > mine) fAllFromMe = mine;
    }

    if ((nNet > 0 && fAllFromMe) || wtx.IsCoinBase() || wtx.IsCoinStake())
    {
        //
        // Credit
        //
        unsigned int i = 0;
        bool fBLSCT = false;
        CAmount nReward = -nDebit;
        if (wtx.IsCoinStake())
        {
            for (unsigned int j = 0; j < wtx.vout.size(); j++)
                if (wtx.vout[j].scriptPubKey == wtx.vout[1].scriptPubKey)
                    nReward += wtx.vout[j].nValue;
        }

        bool fAddedReward = false;

        for(const CTxOut& txout: wtx.vout)
        {
            isminetype mine = wallet->IsMine(txout);
            if(mine)
            {
                TransactionRecord sub(hash, nTime);
                CTxDestination address;
                sub.idx = parts.size(); // sequence number
                sub.credit = txout.nValue;
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                {
                    // Received by Navcoin Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = CNavcoinAddress(address).ToString();
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.IsCoinBase())
                {
                    // Generated
                    sub.type = TransactionRecord::Generated;
                    if(i > 0)
                        sub.type = TransactionRecord::CFundPayment;
                }
                if (wtx.IsCoinStake())
                {
                    // Generated (proof-of-stake)

                    if (wtx.vout[i].scriptPubKey == wtx.vout[1].scriptPubKey)
                    {
                        if (fAddedReward)
                        {
                            i++;
                            continue; // only append details of the address with reward output
                        }
                        else
                        {
                            fAddedReward = true;

                            sub.type = TransactionRecord::Staked;
                            sub.credit = nReward;
                        }
                    }
                    else
                    {
                        sub.type = TransactionRecord::Staked;
                        if (wtx.IsCTOutput())
                        {
                            sub.type = TransactionRecord::AnonTxRecv;
                            for(auto&it: wtx.vAmounts)
                            {
                                sub.credit += it;
                            }
                            sub.memo = "Staking Reward";
                        }
                    }
                }
                else if(txout.HasRangeProof())
                {
                    sub.type = TransactionRecord::AnonTxRecv;
                    sub.memo = wtx.vMemos[i];
                    sub.credit = wtx.vAmounts[i];
                    if (sub.memo.substr(0,13) == "Mixing Reward")
                    {
                        sub.type = TransactionRecord::MixingReward;
                        sub.credit += nReward;
                    }
                    fBLSCT = true;
                }

                if(txout.scriptPubKey.IsCommunityFundContribution())
                {
                    sub.type = TransactionRecord::CFund;
                }

                if (sub.credit > 0)
                {
                    parts.append(sub);
                }
            }
            i++;
        }
    }
    else
    {
        isminetype fAllToMe = ISMINE_SPENDABLE;
        bool fHasSomeNormalOut = false;
        for(const CTxOut& txout: wtx.vout)
        {
            if (txout.scriptPubKey.IsFee()) continue;
            isminetype mine = wallet->IsMine(txout);
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(mine & ISMINE_SPENDABLE) fHasSomeNormalOut = true;
            if(fAllToMe > mine) fAllToMe = mine;
        }

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            CAmount nChange = wtx.GetChange();

            parts.append(TransactionRecord(hash, nTime, TransactionRecord::Fee, "",
                            -(nDebit - nChange), nCredit - nChange));
            parts.last().involvesWatchAddress = involvesWatchAddress;   // maybe pass to TransactionRecord as constructor argument

            if (wtx.IsBLSInput() && fHasSomeNormalOut)
            {
                 parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelfPublic, "",
                                                0, nPublicCredit));
            }
             else if (!wtx.IsBLSInput() && wtx.IsCTOutput())
            {
                 parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelfPrivate, "",
                                                0, nPrivateCredit));
            }
        }
        else if (fAllFromMe && !wtx.IsBLSInput())
        {
            //
            // Debit
            //
            CAmount blsFee = wtx.GetFee();
            CAmount nTxFee = wtx.IsBLSCT() ? blsFee : nDebit - wtx.GetValueOut();
            CAmount nSentToOthersPublic = 0;

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];
                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();
                sub.involvesWatchAddress = involvesWatchAddress;

                if(wallet->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    // Sent to Navcoin Address
                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = CNavcoinAddress(address).ToString();
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];
                }

                CAmount nValue = txout.nValue;
                /* Add fee to first output */
                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }
                sub.debit = -nValue;

                if(txout.HasRangeProof())
                {
                    sub.type = TransactionRecord::AnonTxSend;
                    sub.memo = wtx.vMemos[nOut];
                    sub.debit = -wtx.vAmounts[nOut];
                }

                else if (txout.scriptPubKey.IsCommunityFundContribution())
                    sub.type = TransactionRecord::CFund;

                else if (txout.scriptPubKey.IsFee())
                {
                    sub.type = TransactionRecord::Fee;
                }

                nSentToOthersPublic += -sub.debit;

                parts.append(sub);
            }
        }
        else if (fAllFromMe && wtx.IsBLSInput())
        {
            //
            // Debit
            //
            CAmount blsFee = wtx.GetFee();
            CAmount nTxFee = wtx.IsBLSCT() ? blsFee : nDebit - wtx.GetValueOut();
            CAmount nSentToOthersPublic = 0;

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];
                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();
                sub.involvesWatchAddress = involvesWatchAddress;

                if(wallet->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    // Sent to Navcoin Address
                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = CNavcoinAddress(address).ToString();
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];
                }

                CAmount nValue = txout.nValue;

                sub.debit = -nValue;

                if (txout.scriptPubKey.IsCommunityFundContribution())
                    sub.type = TransactionRecord::CFund;

                if (txout.scriptPubKey.IsFee())
                    sub.type = TransactionRecord::Fee;

                if(txout.HasRangeProof())
                {
                    sub.type = TransactionRecord::AnonTxSend;
                    sub.memo = wtx.vMemos[nOut];
                    sub.debit = -wtx.vAmounts[nOut];
                }

                if (sub.debit == 0)
                    continue;

                nSentToOthersPublic += -sub.debit;

                parts.append(sub);
            }

            if (nPrivateCredit > nPrivateDebit + nSentToOthersPublic)
            {
                parts.append(TransactionRecord(hash, nTime, TransactionRecord::AnonTxSend, "", nPrivateCredit-nSentToOthersPublic-nPrivateDebit, 0));
            }
        }
        else
        {
            //
            // Mixed debit transaction, can't break down payees
            //

            CAmount sent = nPrivateDebit + nPublicDebit;

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];
                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();
                sub.involvesWatchAddress = involvesWatchAddress;

                if(wallet->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    if(txout.HasRangeProof())
                    {
                        if (wtx.vMemos[nOut] == "Change" || wtx.vAmounts[nOut] == 0)
                        {
                            sent -= wtx.vAmounts[nOut];
                            continue;
                        }
                        if (wtx.vMemos[nOut].substr(0,13) == "Mixing Reward")
                        {
                            CAmount reward = wtx.vMemos[nOut].size() > 13 ? std::stof(wtx.vMemos[nOut].substr(15)) * COIN : 0.1 * COIN;
                            sent -= wtx.vAmounts[nOut] - reward;
                            sub.credit = reward;
                            sub.type = TransactionRecord::MixingReward;
                            parts.append(sub);
                        }
                        else
                        {
                            sub.type = TransactionRecord::AnonTxRecv;
                            sub.memo = wtx.vMemos[nOut];
                            sub.credit = wtx.vAmounts[nOut];
                            parts.append(sub);
                        }
                    }
                    else
                    {
                        sub.credit = txout.nValue;
                        CTxDestination address;
                        if (ExtractDestination(txout.scriptPubKey, address))
                        {
                            sub.address = CNavcoinAddress(address).ToString();

                            if (IsMine(*wallet, address))
                            {
                                // Received by Navcoin Address
                                sub.type = TransactionRecord::RecvWithAddress;
                            }
                            else
                            {
                                sub.type = TransactionRecord::Other;
                            }
                        }
                        else
                        {
                            sub.type = TransactionRecord::Other;
                        }
                        parts.append(sub);
                    }
                }
                else
                {
                    if(txout.HasRangeProof())
                    {
                        if (wtx.vAmounts[nOut] == 0)
                            continue;

                        sub.type = TransactionRecord::AnonTxSend;
                        sub.memo = wtx.vMemos[nOut];
                        sub.debit = -wtx.vAmounts[nOut];
                        sent -= -sub.debit;

                        parts.append(sub);
                    }
                }
            }
            CAmount nTxFee = wtx.GetFee();
            if (wtx.IsBLSInput())
            {
                if (sent > 0)
                {
                    parts.append(TransactionRecord(hash, nTime, TransactionRecord::AnonTxSend, "", -sent, 0));
                }
                else if (sent < 0)
                {
                    parts.append(TransactionRecord(hash, nTime, TransactionRecord::AnonTxRecv, "", 0, sent));
                }
            }
            else
            {
                parts.append(TransactionRecord(hash, nTime, TransactionRecord::Other, "", nNet, 0));
                parts.last().involvesWatchAddress = involvesWatchAddress;
            }
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex* pindex = nullptr;
    BlockMap::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = chainActive.Height();

    if (!CheckFinalTx(wtx))
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = wtx.nLockTime - chainActive.Height();
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    // For generated transactions, determine maturity
    else if(type == TransactionRecord::Generated || type == TransactionRecord::Staked)
    {
        if (wtx.GetBlocksToMaturity() > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = TransactionStatus::MaturesWarning;
            }
            else
            {
                status.status = TransactionStatus::Orphan;
            }
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
    else
    {
        if (status.depth < 0)
        {
            status.status = TransactionStatus::Conflicted;
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = TransactionStatus::Offline;
        }
        else if (status.depth == 0)
        {
            status.status = TransactionStatus::Unconfirmed;
            if (wtx.isAbandoned())
                status.status = TransactionStatus::Abandoned;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = TransactionStatus::Confirming;
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }

}

bool TransactionRecord::statusUpdateNeeded()
{
    AssertLockHeld(cs_main);
    return status.cur_num_blocks != chainActive.Height();
}

QString TransactionRecord::getTxID() const
{
    return QString::fromStdString(hash.ToString());
}

int TransactionRecord::getOutputIndex() const
{
    return idx;
}
