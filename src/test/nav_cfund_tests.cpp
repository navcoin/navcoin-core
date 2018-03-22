
#include "amount.h"
#include "test/test_navcoin.h"

#include <boost/test/unit_test.hpp>


int add(int i,  int j) {return i+j; }

BOOST_FIXTURE_TEST_SUITE(sdfgsdfg_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(dddd)
{

    BOOST_CHECK(add(2,2) == 5);

}


BOOST_AUTO_TEST_SUITE_END()


