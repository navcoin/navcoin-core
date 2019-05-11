// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/cfund.h"
#include "base58.h"
#include "main.h"
#include "rpc/server.h"
#include "utilmoneystr.h"

void CFund::SetScriptForCommunityFundContribution(CScript &script)
{
    script.resize(2);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
}

void CFund::SetScriptForProposalVote(CScript &script, uint256 proposalhash, bool vote)
{
    script.resize(37);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PROP;
    script[3] = vote ? OP_YES : OP_NO;
    script[4] = 0x20;
    memcpy(&script[5], proposalhash.begin(), 32);
}

void CFund::SetScriptForPaymentRequestVote(CScript &script, uint256 prequesthash, bool vote)
{
    script.resize(37);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PREQ;
    script[3] = vote ? OP_YES : OP_NO;
    script[4] = 0x20;
    memcpy(&script[5], prequesthash.begin(), 32);
}

bool CFund::FindProposal(string propstr, CFund::CProposal &proposal)
{

    return CFund::FindProposal(uint256S("0x"+propstr), proposal);

}

bool CFund::FindProposal(uint256 prophash, CFund::CProposal &proposal)
{

    CFund::CProposal temp;
    if(pblocktree->ReadProposalIndex(prophash, temp)) {
        proposal = temp;
        return true;
    }

    return false;

}

bool CFund::FindPaymentRequest(uint256 preqhash, CFund::CPaymentRequest &prequest)
{

    CFund::CPaymentRequest temp;
    if(pblocktree->ReadPaymentRequestIndex(preqhash, temp)) {
        prequest = temp;
        return true;
    }

    return false;

}

bool CFund::FindPaymentRequest(string preqstr, CFund::CPaymentRequest &prequest)
{

    return CFund::FindPaymentRequest(uint256S("0x"+preqstr), prequest);

}

bool CFund::VoteProposal(string strProp, bool vote, bool &duplicate)
{
    AssertLockHeld(cs_main);

    CFund::CProposal proposal;
    bool found = CFund::FindProposal(uint256S("0x"+strProp), proposal);

    if(!found || proposal.IsNull() || !proposal.CanVote())
        return false;

    vector<std::pair<std::string, bool>>::iterator it = vAddedProposalVotes.begin();
    for(; it != vAddedProposalVotes.end(); it++)
        if (strProp == (*it).first) {
            if (vote == (*it).second)
                duplicate = true;
            break;
        }
    RemoveConfigFile("addproposalvoteyes", strProp);
    RemoveConfigFile("addproposalvoteno", strProp);
    WriteConfigFile(vote ? "addproposalvoteyes" : "addproposalvoteno", strProp);
    if (it == vAddedProposalVotes.end()) {
        vAddedProposalVotes.push_back(make_pair(strProp, vote));
    } else {
        vAddedProposalVotes.erase(it);
        vAddedProposalVotes.push_back(make_pair(strProp, vote));
    }
    return true;
}

bool CFund::VoteProposal(uint256 proposalHash, bool vote, bool &duplicate)
{
    return VoteProposal(proposalHash.ToString(), vote, duplicate);
}

void CFund::PoolVoteProposal(string stakingAddress, string strProp, bool vote) {
    boost::filesystem::path cFundFile = PoolGetCFundFile(stakingAddress);
    PoolRemoveFile(cFundFile, "addproposalvoteyes", strProp);
    PoolRemoveFile(cFundFile, "addproposalvoteno", strProp);
    PoolWriteFile(cFundFile, vote ? "addproposalvoteyes" : "addproposalvoteno", strProp);
}


bool CFund::RemoveVoteProposal(string strProp)
{
    vector<std::pair<std::string, bool>>::iterator it = vAddedProposalVotes.begin();
    for(; it != vAddedProposalVotes.end(); it++)
        if (strProp == (*it).first)
            break;

    RemoveConfigFile("addproposalvoteyes", strProp);
    RemoveConfigFile("addproposalvoteno", strProp);
    if (it != vAddedProposalVotes.end())
        vAddedProposalVotes.erase(it);
    else
        return false;
    return true;
}

bool CFund::RemoveVoteProposal(uint256 proposalHash)
{
    return RemoveVoteProposal(proposalHash.ToString());
}

void CFund::PoolRemoveVoteProposal(string stakingAddress, string strProp)
{
    boost::filesystem::path cFundFile = PoolGetCFundFile(stakingAddress);
    PoolRemoveFile(cFundFile, "addproposalvoteyes", strProp);
    PoolRemoveFile(cFundFile, "addproposalvoteno", strProp);
}

bool CFund::VotePaymentRequest(string strProp, bool vote, bool &duplicate)
{
    AssertLockHeld(cs_main);

    CFund::CPaymentRequest prequest;
    bool found = CFund::FindPaymentRequest(uint256S("0x"+strProp), prequest);

    if(!found || prequest.IsNull() || !prequest.CanVote(*pcoinsTip))
        return false;

    vector<std::pair<std::string, bool>>::iterator it = vAddedPaymentRequestVotes.begin();
    for(; it != vAddedPaymentRequestVotes.end(); it++)
        if (strProp == (*it).first) {
            if (vote == (*it).second)
                duplicate = true;
            break;
        }
    RemoveConfigFile("addpaymentrequestvoteyes", strProp);
    RemoveConfigFile("addpaymentrequestvoteno", strProp);
    WriteConfigFile(vote ? "addpaymentrequestvoteyes" : "addpaymentrequestvoteno", strProp);
    if (it == vAddedPaymentRequestVotes.end()) {
        vAddedPaymentRequestVotes.push_back(make_pair(strProp, vote));
    } else {
        vAddedPaymentRequestVotes.erase(it);
        vAddedPaymentRequestVotes.push_back(make_pair(strProp, vote));
    }

    return true;

}

bool CFund::VotePaymentRequest(uint256 proposalHash, bool vote, bool &duplicate)
{
    return VotePaymentRequest(proposalHash.ToString(), vote, duplicate);
}

void CFund::PoolVotePaymentRequest(string stakingAddress, string strProp, bool vote) {
    boost::filesystem::path cFundFile = PoolGetCFundFile(stakingAddress);
    PoolRemoveFile(cFundFile, "addpaymentrequestvoteyes", strProp);
    PoolRemoveFile(cFundFile, "addpaymentrequestvoteno", strProp);
    PoolWriteFile(cFundFile, vote ? "addpaymentrequestvoteyes" : "addpaymentrequestvoteno", strProp);
}

bool CFund::RemoveVotePaymentRequest(string strProp)
{
    vector<std::pair<std::string, bool>>::iterator it = vAddedPaymentRequestVotes.begin();
    for(; it != vAddedPaymentRequestVotes.end(); it++)
        if (strProp == (*it).first)
            break;

    RemoveConfigFile("addpaymentrequestvoteyes", strProp);
    RemoveConfigFile("addpaymentrequestvoteno", strProp);
    if (it != vAddedPaymentRequestVotes.end())
        vAddedPaymentRequestVotes.erase(it);
    else
        return false;
    return true;

}

bool CFund::RemoveVotePaymentRequest(uint256 proposalHash)
{
    return RemoveVotePaymentRequest(proposalHash.ToString());
}

void CFund::PoolRemoveVotePaymentRequest(string stakingAddress, string strProp)
{
    boost::filesystem::path cFundFile = PoolGetCFundFile(stakingAddress);
    PoolRemoveFile(cFundFile, "addpaymentrequestvoteyes", strProp);
    PoolRemoveFile(cFundFile, "addpaymentrequestvoteno", strProp);
}

bool CFund::IsValidPaymentRequest(CTransaction tx, CCoinsViewCache& coins, int nMaxVersion)
{    
    if(tx.strDZeel.length() > 1024)
        return error("%s: Too long strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
            return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

        if (valRequest.isObject())
            metadata = valRequest.get_obj();
        else
            return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    } catch (const UniValue& objError) {
        return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());
    } catch (const std::exception& e) {
        return error("%s: Wrong strdzeel for payment request %s: %s", __func__, tx.GetHash().ToString(), e.what());
    }

    if(!(find_value(metadata, "n").isNum() && find_value(metadata, "s").isStr() && find_value(metadata, "h").isStr() && find_value(metadata, "i").isStr()))
        return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    CAmount nAmount = find_value(metadata, "n").get_int64();
    std::string Signature = find_value(metadata, "s").get_str();
    std::string Hash = find_value(metadata, "h").get_str();
    std::string strDZeel = find_value(metadata, "i").get_str();
    int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int() : 1;

    if (nAmount < 0) {
         return error("%s: Payment Request cannot have amount less than 0: %s", __func__, tx.GetHash().ToString());
    }

    CFund::CProposal proposal;

    if(!CFund::FindProposal(Hash, proposal) || proposal.fState != CFund::ACCEPTED)
        return error("%s: Could not find parent proposal %s for payment request %s", __func__, Hash.c_str(),tx.GetHash().ToString());

    std::string sRandom = "";

    if (nVersion >= 2 && find_value(metadata, "r").isStr())
        sRandom = find_value(metadata, "r").get_str();

    std::string Secret = sRandom + "I kindly ask to withdraw " +
            std::to_string(nAmount) + "NAV from the proposal " +
            proposal.hash.ToString() + ". Payment request id: " + strDZeel;

    CNavCoinAddress addr(proposal.Address);
    if (!addr.IsValid())
        return error("%s: Address %s is not valid for payment request %s", __func__, proposal.Address, Hash.c_str(), tx.GetHash().ToString());

    CKeyID keyID;
    addr.GetKeyID(keyID);

    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(Signature.c_str(), &fInvalid);

    if (fInvalid)
        return error("%s: Invalid signature for payment request  %s", __func__, tx.GetHash().ToString());

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << Secret;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig) || pubkey.GetID() != keyID)
        return error("%s: Invalid signature for payment request %s", __func__, tx.GetHash().ToString());

    if(nAmount > proposal.GetAvailable(coins, true))
        return error("%s: Invalid requested amount for payment request %s (%d vs %d available)",
                     __func__, tx.GetHash().ToString(), nAmount, proposal.GetAvailable(coins, true));
    
    bool ret = (nVersion <= nMaxVersion);

    if(!ret)
        return error("%s: Invalid version for payment request %s", __func__, tx.GetHash().ToString());

    return nVersion <= Params().GetConsensus().nPaymentRequestMaxVersion;

}

bool CFund::CPaymentRequest::CanVote(CCoinsViewCache& coins) const
{
    AssertLockHeld(cs_main);

    CBlockIndex* pindex;
    if(txblockhash == uint256() || !mapBlockIndex.count(txblockhash))
        return false;

    pindex = mapBlockIndex[txblockhash];
    if(!chainActive.Contains(pindex))
        return false;

    CFund::CProposal proposal;
    if(!CFund::FindProposal(proposalhash, proposal))
        return false;

    return nAmount <= proposal.GetAvailable(coins) && fState != ACCEPTED && fState != REJECTED && fState != EXPIRED && !ExceededMaxVotingCycles();
}

bool CFund::CPaymentRequest::IsExpired() const {
    if(nVersion >= 2)
        return (ExceededMaxVotingCycles() && fState != ACCEPTED && fState != REJECTED);
    return false;
}

bool CFund::IsValidProposal(CTransaction tx, int nMaxVersion)
{
    if(tx.strDZeel.length() > 1024)
        return error("%s: Too long strdzeel for proposal %s", __func__, tx.GetHash().ToString());

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
            return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());

        if (valRequest.isObject())
          metadata = valRequest.get_obj();
        else
            return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());

    } catch (const UniValue& objError) {
        return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());
    } catch (const std::exception& e) {
        return error("%s: Wrong strdzeel for proposal %s: %s", __func__, tx.GetHash().ToString(), e.what());
    }

    if(!(find_value(metadata, "n").isNum() &&
            find_value(metadata, "a").isStr() &&
            find_value(metadata, "d").isNum() &&
            find_value(metadata, "s").isStr()))
    {

        return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());
    }

    CAmount nAmount = find_value(metadata, "n").get_int64();
    std::string Address = find_value(metadata, "a").get_str();
    int64_t nDeadline = find_value(metadata, "d").get_int64();
    CAmount nContribution = 0;
    int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int() : 1;

    CNavCoinAddress address(Address);
    if (!address.IsValid())
        return error("%s: Wrong address %s for proposal %s", __func__, Address.c_str(), tx.GetHash().ToString());

    for(unsigned int i=0;i<tx.vout.size();i++)
        if(tx.vout[i].IsCommunityFundContribution())
            nContribution +=tx.vout[i].nValue;

    bool ret = (nContribution >= Params().GetConsensus().nProposalMinimalFee &&
            Address != "" &&
            nAmount < MAX_MONEY &&
            nAmount > 0 &&
            nDeadline > 0 &&
            nVersion <= nMaxVersion);

    if (!ret)
        return error("%s: Wrong strdzeel %s for proposal %s", __func__, tx.strDZeel.c_str(), tx.GetHash().ToString());

    return true;

}

bool CFund::CPaymentRequest::IsAccepted() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;
    if (nVersion >= 3) {
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;
    }
    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesYes > ((float)(nTotalVotes) * Params().GetConsensus().nVotesAcceptPaymentRequest));
}

bool CFund::CPaymentRequest::IsRejected() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;
    if (nVersion >= 3) {
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;
    }
    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesNo > ((float)(nTotalVotes) * Params().GetConsensus().nVotesRejectPaymentRequest));
}

bool CFund::CPaymentRequest::ExceededMaxVotingCycles() const {
    return nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting;
}

bool CFund::CProposal::IsAccepted() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;
    if (nVersion >= 3) {
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesProposalVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;
    }
    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesYes > ((float)(nTotalVotes) * Params().GetConsensus().nVotesAcceptProposal));
}

bool CFund::CProposal::IsRejected() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;
    if (nVersion >= 3) {
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesProposalVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;
    }
    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesNo > ((float)(nTotalVotes) * Params().GetConsensus().nVotesRejectProposal));
}

bool CFund::CProposal::CanVote() const {
    AssertLockHeld(cs_main);

    CBlockIndex* pindex;
    if(txblockhash == uint256() || !mapBlockIndex.count(txblockhash))
        return false;

    pindex = mapBlockIndex[txblockhash];
    if(!chainActive.Contains(pindex))
        return false;

    return (fState == NIL) && (!ExceededMaxVotingCycles());
}

uint64_t CFund::CProposal::getTimeTillExpired(uint32_t currentTime) const
{
    if(nVersion >= 2) {
        if (mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pblockindex = mapBlockIndex[blockhash];
            return currentTime - (pblockindex->GetBlockTime() + nDeadline);
        }
    }
    return 0;
}

bool CFund::CProposal::IsExpired(uint32_t currentTime) const {
    if(nVersion >= 2) {
        if (fState == ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pBlockIndex = mapBlockIndex[blockhash];
            return (pBlockIndex->GetBlockTime() + nDeadline < currentTime);
        }
        return (fState == EXPIRED) || (fState == PENDING_VOTING_PREQ) || (ExceededMaxVotingCycles() && fState == NIL);
    } else {
        return (nDeadline < currentTime);
    }
}

bool CFund::CProposal::ExceededMaxVotingCycles() const {
    return nVotingCycle > Params().GetConsensus().nCyclesProposalVoting;
}

CAmount CFund::CProposal::GetAvailable(CCoinsViewCache& coins, bool fIncludeRequests) const
{
    AssertLockHeld(cs_main);

    CAmount initial = nAmount;
    for (unsigned int i = 0; i < vPayments.size(); i++)
    {
        CFund::CPaymentRequest prequest;
        if(FindPaymentRequest(vPayments[i], prequest))
        {
            if (!coins.HaveCoins(prequest.hash))
            {
                CBlockIndex* pindex;
                if(prequest.txblockhash == uint256() || !mapBlockIndex.count(prequest.txblockhash))
                    continue;
                pindex = mapBlockIndex[prequest.txblockhash];
                if(!chainActive.Contains(pindex))
                    continue;
            }
            if((fIncludeRequests && prequest.fState != REJECTED && prequest.fState != EXPIRED) || (!fIncludeRequests && prequest.fState == ACCEPTED))
                initial -= prequest.nAmount;
        }
    }
    return initial;
}

std::string CFund::CProposal::ToString(CCoinsViewCache& coins, uint32_t currentTime) const {
    std::string str;
    str += strprintf("CProposal(hash=%s, nVersion=%i, nAmount=%f, available=%f, nFee=%f, address=%s, nDeadline=%u, nVotesYes=%u, "
                     "nVotesNo=%u, nVotingCycle=%u, fState=%s, strDZeel=%s, blockhash=%s)",
                     hash.ToString(), nVersion, (float)nAmount/COIN, (float)GetAvailable(coins)/COIN, (float)nFee/COIN, Address, nDeadline,
                     nVotesYes, nVotesNo, nVotingCycle, GetState(currentTime), strDZeel, blockhash.ToString().substr(0,10));
    for (unsigned int i = 0; i < vPayments.size(); i++) {
        CFund::CPaymentRequest prequest;
        if(FindPaymentRequest(vPayments[i], prequest))
            str += "\n    " + prequest.ToString();
    }
    return str + "\n";
}

bool CFund::CProposal::HasPendingPaymentRequests(CCoinsViewCache& coins) const {
    AssertLockHeld(cs_main);

    for (unsigned int i = 0; i < vPayments.size(); i++)
    {
        CFund::CPaymentRequest prequest;
        if(FindPaymentRequest(vPayments[i], prequest))
            if(prequest.CanVote(coins))
                return true;
    }
    return false;
}

std::string CFund::CProposal::GetState(uint32_t currentTime) const {
    std::string sFlags = "pending";
    if(IsAccepted()) {
        sFlags = "accepted";
        if(fState == PENDING_FUNDS)
            sFlags += " waiting for enough coins in fund";
        else if(fState != ACCEPTED)
            sFlags += " waiting for end of voting period";
    }
    if(IsRejected()) {
        sFlags = "rejected";
        if(fState != REJECTED)
            sFlags += " waiting for end of voting period";
    }
    if(currentTime > 0 && IsExpired(currentTime)) {
        sFlags = "expired";
        if(fState != EXPIRED && !ExceededMaxVotingCycles())
            // This branch only occurs when a proposal expires due to exceeding its nDeadline during a voting cycle, not due to exceeding max voting cycles
            sFlags += " waiting for end of voting period";
    }
    if(fState == PENDING_VOTING_PREQ) {
        sFlags = "expired pending voting of payment requests";
    }
    return sFlags;
}

void CFund::CProposal::ToJson(UniValue& ret, CCoinsViewCache& coins) const {
    AssertLockHeld(cs_main);

    ret.push_back(Pair("version", nVersion));
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("blockHash", txblockhash.ToString()));
    ret.push_back(Pair("description", strDZeel));
    ret.push_back(Pair("requestedAmount", FormatMoney(nAmount)));
    ret.push_back(Pair("notPaidYet", FormatMoney(GetAvailable(coins))));
    ret.push_back(Pair("userPaidFee", FormatMoney(nFee)));
    ret.push_back(Pair("paymentAddress", Address));
    if(nVersion >= 2) {
        ret.push_back(Pair("proposalDuration", (uint64_t)nDeadline));
        if (fState == ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pBlockIndex = mapBlockIndex[blockhash];
            ret.push_back(Pair("expiresOn", pBlockIndex->GetBlockTime() + (uint64_t)nDeadline));
        }
    } else {
        ret.push_back(Pair("expiresOn", (uint64_t)nDeadline));
    }
    ret.push_back(Pair("votesYes", nVotesYes));
    ret.push_back(Pair("votesNo", nVotesNo));
    ret.push_back(Pair("votingCycle", (uint64_t)std::min(nVotingCycle, Params().GetConsensus().nCyclesProposalVoting)));
    // votingCycle does not return higher than nCyclesProposalVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesProposalVoting
    ret.push_back(Pair("status", GetState(chainActive.Tip()->GetBlockTime())));
    ret.push_back(Pair("state", (uint64_t)fState));
    if(fState == ACCEPTED)
        ret.push_back(Pair("stateChangedOnBlock", blockhash.ToString()));
    if(vPayments.size() > 0) {
        UniValue arr(UniValue::VARR);
        for(unsigned int i = 0; i < vPayments.size(); i++) {
            UniValue preq(UniValue::VOBJ);
            CFund::CPaymentRequest prequest;
            if(CFund::FindPaymentRequest(vPayments[i],prequest)) {
                prequest.ToJson(preq);
                arr.push_back(preq);
            }
        }
        ret.push_back(Pair("paymentRequests", arr));
    }
}

void CFund::CPaymentRequest::ToJson(UniValue& ret) const {
    ret.push_back(Pair("version", nVersion));
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("blockHash", txblockhash.ToString()));
    ret.push_back(Pair("description", strDZeel));
    ret.push_back(Pair("requestedAmount", FormatMoney(nAmount)));
    ret.push_back(Pair("votesYes", nVotesYes));
    ret.push_back(Pair("votesNo", nVotesNo));
    ret.push_back(Pair("votingCycle", (uint64_t)std::min(nVotingCycle, Params().GetConsensus().nCyclesPaymentRequestVoting)));
    // votingCycle does not return higher than nCyclesPaymentRequestVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesPaymentRequestVoting
    ret.push_back(Pair("status", GetState()));
    ret.push_back(Pair("state", (uint64_t)fState));
    ret.push_back(Pair("stateChangedOnBlock", blockhash.ToString()));
    if(fState == ACCEPTED) {        
        ret.push_back(Pair("paidOnBlock", paymenthash.ToString()));
    }
}

