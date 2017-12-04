// Copyright (c) 2017 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/cfund.h"
#include "base58.h"
#include "main.h"
#include "rpc/server.h"

void CFund::SetScriptForCommunityFundContribution(CScript &script)
{
    script.resize(2);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
}

void CFund::SetScriptForProposalVote(CScript &script, uint256 proposalhash, bool vote)
{
    script.resize(36);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PROP;
    script[3] = vote ? OP_YES : OP_NO;
    memcpy(&script[4], proposalhash.begin(), 32);
}

void CFund::SetScriptForPaymentRequestVote(CScript &script, uint256 prequesthash, bool vote)
{
    script.resize(36);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PREQ;
    script[3] = vote ? OP_YES : OP_NO;
    memcpy(&script[4], prequesthash.begin(), 32);
}

bool CFund::FindProposal(string propstr, CFund::CProposal &proposal)
{

    return pblocktree->ReadProposalIndex(uint256S("0x"+propstr), proposal);

}

bool CFund::FindProposal(uint256 prophash, CFund::CProposal &proposal)
{

    return pblocktree->ReadProposalIndex(prophash, proposal);

}

bool CFund::FindPaymentRequest(uint256 preqhash, CFund::CPaymentRequest &prequest)
{

    return pblocktree->ReadPaymentRequestIndex(preqhash, prequest);

}

bool CFund::FindPaymentRequest(string preqstr, CFund::CPaymentRequest &prequest)
{

    return pblocktree->ReadPaymentRequestIndex(uint256S("0x"+preqstr), prequest);

}

void CFund::VoteProposal(string strProp, bool vote)
{

    CFund::CProposal proposal;
    bool found = pblocktree->ReadProposalIndex(uint256S("0x"+strProp), proposal);

    if(!found || proposal.IsNull())
        return;

    vector<std::pair<std::string, bool>>::iterator it = vAddedProposalVotes.begin();
    for(; it != vAddedProposalVotes.end(); it++)
        if (strProp == (*it).first)
            break;
    RemoveConfigFile("addproposalvoteyes", strProp);
    RemoveConfigFile("addproposalvoteno", strProp);
    WriteConfigFile(vote ? "addproposalvoteyes" : "addproposalvoteno", strProp);
    if (it == vAddedProposalVotes.end())
        vAddedProposalVotes.push_back(make_pair(strProp, vote));
    else {
        vAddedProposalVotes.erase(it);
        vAddedProposalVotes.push_back(make_pair(strProp, vote));
    }
}

void CFund::VoteProposal(uint256 proposalHash, bool vote)
{
    VoteProposal(proposalHash.ToString(), vote);
}

void CFund::RemoveVoteProposal(string strProp)
{
    vector<std::pair<std::string, bool>>::iterator it = vAddedProposalVotes.begin();
    for(; it != vAddedProposalVotes.end(); it++)
        if (strProp == (*it).first)
            break;

    RemoveConfigFile("addproposalvoteyes", strProp);
    RemoveConfigFile("addproposalvoteno", strProp);
    if (it != vAddedProposalVotes.end())
        vAddedProposalVotes.erase(it);
}

void CFund::RemoveVoteProposal(uint256 proposalHash)
{
    RemoveVoteProposal(proposalHash.ToString());
}

void CFund::VotePaymentRequest(string strProp, bool vote)
{

    CFund::CPaymentRequest prequest;
    bool found = pblocktree->ReadPaymentRequestIndex(uint256S("0x"+strProp), prequest);

    if(!found || prequest.IsNull())
        return;

    vector<std::pair<std::string, bool>>::iterator it = vAddedPaymentRequestVotes.begin();
    for(; it != vAddedPaymentRequestVotes.end(); it++)
        if (strProp == (*it).first)
            break;

    RemoveConfigFile("addpaymentrequestvoteyes", strProp);
    RemoveConfigFile("addpaymentrequestvoteno", strProp);
    WriteConfigFile(vote ? "addpaymentrequestvoteyes" : "addpaymentrequestvoteno", strProp);
    if (it == vAddedPaymentRequestVotes.end())
        vAddedPaymentRequestVotes.push_back(make_pair(strProp, vote));
    else {
        vAddedPaymentRequestVotes.erase(it);
        vAddedPaymentRequestVotes.push_back(make_pair(strProp, vote));
    }

}

void CFund::VotePaymentRequest(uint256 proposalHash, bool vote)
{
    VotePaymentRequest(proposalHash.ToString(), vote);
}

void CFund::RemoveVotePaymentRequest(string strProp)
{
    vector<std::pair<std::string, bool>>::iterator it = vAddedPaymentRequestVotes.begin();
    for(; it != vAddedPaymentRequestVotes.end(); it++)
        if (strProp == (*it).first)
            break;

    RemoveConfigFile("addpaymentrequestvoteyes", strProp);
    RemoveConfigFile("addpaymentrequestvoteno", strProp);
    if (it != vAddedPaymentRequestVotes.end())
        vAddedPaymentRequestVotes.erase(it);

}

void CFund::RemoveVotePaymentRequest(uint256 proposalHash)
{
    RemoveVotePaymentRequest(proposalHash.ToString());
}

bool CFund::IsValidPaymentRequest(CTransaction tx)
{

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
          return false;

        if (valRequest.isObject())
          metadata = valRequest.get_obj();
        else
          return false;

    } catch (const UniValue& objError) {
      return false;
    } catch (const std::exception& e) {
      return false;
    }

    if(!(find_value(metadata, "n").isNum() && find_value(metadata, "s").isStr() && find_value(metadata, "h").isStr() && find_value(metadata, "i").isStr()))
        return false;

    CAmount nAmount = find_value(metadata, "n").get_int64();
    std::string Signature = find_value(metadata, "s").get_str();
    std::string Hash = find_value(metadata, "h").get_str();
    std::string strDZeel = find_value(metadata, "i").get_str();

    CFund::CProposal proposal;

    if(!CFund::FindProposal(Hash, proposal) || proposal.fState != CFund::ACCEPTED)
        return false;

    std::string Secret = "I kindly ask to withdraw " + std::to_string(nAmount) + "NAV from the proposal " + proposal.hash.ToString() + ". Payment request id: " + strDZeel;

    CNavCoinAddress addr(proposal.Address);
    CKeyID keyID;
    addr.GetKeyID(keyID);

    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(Signature.c_str(), &fInvalid);

    if (fInvalid)
        return false;

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << Secret;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig) || pubkey.GetID() != keyID)
        return false;

    if(nAmount > proposal.GetAvailable())
        return false;

    return true;

}

bool CFund::CPaymentRequest::CanVote() const {
    CFund::CProposal parent;
    if(!CFund::FindProposal(proposalhash, parent))
        return false;
    return nAmount >= parent.GetAvailable() && fState != ACCEPTED && fState != REJECTED;
}

bool CFund::IsValidProposal(CTransaction tx)
{
    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
          return false;

        if (valRequest.isObject())
          metadata = valRequest.get_obj();
        else
          return false;

    } catch (const UniValue& objError) {
      return false;
    } catch (const std::exception& e) {
      return false;
    }

    if(!(find_value(metadata, "n").isNum() && find_value(metadata, "a").isStr() && find_value(metadata, "d").isNum()))
        return false;

    CAmount nAmount = find_value(metadata, "n").get_int64();
    std::string Address = find_value(metadata, "a").get_str();
    int64_t nDeadline = find_value(metadata, "d").get_int64();
    CAmount nContribution = 0;

    for(unsigned int i=0;i<tx.vout.size();i++)
        if(tx.vout[i].IsCommunityFundContribution())
            nContribution +=tx.vout[i].nValue;

    return (nContribution >= Params().GetConsensus().nProposalMinimalFee &&
            Address != "" &&
            nAmount < MAX_MONEY &&
            nAmount > 0 &&
            nDeadline > 0);

}

bool CFund::CPaymentRequest::IsAccepted() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    return nTotalVotes > Params().GetConsensus().nQuorumVotes
           && ((float)nVotesYes > ((float)(nTotalVotes) * Params().GetConsensus().nVotesAcceptPaymentRequest));
}

bool CFund::CPaymentRequest::IsRejected() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    return nTotalVotes > Params().GetConsensus().nQuorumVotes
           && ((float)nVotesNo > ((float)(nTotalVotes) * Params().GetConsensus().nVotesRejectPaymentRequest));
}

bool CFund::CProposal::IsAccepted() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    return nTotalVotes > Params().GetConsensus().nQuorumVotes
           && ((float)nVotesYes > ((float)(nTotalVotes) * Params().GetConsensus().nVotesAcceptProposal));
}

bool CFund::CProposal::IsRejected() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    return nTotalVotes > Params().GetConsensus().nQuorumVotes
           && ((float)nVotesNo > ((float)(nTotalVotes) * Params().GetConsensus().nVotesRejectProposal));
}

void CFund::CProposal::ToJson(UniValue& ret) const {
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("description", strDZeel));
    ret.push_back(Pair("requestedAmount", (float)nAmount/COIN));
    ret.push_back(Pair("notPaidYet", (float)GetAvailable()/COIN));
    ret.push_back(Pair("userPaidFee", (float)nFee/COIN));
    ret.push_back(Pair("paymentAddress", Address));
    ret.push_back(Pair("deadline", (uint64_t)nDeadline));
    ret.push_back(Pair("votesYes", nVotesYes));
    ret.push_back(Pair("votesNo", nVotesNo));
    ret.push_back(Pair("status", GetState(pindexBestHeader->GetMedianTimePast())));
    if(fState == ACCEPTED)
        ret.push_back(Pair("approvedOnBlock", blockhash.ToString()));
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
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("description", strDZeel));
    ret.push_back(Pair("requestedAmount", (float)nAmount/COIN));
    ret.push_back(Pair("votesYes", nVotesYes));
    ret.push_back(Pair("votesNo", nVotesNo));
    ret.push_back(Pair("status", GetState()));
    if(fState == ACCEPTED) {
        ret.push_back(Pair("approvedOnBlock", blockhash.ToString()));
        ret.push_back(Pair("paidOnBlock", paymenthash.ToString()));
    }
}
