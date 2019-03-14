/**
 * @file       Benchmark.cpp
 *
 * @brief      Benchmarking tests for Zerocoin.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 The NavCoin Core developers

#include <boost/test/unit_test.hpp>
#include <string>
#include <iostream>
#include <fstream>
// #include <curses.h>
#include <exception>
#include <cstdlib>
#include <sys/time.h>
#include "streams.h"
#include "libzeroct/ParamGeneration.h"
#include "libzeroct/Coin.h"
#include "libzeroct/CoinSpend.h"
#include "libzeroct/Accumulator.h"
#include "test/test_navcoin.h"

using namespace std;
using namespace libzeroct;

#define COLOR_STR_GREEN   "\033[32m"
#define COLOR_STR_NORMAL  "\033[0m"
#define COLOR_STR_RED     "\033[31m"

#define TESTS_COINS_TO_ACCUMULATE   50

// Global test counters
uint32_t    ggNumTests        = 0;
uint32_t    ggSuccessfulTests = 0;

// Global coin array
PrivateCoin    *ggCoins[TESTS_COINS_TO_ACCUMULATE];

// Global params
ZeroCTParams *gg_Params;
CKey privKey_;
CPubKey pubKey_;
CBigNum obfuscation_jj1;
CBigNum obfuscation_jj2;
CBigNum obfuscation_kk1;
CBigNum obfuscation_kk2;
CBigNum blindingCommitment1_;
CBigNum blindingCommitment2_;
libzeroct::ObfuscationValue obfuscation_jj;
libzeroct::ObfuscationValue obfuscation_kk;
libzeroct::BlindingCommitment blindingCommitment_;

//////////
// Utility routines
//////////

class Timer
{
    timeval timer[2];

public:

    timeval start()
    {
        gettimeofday(&this->timer[0], NULL);
        return this->timer[0];
    }

    timeval stop()
    {
        gettimeofday(&this->timer[1], NULL);
        return this->timer[1];
    }

    int duration() const
    {
        int secs(this->timer[1].tv_sec - this->timer[0].tv_sec);
        int usecs(this->timer[1].tv_usec - this->timer[0].tv_usec);

        if(usecs < 0)
        {
            --secs;
            usecs += 1000000;
        }

        return static_cast<int>(secs * 1000 + usecs / 1000.0 + 0.5);
    }
};

// Global timer
Timer timer;

void
gLogTestResult(string testName, bool (*testPtr)())
{
    string colorGreen(COLOR_STR_GREEN);
    string colorNormal(COLOR_STR_NORMAL);
    string colorRed(COLOR_STR_RED);

    cout << "Testing if " << testName << "..." << endl;

    bool testResult = testPtr();

    if (testResult == true) {
        cout << "\t" << colorGreen << "[PASS]"  << colorNormal << endl;
        ggSuccessfulTests++;
    } else {
        cout << colorRed << "\t[FAIL]" << colorNormal << endl;
    }

    ggNumTests++;
}

CBigNum
gGetTestModulus()
{
    static CBigNum testModulus(0);

    // TODO: should use a hard-coded RSA modulus for testing
    if (!testModulus) {
        CBigNum p, q;
        p = CBigNum::generatePrime(1024, false);
        q = CBigNum::generatePrime(1024, false);
        testModulus = p * q;
    }

    return testModulus;
}

//////////
// Test routines
//////////


bool
Testb_GenRSAModulus()
{
    CBigNum result = gGetTestModulus();

    if (!result) {
        return false;
    }
    else {
        return true;
    }
}

bool
Testb_CalcParamSizes()
{
    bool result = true;
#if 0

    uint32_t pLen, qLen;

    try {
        calculateGroupParamLengths(4000, 80, &pLen, &qLen);
        if (pLen < 1024 || qLen < 256) {
            result = false;
        }
        calculateGroupParamLengths(4000, 96, &pLen, &qLen);
        if (pLen < 2048 || qLen < 256) {
            result = false;
        }
        calculateGroupParamLengths(4000, 112, &pLen, &qLen);
        if (pLen < 3072 || qLen < 320) {
            result = false;
        }
        calculateGroupParamLengths(4000, 120, &pLen, &qLen);
        if (pLen < 3072 || qLen < 320) {
            result = false;
        }
        calculateGroupParamLengths(4000, 128, &pLen, &qLen);
        if (pLen < 3072 || qLen < 320) {
            result = false;
        }
    } catch (exception &e) {
        result = false;
    }
#endif

    return result;
}

bool
Testb_GenerateGroupParams()
{
    uint32_t pLen = 1024, qLen = 256, count;
    IntegerGroupParams group;

    for (count = 0; count < 1; count++) {

        try {
            group = deriveIntegerGroupParams(calculateSeed(gGetTestModulus(), "test", ZEROCOIN_DEFAULT_SECURITYLEVEL, "TEST GROUP"), pLen, qLen);
        } catch (std::runtime_error e) {
            cout << "Caught exception " << e.what() << endl;
            return false;
        }

        // Now perform some simple tests on the resulting parameters
        if ((uint32_t)group.groupOrder.bitSize() < qLen || (uint32_t)group.modulus.bitSize() < pLen) {
            return false;
        }

        CBigNum c = group.g.pow_mod(group.groupOrder, group.modulus);
        //cout << "g^q mod p = " << c << endl;
        if (!(c.isOne())) return false;

        // Try at multiple parameter sizes
        pLen = pLen * 1.5;
        qLen = qLen * 1.5;
    }

    return true;
}

bool
Testb_ParamGen()
{
    bool result = true;

    try {
        timer.start();
        // Instantiating testParams runs the parameter generation code
        ZeroCTParams testParams(gGetTestModulus(),ZEROCOIN_DEFAULT_SECURITYLEVEL);
        timer.stop();

        cout << "\tPARAMGEN ELAPSED TIME: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s" << endl;
    } catch (runtime_error e) {
        cout << e.what() << endl;
        result = false;
    }

    return result;
}

bool
Testb_Accumulator()
{
    // This test assumes a list of coins were generated during
    // the Testb_MintCoin() test.
    if (ggCoins[0] == NULL) {
        return false;
    }
    try {
        // Accumulate the coin list from first to last into one accumulator
        Accumulator accOne(&gg_Params->accumulatorParams);
        Accumulator accTwo(&gg_Params->accumulatorParams);
        Accumulator accThree(&gg_Params->accumulatorParams);
        Accumulator accFour(&gg_Params->accumulatorParams);
        AccumulatorWitness wThree(gg_Params, accThree, ggCoins[0]->getPublicCoin());

        for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
            accOne += ggCoins[i]->getPublicCoin();
            accTwo += ggCoins[TESTS_COINS_TO_ACCUMULATE - (i+1)]->getPublicCoin();
            accThree += ggCoins[i]->getPublicCoin();
            wThree += ggCoins[i]->getPublicCoin();
            if(i != 0) {
                accFour += ggCoins[i]->getPublicCoin();
            }
        }

        // Compare the accumulated results
        if (accOne.getValue() != accTwo.getValue() || accOne.getValue() != accThree.getValue()) {
            cout << "Accumulators don't match" << endl;
            return false;
        }

        if(accFour.getValue() != wThree.getValue()) {
            cout << "Witness math not working," << endl;
            return false;
        }

        // Verify that the witness is correct
        if (!wThree.VerifyWitness(accThree, ggCoins[0]->getPublicCoin()) ) {
            cout << "Witness not valid" << endl;
            return false;
        }

    } catch (runtime_error e) {
        cout << e.what() << endl;
        return false;
    }

    return true;
}

bool
Testb_MintCoin()
{
    int mintTotal = 0;
    int bpproveTotal = 0;
    int bpverifyTotal = 0;
    try {
        // Generate a list of coins
        for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
            CBigNum rpdata;
            timer.start();
            PublicCoin pubCoin(gg_Params,pubKey_,blindingCommitment_, "test", COIN, &rpdata);
            timer.stop();
            mintTotal += timer.duration();
            ggCoins[i] = new PrivateCoin(gg_Params, privKey_,pubCoin.getPubKey(),blindingCommitment_,pubCoin.getValue(), pubCoin.getPaymentId(), pubCoin.getAmount());

            BulletproofsRangeproof bprp(&gg_Params->coinCommitmentGroup);
            CBN_matrix mValueCommitments;

            CBN_vector values;
            CBN_vector gammas;

            values.push_back(CBigNum(ggCoins[i]->getAmount()));
            gammas.push_back(rpdata);

            timer.start();
            bprp.Prove(values, gammas);
            timer.stop();

            bpproveTotal += timer.duration();

            std::vector<BulletproofsRangeproof> proofs;
            proofs.push_back(bprp);

            CBN_vector valueCommitments = bprp.GetValueCommitments();
            mValueCommitments.push_back(valueCommitments);

            timer.start();
            VerifyBulletproof(&gg_Params->coinCommitmentGroup, proofs, mValueCommitments);
            timer.stop();

            bpverifyTotal += timer.duration();

        }

    } catch (exception &e) {
        return false;
    }
    cout << "\tMINT ELAPSED TIME:\n\t\tTotal: " << mintTotal << " ms\t" << mintTotal*0.001 << " s\n\t\tPer Coin: " << mintTotal/TESTS_COINS_TO_ACCUMULATE << " ms\t" << (mintTotal/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s" << endl;
    cout << "\tRANGEPROOF PROVE ELAPSED TIME:\n\t\tTotal: " << bpproveTotal << " ms\t" << bpproveTotal*0.001 << " s\n\t\tPer Coin: " << bpproveTotal/TESTS_COINS_TO_ACCUMULATE << " ms\t" << (bpproveTotal/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s" << endl;
    cout << "\tRANGEPROOF VERIFY ELAPSED TIME:\n\t\tTotal: " << bpverifyTotal << " ms\t" << bpverifyTotal*0.001 << " s\n\t\tPer Coin: " << bpverifyTotal/TESTS_COINS_TO_ACCUMULATE << " ms\t" << (bpverifyTotal/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s" << endl;

    return true;
}

bool
Testb_MintAndSpend()
{
    try {
        // This test assumes a list of coins were generated in Testb_MintCoin()
        if (ggCoins[0] == NULL)
        {
            // No coins: mint some.
            Testb_MintCoin();
            if (ggCoins[0] == NULL) {
                return false;
            }
        }

        // Accumulate the list of generated coins into a fresh accumulator.
        // The first one gets marked as accumulated for a witness, the
        // others just get accumulated normally.
        Accumulator acc(&gg_Params->accumulatorParams);
        AccumulatorWitness wAcc(gg_Params, acc, ggCoins[0]->getPublicCoin());

        timer.start();
        for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
            acc += ggCoins[i]->getPublicCoin();
        }
        timer.stop();
        cout << "\tACCUMULATOR ELAPSED TIME:\n\t\tTotal: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s\n\t\tPer Element: " << timer.duration()/TESTS_COINS_TO_ACCUMULATE << " ms\t" << (timer.duration()/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s" << endl;

        timer.start();
        for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
            wAcc +=ggCoins[i]->getPublicCoin();
        }
        timer.stop();

        cout << "\tWITNESS ELAPSED TIME: \n\t\tTotal: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s\n\t\tPer Element: " << timer.duration()/TESTS_COINS_TO_ACCUMULATE << " ms\t" << (timer.duration()/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s" << endl;

        CBigNum r;

        // Now spend the coin
        timer.start();
        CoinSpend spend(gg_Params, *(ggCoins[0]), acc, 0, wAcc, 0, SpendType::SPEND, obfuscation_jj, obfuscation_kk, r);
        timer.stop();

        cout << "\tSPEND ELAPSED TIME: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s" << endl;

        // Serialize the proof and deserialize into newSpend
        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);

        timer.start();
        ss << spend;
        timer.stop();

        CoinSpend newSpend(gg_Params, ss);

        cout << "\tSERIALIZE ELAPSED TIME: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s" << endl;

        // Finally, see if we can verify the deserialized proof (return our result)
        timer.start();
        bool ret = newSpend.Verify(acc);
        timer.stop();
        cout << "\tSPEND VERIFY ELAPSED TIME: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s" << endl;

        return ret;
    } catch (runtime_error &e) {
        cout << e.what() << endl;
        return false;
    }

    return false;
}

void
Testb_RunAllTests()
{
    // Make a new set of parameters from a random RSA modulus
    gg_Params = new ZeroCTParams(gGetTestModulus());

    // Generate a new Key Pair
    privKey_.MakeNewKey(false);
    pubKey_ = privKey_.GetPubKey();

    // Calculate obfuscation values
    obfuscation_jj1 = CBigNum::randBignum(gg_Params->coinCommitmentGroup.groupOrder);
    obfuscation_kk1 = CBigNum::randBignum(gg_Params->coinCommitmentGroup.groupOrder);
    obfuscation_jj2 = CBigNum::randBignum(gg_Params->coinCommitmentGroup.groupOrder);
    obfuscation_kk2 = CBigNum::randBignum(gg_Params->coinCommitmentGroup.groupOrder);
    blindingCommitment1_ = gg_Params->coinCommitmentGroup.g.pow_mod(obfuscation_jj1, gg_Params->coinCommitmentGroup.modulus).mul_mod(gg_Params->coinCommitmentGroup.h.pow_mod(obfuscation_kk1, gg_Params->coinCommitmentGroup.modulus), gg_Params->coinCommitmentGroup.modulus);
    blindingCommitment2_ = gg_Params->coinCommitmentGroup.g.pow_mod(obfuscation_jj2, gg_Params->coinCommitmentGroup.modulus).mul_mod(gg_Params->coinCommitmentGroup.h.pow_mod(obfuscation_kk2, gg_Params->coinCommitmentGroup.modulus), gg_Params->coinCommitmentGroup.modulus);

    obfuscation_jj = make_pair(obfuscation_jj1, obfuscation_jj2);
    obfuscation_kk = make_pair(obfuscation_kk1, obfuscation_kk2);
    blindingCommitment_ = make_pair(blindingCommitment1_, blindingCommitment2_);

    ggNumTests = ggSuccessfulTests = 0;
    for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
        ggCoins[i] = NULL;
    }

    // Run through all of the Zerocoin tests
    gLogTestResult("an RSA modulus can be generated", Testb_GenRSAModulus);
    gLogTestResult("parameter sizes are correct", Testb_CalcParamSizes);
    gLogTestResult("group/field parameters can be generated", Testb_GenerateGroupParams);
    gLogTestResult("parameter generation is correct", Testb_ParamGen);
    gLogTestResult("coins can be minted", Testb_MintCoin);
    gLogTestResult("the accumulator works", Testb_Accumulator);
    gLogTestResult("a minted coin can be spent", Testb_MintAndSpend);

    // Summarize test results
    if (ggSuccessfulTests < ggNumTests) {
        cout << endl << "ERROR: SOME TESTS FAILED" << endl;
    }

    // Clear any generated coins
    for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
        delete ggCoins[i];
    }

    cout << ggSuccessfulTests << " out of " << ggNumTests << " tests passed." << endl << endl;
    delete gg_Params;
}
BOOST_FIXTURE_TEST_SUITE(libzeroct_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(benchmark_test)
{
    cout << "libzeroct v" << ZEROCOIN_VERSION_STRING << " benchmark utility." << endl << endl;

    Testb_RunAllTests();
}
BOOST_AUTO_TEST_SUITE_END()

