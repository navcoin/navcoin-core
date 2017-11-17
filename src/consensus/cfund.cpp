// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/cfund.h"
#include "main.h"

void CFund::SetScriptForCommunityFundContribution(CScript &script)
{
    script.resize(4);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_CFUND;
    script[3] = OP_CFUND;
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
