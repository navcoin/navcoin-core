// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/cfund.h"
#include "main.h"

void CFund::SetScriptForCommunityFundContribution(CScript &script)
{
    script.resize(4);
    script[0] = OP_RETURN;
    script[1] = 0x20;
    script[2] = 0x20;
    script[3] = 0x20;
}

void CFund::VoteProposal(string strProp)
{
    vector<string>::iterator it = vAddedProposalVotes.begin();
    for(; it != vAddedProposalVotes.end(); it++)
        if (strProp == *it)
            break;

    CFund::CProposal proposal;
    bool found = pblocktree->ReadProposalIndex(uint256S("0x"+strProp), proposal);

    if(!found || proposal.IsNull())
        return;

    WriteConfigFile("addproposalvote", strProp);
    if (it == vAddedProposalVotes.end())
        vAddedProposalVotes.push_back(strProp);
}

void CFund::VoteProposal(uint256 proposalHash)
{
    VoteProposal(proposalHash.ToString());
}

void CFund::RemoveVoteProposal(string strProp)
{
    vector<string>::iterator it = vAddedProposalVotes.begin();
    for(; it != vAddedProposalVotes.end(); it++)
        if (strProp == *it)
            break;

    RemoveConfigFile("addproposalvote", strProp);
    if (it != vAddedProposalVotes.end())
        vAddedProposalVotes.erase(it);
}

void CFund::RemoveVoteProposal(uint256 proposalHash)
{
    RemoveVoteProposal(proposalHash.ToString());
}

void CFund::VotePaymentRequest(string strProp)
{
    vector<string>::iterator it = vAddedPaymentRequestVotes.begin();
    for(; it != vAddedPaymentRequestVotes.end(); it++)
        if (strProp == *it)
            break;

    CFund::CPaymentRequest prequest;
    bool found = pblocktree->ReadPaymentRequestIndex(uint256S("0x"+strProp), prequest);

    if(!found || prequest.IsNull())
        return;

    WriteConfigFile("addpaymentrequestvote", strProp);
    if (it == vAddedPaymentRequestVotes.end())
        vAddedPaymentRequestVotes.push_back(strProp);
}

void CFund::VotePaymentRequest(uint256 proposalHash)
{
    VotePaymentRequest(proposalHash.ToString());
}

void CFund::RemoveVotePaymentRequest(string strProp)
{
    vector<string>::iterator it = vAddedPaymentRequestVotes.begin();
    for(; it != vAddedPaymentRequestVotes.end(); it++)
        if (strProp == *it)
            break;

    RemoveConfigFile("addpaymentrequestvote", strProp);
    if (it != vAddedPaymentRequestVotes.end())
        vAddedPaymentRequestVotes.erase(it);
}

void CFund::RemoveVotePaymentRequest(uint256 proposalHash)
{
    RemoveVotePaymentRequest(proposalHash.ToString());
}
