#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundRawTXCreateProposalTest(NavCoinTestFramework):
    """Tests the state transition of proposals of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1


        self.goodDescription = "these are not the NAV Droids you are looking for"
        self.goodDuration = 360000
        self.goodAmount = 100
        self.goodPropHash = ""
        self.goodAddress = ""

    def setup_network(self, split=False):
        self.all_desc_text_options()

        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        self.nodes[0].staking(False)
        activate_cfund(self.nodes[0])

        # creates a good proposal and sets things we use later
        self.test_happy_path()

        # test incorrect amounts
        self.test_invalid_proposal(self.goodAddress, -100, self.goodDuration, "I should not work")
        self.test_invalid_proposal(self.goodAddress, -1, self.goodDuration, "I should not work")
        self.test_invalid_proposal(self.goodAddress, 0, self.goodDuration, "I should not work")
        self.test_invalid_proposal(self.goodAddress, "", self.goodDuration, "I should not work")


        # test incorrect duration
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, 0, "I should not work")
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, -13838, "I should not work")
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, True, "I should not work")
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, False, "I should not work")
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, "dsf", "I should not work")
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, "36000", "I should not work")
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, "", "I should not work")

        # test invalid address
        self.test_invalid_proposal("", self.goodAmount, self.goodDuration, "I should not work")
        self.test_invalid_proposal("a", self.goodAmount, self.goodDuration, "I should not work")
        self.test_invalid_proposal("1KFHE7w8BhaENAswwryaoccDb6qcT6DbYY", self.goodAmount, self.goodDuration, "I should not work") # bitcoin address
        self.test_invalid_proposal("NPyEJsv82GaguVsY3Ur4pu4WwnFCsYQ94g", self.goodAmount, self.goodDuration, "I should not work") # nav address we don't own
        self.test_invalid_proposal(False, self.goodAmount, self.goodDuration, "I should not work")
        self.test_invalid_proposal(True, self.goodAmount, self.goodDuration, "I should not work")
        self.test_invalid_proposal(8888, self.goodAmount, self.goodDuration, "I should not work")
        self.test_invalid_proposal(-8888, self.goodAmount, self.goodDuration, "I should not work")
        self.test_invalid_proposal(0, self.goodAmount, self.goodDuration, "I should not work")
        self.test_invalid_proposal(1, self.goodAmount, self.goodDuration, "I should not work")

        # test invalid descriptions
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, self.goodDuration, self.descTxtToLong)
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, self.goodDuration, 800)
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, self.goodDuration, True)
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, self.goodDuration, False)
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, self.goodDuration, -100)
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, self.goodDuration, 0)
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, self.goodDuration, 1)
        self.test_invalid_proposal(self.goodAddress, self.goodAmount, self.goodDuration, -1)

        self.test_valid_description(self.descTxtWhiteSpace, 2)
        self.test_valid_description(self.descTxtMaxLength, 3)
        self.test_valid_description(self.descTxtAllCharsAtoB, 4)
        self.test_valid_description(self.descTxtAllCharsCtoE, 5)
        self.test_valid_description(self.descTxtAllCharsCtoE, 6)
        self.test_valid_description(self.descTxtAllCharsFtoG, 7)
        self.test_valid_description(self.descTxtAllCharsHtoK, 8)
        self.test_valid_description(self.descTxtAllCharsLtoN, 9)
        self.test_valid_description(self.descTxtAllCharsOtoP, 10)
        self.test_valid_description(self.descTxtAllCharsQtoS, 11)
        self.test_valid_description(self.descTxtAllCharsTtoU, 12)
        self.test_valid_description(self.descTxtAllCharsVtoZ, 13)
        self.test_valid_description(self.descTxtAllCharsArrows1, 14)
        self.test_valid_description(self.descTxtAllCharsArrows2, 15)
        self.test_valid_description(self.descTxtAllCharsArrows3, 16)
        self.test_valid_description(self.descTxtAllCharsClassic1, 17)
        self.test_valid_description(self.descTxtAllCharsClassic2, 18)
        self.test_valid_description(self.descTxtAllCharsCurrency, 19)
        self.test_valid_description(self.descTxtAllCharsShapes1, 20)
        self.test_valid_description(self.descTxtAllCharsShapes2, 21)
        self.test_valid_description(self.descTxtAllCharsShapes3, 22)
        self.test_valid_description(self.descTxtAllCharsShapes4, 23)
        self.test_valid_description(self.descTxtAllCharsShapes4, 24)
        self.test_valid_description(self.descTxtAllCharsMath1, 25)
        self.test_valid_description(self.descTxtAllCharsMath2, 26)
        self.test_valid_description(self.descTxtAllCharsMath3, 27)
        self.test_valid_description(self.descTxtAllCharsMath4, 28)
        self.test_valid_description(self.descTxtAllCharsNumerals1, 29)
        self.test_valid_description(self.descTxtAllCharsNumerals2, 30)
        self.test_valid_description(self.descTxtAllCharsPunch1, 31)
        self.test_valid_description(self.descTxtAllCharsPunch2, 32)
        self.test_valid_description(self.descTxtAllCharsSymbol1, 33)
        self.test_valid_description(self.descTxtAllCharsSymbol2, 34)
        self.test_valid_description(self.descTxtAllCharsSymbol3, 35)

        # i = 4
        # for char in self.descTxtAllCharsAtoE:
        #     i = i + 1
        #     self.test_desc_should_succeed(char, i)

    def test_invalid_proposal(self, address, amount, duration, description):

        # Create new payment request for more than the amount
        proposal = ""
        callSucceed = False
        try:
            proposal = self.send_raw_proposalrequest(address, amount, duration, description)
            #print(proposal)
            callSucceed = True
        except :
            pass

        assert(proposal == "")
        assert(callSucceed is False)

        #check a gen - should still only have the last good prop
        blocks = slow_gen(self.nodes[0], 1)
        proposal_list = self.nodes[0].listproposals()

        #should still only have 1 proposal from the good test run
        assert(len(proposal_list) == 1)
        self.check_good_proposal(proposal_list[0])


    def test_valid_description(self, descriptionTxt, proposal_list_len):

        duration = 360000
        amount = 100

        # Create new payment request for more than the amount
        propHash = ""
        callSucceed = True

        #print("Test Description: -------------------------")
        #print(descriptionTxt)
        try:
            propHash = self.send_raw_proposalrequest(self.goodAddress, self.goodAmount, self.goodDuration, descriptionTxt)
            #print(propHash)
        except Exception as e:
            print(e)
            callSucceed = False

        assert(propHash != "")
        assert (callSucceed is True)

        # check a gen - should still only have the last good prop
        blocks = slow_gen(self.nodes[0], 1)
        proposal_list = self.nodes[0].listproposals()

        # should still only have the correct amount of proposals from the other runs
        assert(len(proposal_list) == proposal_list_len)

        # find the proposal we just made and test the description
        proposal_found = False
        for proposal in proposal_list:
            if proposal['hash'] == propHash:
                proposal_found = True
                assert(proposal['description'] == descriptionTxt)

        assert(proposal_found)



    # Test everything the way it should be
    def test_happy_path(self):

        self.goodAddress = self.nodes[0].getnewaddress()

        self.goodPropHash = self.send_raw_proposalrequest(self.goodAddress, self.goodAmount, self.goodDuration, self.goodDescription)

        blocks = slow_gen(self.nodes[0], 1)
        proposal_list = self.nodes[0].listproposals()

        # Should only have 1 proposal
        assert(len(proposal_list) == 1)

        # The proposal should have all the same required fields
        assert (proposal_list[0]['blockHash'] == blocks[0])
        self.check_good_proposal(proposal_list[0])

    def check_good_proposal(self, proposal):

        assert (proposal['votingCycle'] == 0)
        assert (proposal['version'] == 2)
        assert (proposal['paymentAddress'] == self.goodAddress)
        assert (proposal['proposalDuration'] == self.goodDuration)
        assert (proposal['description'] == self.goodDescription)
        assert (proposal['votesYes'] == 0)
        assert (proposal['votesNo'] == 0)
        assert (proposal['status'] == 'pending')
        assert (proposal['state'] == 0)
        assert (proposal['hash'] == self.goodPropHash)
        assert (float(proposal['requestedAmount']) == float(self.goodAmount))
        assert (float(proposal['notPaidYet']) == float(self.goodAmount))
        assert (float(proposal['userPaidFee']) == float(1))


    def send_raw_proposalrequest(self, address, amount, time, description):

        amount = amount * 100000000

        # Create a raw proposal tx
        raw_proposal_tx = self.nodes[0].createrawtransaction(
            [],
            {"6ac1": 1},
            json.dumps({"v": 2, "n": amount, "a": address,  "d": time, "s": description})
        )

        # Modify version
        raw_proposal_tx = "04" + raw_proposal_tx[2:]

        # Fund raw transaction
        raw_proposal_tx = self.nodes[0].fundrawtransaction(raw_proposal_tx)['hex']

        # Sign raw transaction
        raw_proposal_tx = self.nodes[0].signrawtransaction(raw_proposal_tx)['hex']

        # Send raw transaction
        return self.nodes[0].sendrawtransaction(raw_proposal_tx)


    def all_desc_text_options(self):
        self.descTxtToLong = "LOOOOONNNNNGGGG, Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque semper justo ac neque mollis, a cursus nisl placerat. Aliquam ipsum quam, congue vitae vulputate id, ullamcorper vel libero. Phasellus et tristique justo. Curabitur eu porta magna, vitae auctor libero. Fusce tellus ipsum, aliquet nec consequat ut, dictum eget libero. Maecenas eu velit quam. Nunc ac libero in purus vestibulum feugiat quis nec urna. Donec faucibus consequat dignissim. Donec ornare turpis nec lobortis vestibulum. Vivamus lobortis vel massa ac ultrices. Ut vel eros in elit vehicula luctus vel vitae justo. Praesent quis semper nisi. Vivamus viverra blandit ex. Sed nec fringilla quam. Nulla condimentum rhoncus erat sit amet vulputate. Phasellus viverra sagittis consequat. Sed dapibus augue ac enim dignissim, at consequat arcu ornare. Vestibulum facilisis pretium aliquet. asdfjasdlkfjhadsfkjhasdkjhakjdhfaskjdakjsdhf  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
        self.descTxtMaxLength ="IM LOOOOONNNNNGGGG, Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque semper justo ac neque mollis, a cursus nisl placerat. Aliquam ipsum quam, congue vitae vulputate id, ullamcorper vel libero. Phasellus et tristique justo. Curabitur eu porta magna, vitae auctor libero. Fusce tellus ipsum, aliquet nec consequat ut, dictum eget libero. Maecenas eu velit quam. Nunc ac libero in purus vestibulum feugiat quis nec urna. Donec faucibus consequat dignissim. Donec ornare turpis nec lobortis vestibulum. Vivamus lobortis vel massa ac ultrices. Ut vel eros in elit vehicula luctus vel vitae justo. Praesent quis semper nisi. Vivamus viverra blandit ex. Sed nec fringilla quam. Nulla condimentum rhoncus erat sit amet vulputate. Phasellus viverra sagittis consequat. Sed dapibus augue ac enim dignissim, at consequat arcu ornare. Vestibulum facilisis pretium aliquet. xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxabc"

        self.descTxtAllCharsAtoB = "Ⓐ ⓐ ⒜ A a Ạ ạ Å å Ä ä Ả ả Ḁ ḁ Ấ ấ Ầ ầ Ẩ ẩ Ȃ ȃ Ẫ ẫ Ậ ậ Ắ ắ Ằ ằ Ẳ ẳ Ẵ ẵ Ặ ặ Ā ā Ą ą Ȁ ȁ Ǻ ǻ Ȧ ȧ Á á Ǟ ǟ Ǎ ǎ À à Ã ã Ǡ ǡ Â â Ⱥ ⱥ Æ æ Ǣ ǣ Ǽ ǽ Ɐ Ꜳ ꜳ Ꜹ ꜹ Ꜻ ꜻ Ɑ ℀ ⅍ ℁ ª Ⓑ ⓑ ⒝ B b Ḃ ḃ Ḅ ḅ Ḇ ḇ Ɓ Ƀ ƀ Ƃ ƃ Ƅ ƅ ℬ"
        self.descTxtAllCharsCtoE = "Ⓒ ⓒ ⒞ C c Ḉ ḉ Ć ć Ĉ ĉ Ċ ċ Č č Ç ç Ƈ ƈ Ȼ ȼ ℂ ℃ Ɔ Ꜿ ꜿ ℭ ℅ ℆ ℄ Ⓓ ⓓ ⒟ D d Ḋ ḋ Ḍ ḍ Ḏ ḏ Ḑ ḑ Ḓ ḓ Ď ď Ɗ Ƌ ƌ Ɖ Đ đ ȡ Ǳ ǲ ǳ Ǆ ǅ ǆ ȸ ⅅ ⅆ Ⓔ ⓔ ⒠ E e Ḕ ḕ Ḗ ḗ Ḙ ḙ Ḛ ḛ Ḝ ḝ Ẹ ẹ Ẻ ẻ Ế ế Ẽ ẽ Ề ề Ể ể Ễ ễ Ệ ệ Ē ē Ĕ ĕ Ė ė Ę ę Ě ě È è É é Ê ê Ë ë Ȅ ȅ Ȩ ȩ Ȇ ȇ Ǝ ⱻ Ɇ ɇ Ə ǝ ℰ ⱸ ℯ ℮ ℇ Ɛ"
        self.descTxtAllCharsFtoG = "Ⓕ ⓕ ⒡ F f Ḟ ḟ Ƒ ƒ ꜰ Ⅎ ⅎ ꟻ ℱ ℻ Ⓖ ⓖ ⒢ G g Ɠ Ḡ ḡ Ĝ ĝ Ğ ğ Ġ ġ Ǥ ǥ Ǧ ǧ Ǵ ℊ ⅁ ǵ Ģ ģ"
        self.descTxtAllCharsHtoK = "Ⓗ ⓗ ⒣ H h Ḣ ḣ Ḥ ḥ Ḧ ḧ Ḩ ḩ Ḫ ḫ Ĥ ĥ Ȟ ȟ Ħ ħ Ⱨ ⱨ Ꜧ ℍ Ƕ ẖ ℏ ℎ ℋ ℌ ꜧ Ⓘ ⓘ ⒤ I i Ḭ ḭ Ḯ ḯ Ĳ ĳ Í í Ì ì Î î Ï ï Ĩ ĩ Ī ī Ĭ ĭ Į į Ǐ ǐ ı ƚ Ỻ ⅈ ⅉ ℹ ℑ ℐ Ⓙ ⓙ ⒥ J j Ĵ ĵ Ɉ ɉ ȷ ⱼ ǰ Ⓚ ⓚ ⒦ K k Ḱ ḱ Ḳ ḳ Ḵ ḵ Ķ ķ Ƙ ƙ Ꝁ ꝁ Ꝃ ꝃ Ꝅ ꝅ Ǩ ǩ Ⱪ ⱪ ĸ "
        self.descTxtAllCharsLtoN = "Ⓛ ⓛ ⒧ L l Ḷ ḷ Ḹ ḹ Ḻ ḻ Ḽ ḽ Ĺ ĺ Ļ ļ Ľ ľ Ŀ ŀ Ł ł Ỉ ỉ Ⱡ ⱡ Ƚ ꝉ Ꝉ Ɫ Ǉ ǈ ǉ Ị İ ị ꞁ ⅃ ⅂ Ȉ ȉ Ȋ ȋ ℓ ℒ Ⓜ ⓜ ⒨ M m Ḿ ḿ Ṁ ṁ Ṃ ṃ ꟿ ꟽ Ɱ Ɯ ℳ Ⓝ ⓝ ⒩ N n Ṅ ṅ Ṇ ṇ Ṉ ṉ Ṋ ṋ Ń ń Ņ ņ Ň ň Ǹ ǹ Ñ ñ Ƞ ƞ Ŋ ŋ Ɲ ŉ Ǌ ǋ ǌ ȵ ℕ №"
        self.descTxtAllCharsOtoP = "Ⓞ ⓞ ⒪ O o Ö ö Ṏ ṏ Ṍ ṍ Ṑ ṑ Ṓ ṓ Ȫ ȫ Ȭ ȭ Ȯ ȯ Ȱ ȱ Ǫ ǫ Ǭ ǭ Ọ ọ Ỏ ỏ Ố ố Ồ ồ Ổ ổ Ỗ ỗ Ộ ộ Ớ ớ Ờ ờ Ở ở Ỡ ỡ Ợ ợ Ơ ơ Ō ō Ŏ ŏ Ő ő Ò ò Ó ó Ô ô Õ õ Ǒ ǒ Ȍ ȍ Ȏ ȏ Œ œ Ø ø Ǿ ǿ Ꝋ Ꝏ ꝏ ⍥ ⍤ ℴ Ⓟ ⓟ ⒫ ℗ P p Ṕ ṕ Ṗ ṗ Ƥ ƥ Ᵽ ℙ Ƿ "
        self.descTxtAllCharsQtoS = "ꟼ ℘ Ⓠ ⓠ ⒬ Q q Ɋ ɋ ℚ ℺ ȹ Ⓡ ⓡ ⒭ R r Ŕ ŕ Ŗ ŗ Ř ř Ṙ ṙ Ṛ ṛ Ṝ ṝ Ṟ ṟ Ȑ ȑ Ȓ ȓ Ɍ ɍ Ʀ Ꝛ ꝛ Ɽ ℞ ℜ ℛ ℟ ℝ Ⓢ ⓢ ⒮ S s Ṡ ṡ Ṣ ṣ Ṥ ṥ Ṧ ṧ Ṩ ṩ Ś ś Ŝ ŝ Ş ş Š š Ș ș ȿ ꜱ Ƨ ƨ Ϩ ϩ ẞ ß ẛ ẜ ẝ ℠"
        self.descTxtAllCharsTtoU = "Ⓣ ⓣ ⒯ T t Ṫ ṫ Ṭ ṭ Ṯ ṯ Ṱ ṱ Ţ ţ Ť ť Ŧ ŧ Ț ț Ⱦ ⱦ Ƭ Ʈ ƫ ƭ ẗ ȶ ℡ ™ Ⓤ ⓤ ⒰ U u Ṳ ṳ Ṵ ṵ Ṷ ṷ Ṹ ṹ Ṻ ṻ Ủ ủ Ụ ụ Ứ ứ Ừ ừ Ử ử Ữ ữ Ự ự Ũ ũ Ū ū Ŭ ŭ Ů ů Ű ű Ǚ ǚ Ǘ ǘ Ǜ ǜ Ų ų Ǔ ǔ Ȕ ȕ Û û Ȗ ȗ Ù ù Ú ú Ü ü Ư ư Ʉ Ʋ Ʊ"
        self.descTxtAllCharsVtoZ = "Ⓥ ⓥ ⒱ V v Ṽ ṽ Ṿ ṿ Ʌ ℣ Ỽ ⱱ ⱴ ⱽ Ⓦ ⓦ ⒲ W w Ẁ ẁ Ẃ ẃ Ẅ ẅ Ẇ ẇ Ẉ ẉ Ŵ ŵ Ⱳ ⱳ Ϣ ϣ ẘ Ⓧ ⓧ ⒳ X x Ẋ ẋ Ẍ ẍ ℵ × Ⓨ ⓨ ⒴ y Y Ẏ ẏ Ỿ ỿ Ỳ ỳ Ỵ ỵ Ỷ ỷ Ỹ ỹ Ŷ ŷ Ƴ ƴ Ÿ ÿ Ý ý Ɏ ɏ Ȳ ȳ Ɣ ẙ ⅄ ℽ Ⓩ ⓩ ⒵ Z z Ẑ ẑ Ẓ ẓ Ẕ ẕ Ź ź Ż ż Ž ž Ȥ ȥ Ⱬ ⱬ Ƶ ƶ ɀ ℨ ℤ"
        self.descTxtAllCharsArrows1 = "↪ ↩ ← ↑ → ↓ ↔ ↕ ↖ ↗ ↘ ↙ ↚ ↛ ↜ ↝ ↞ ↟ ↠ ↡ ↢ ↣ ↤ ↦ ↥ ↧ ↨ ↫ ↬ ↭ ↮ ↯ ↰ ↱ ↲ ↴ ↳ ↵ ↶ ↷ ↸ ↹ ↺ ↻ ⟲ ⟳ ↼ ↽ ↾ ↿ ⇀ ⇁ ⇂ ⇃ ⇄ ⇅ ⇆ ⇇ ⇈ ⇉ ⇊ ⇋ ⇌ ⇍ ⇏ ⇎ ⇑ ⇓ ⇐ ⇒ ⇔ ⇕ ⇖ ⇗ ⇘ ⇙ ⇳ ⇚ ⇛ ⇜ ⇝ ⇞ ⇟ ⇠ ⇡ ⇢ ⇣ ⇤ ⇥ ⇦ ⇨ ⇩ ⇪ ⇧ ⇫ ⇬ ⇭ ⇮ ⇯ ⇰ ⇱ ⇲ ⇴ ⇵ ⇶ ⇷ ⇸ ⇹ ⇺ ⇻ ⇼ ⇽ ⇾ ⇿ ⟰ ⟱ ⟴ ⟵ ⟶ ⟷ ⟸ ⟹ ⟽ ⟾ ⟺ ⟻ ⟼ ⟿"
        self.descTxtAllCharsArrows2 = "⤀ ⤁ ⤅ ⤂ ⤃ ⤄ ⤆ ⤇ ⤈ ⤉ ⤊ ⤋ ⤌ ⤍ ⤎ ⤏ ⤐ ⤑ ⤒ ⤓ ⤔ ⤕ ⤖ ⤗ ⤘ ⤙ ⤚ ⤛ ⤜ ⤝ ⤞ ⤟ ⤠ ⤡ ⤢ ⤣ ⤤ ⤥ ⤦ ⤧ ⤨ ⤩ ⤪ ⤭ ⤮ ⤯ ⤰ ⤱ ⤲ ⤳ ⤻ ⤸ ⤾ ⤿ ⤺ ⤼ ⤽ ⤴ ⤵ ⤶ ⤷ ⤹ ⥀ ⥁ ⥂ ⥃ ⥄ ⥅ ⥆ ⥇ ⥈ ⥉ ⥒ ⥓ ⥔ ⥕ ⥖ ⥗ ⥘ ⥙ ⥚ ⥛ ⥜"
        self.descTxtAllCharsArrows3 = "⥝ ⥞ ⥟ ⥠ ⥡ ⥢ ⥣ ⥤ ⥥ ⥦ ⥧ ⥨ ⥩ ⥪ ⥫ ⥬ ⥭ ⥮ ⥯ ⥰ ⥱ ⥲ ⥳ ⥴ ⥵ ⥶ ⥷ ⥸ ⥹ ⥺ ⥻ ➔ ➘ ➙ ➚ ➛ ➜ ➝ ➞ ➟ ➠ ➡ ➢ ➣ ➤ ➥ ➦ ➧ ➨ ➩ ➪ ➫ ➬ ➭ ➮ ➯ ➱ ➲ ➳ ➴ ➵ ➶ ➷ ➸ ➹ ➺ ➻ ➼ ➽ ➾ ⬀ ⬁ ⬂ ⬃ ⬄ ⬅ ⬆ ⬇ ⬈ ⬉ ⬊ ⬋ ⬌ ⬍ ⏎ ▲ ▼ ◀ ▶ ⬎ ⬏ ⬐ ⬑ ☇ ☈ ⍃ ⍄ ⍇ ⍈ ⍐ ⍗ ⍌ ⍓ ⍍ ⍔ ⍏ ⍖ ⍅ ⍆"
        self.descTxtAllCharsClassic1 = "⌘ « » ‹ › ‘ ’ “ ” „ ‚ ❝ ❞ £ ¥ € $ ¢ ¬ ¶ @ § ® © ™ ° × π ± √ ‰ Ω ∞ ≈ ÷ ~ ≠ ¹ ² ³ ½ ¼ ¾ ‐ – — | ⁄ \ [ ] { } † ‡ … · • ●  ⌥ ⌃ ⇧ ↩ ¡ ¿ ‽ ⁂ ∴ ∵ ◊ ※ ← → ↑ ↓ ☜ ☞ ☝ ☟ ✔ ★ ☆ ♺ ☼ ☂ ☺ ☹ ☃ ✉ ✿"
        self.descTxtAllCharsClassic2 = "✄ ✈ ✌ ✎ ♠ ♦ ♣ ♥ ♪ ♫ ♯ ♀ ♂ α ß Á á À à Å å Ä ä Æ æ Ç ç É é È è Ê ê Í í Ì ì Î î Ñ ñ Ó ó Ò ò Ô ô Ö ö Ø ø Ú ú Ù ù Ü ü Ž ž"
        self.descTxtAllCharsCurrency = "₳ ฿ ￠ ₡ ¢ ₢ ₵ ₫ € ￡ £ ₤ ₣ ƒ ₲ ₭ ₥ ₦ ₱ ＄ $ ₮ ₩ ￦ ¥ ￥ ₴ ¤ ₰ ៛ ₪ ₯ ₠ ₧ ₨ ௹ ﷼ ㍐ ৲ ৳ ₹"
        self.descTxtAllCharsShapes1 = "▲ ▼ ◀ ▶ ◢ ◣ ◥ ◤ △ ▽ ◿ ◺ ◹ ◸ ▴ ▾ ◂ ▸ ▵ ▿ ◃ ▹ ◁ ▷ ◅ ▻ ◬ ⟁ ⧋ ⧊ ⊿ ∆ ∇ ◭ ◮ ⧩ ⧨ ⌔ ⟐ ◇ ◆ ◈ ⬖ ⬗ ⬘ ⬙ ⬠ ⬡ ⎔ ⋄ ◊ ⧫ ⬢ ⬣ ▰ ▪ ◼ ▮ ◾ ▗ ▖ ■ ∎ ▃ ▄ ▅ ▆ ▇ █ ▌ ▐ ▍ ▎ ▉ ▊ ▋ ❘ ❙ ❚ ▀ ▘ ▝ ▙ ▚ ▛ ▜ ▟ ▞ ░ ▒ ▓ ▂ ▁ ▬ ▔ ▫ ▯ ▭ ▱ ◽ □ ◻ ▢ ⊞ ⊡ ⊟ ⊠ ▣ "
        self.descTxtAllCharsShapes2 = "▤ ▥ ▦ ⬚ ▧ ▨ ▩ ⬓ ◧ ⬒ ◨ ◩ ◪ ⬔ ⬕ ❏ ❐ ❑ ❒ ⧈ ◰ ◱ ◳ ◲ ◫ ⧇ ⧅ ⧄ ⍁ ⍂ ⟡ ⧉ ○ ◌ ◍ ◎ ◯ ❍ ◉ ⦾ ⊙ ⦿ ⊜ ⊖ ⊘ ⊚ ⊛ ⊝ ● ⚫ ⦁ ◐ ◑ ◒ ◓ ◔ ◕ ⦶ ⦸ ◵ ◴ ◶ ◷ ⊕ ⊗ ⦇ ⦈ ⦉ ⦊ ❨ ❩ ⸨ ⸩ ◖ ◗ ❪ ❫ ❮ ❯ ❬ ❭ ❰ ❱ ⊏ ⊐ ⊑ ⊒ ◘ ◙ ◚ ◛"
        self.descTxtAllCharsShapes3 = "◜ ◝ ◞ ◟ ◠ ◡ ⋒ ⋓ ⋐ ⋑ ⥰ ╰ ╮ ╭ ╯ ⌒ ⥿ ⥾ ⥽ ⥼ ⥊ ⥋ ⥌ ⥍ ⥎ ⥐ ⥑ ⥏ ╳ ✕ ⤫ ⤬ ╱ ╲ ⧸ ⧹ ⌓ ◦ ❖ ✖ ✚ ✜ ⧓ ⧗ ⧑ ⧒ ⧖ _ ⚊ ╴ ╼ ╾ ‐ ⁃ ‑ ‒ - – ⎯ — ― ╶ ╺ ╸ ─ ━ ┄ ┅ ┈ ┉ ╌ ╍ ═ ≣ ≡ ☰ ☱ ☲ ☳ ☴ ☵ ☶ ☷ ╵ ╷ ╹ ╻ │ ▕ ▏ ┃ ┆ ┇ ┊ ╎ ┋ ╿ ╽ ⌞ ⌟ ⌜ ⌝ ⌊ ⌋ ⌈ ⌉ ⌋ ┌ ┍ ┎ ┏ ┐ ┑ ┒ ┓"
        self.descTxtAllCharsShapes4 = "└ ┕ ┖ ┗ ┘ ┙ ┚ ┛ ├ ┝ ┞ ┟ ┠ ┡ ┢ ┣ ┤ ┥ ┦ ┧ ┨ ┩ ┪ ┫ ┬ ┭ ┮ ┳ ┴ ┵ ┶ ┷ ┸ ┹ ┺ ┻ ┼ ┽ ┾ ┿ ╀ ╁ ╂ ╃ ╄ ╅ ╆ ╇ ╈ ╉ ╊ ╋ ╏ ║ ╔ ╒ ╓ ╕ ╖ ╗ ╚ ╘ ╙ ╛ ╜ ╝ ╞ ╟ ╠ ╡ ╢ ╣ ╤ ╥ ╦ ╧ ╨ ╩ ╪ ╫ ╬"
        self.descTxtAllCharsMath1 = "∞ ⟀ ⟁ ⟂ ⟃ ⟄ ⟇ ⟈ ⟉ ⟊ ⟐ ⟑ ⟒ ⟓ ⟔ ⟕ ⟖ ⟗ ⟘ ⟙ ⟚ ⟛ ⟜ ⟝ ⟞ ⟟ ⟠ ⟡ ⟢ ⟣ ⟤ ⟥ ⟦ ⟧ ⟨ ⟩ ⟪ ⟫ ⦀ ⦁ ⦂ ⦃ ⦄ ⦅ ⦆ ⦇ ⦈ ⦉ ⦊ ⦋ ⦌ ⦍ ⦎ ⦏ ⦐ ⦑ ⦒ ⦓ ⦔ ⦕ ⦖ ⦗ ⦘ ⦙ ⦚ ⦛ ⦜ ⦝ ⦞ ⦟ ⦠ ⦡ ⦢ ⦣ ⦤ ⦥ ⦦ ⦧ ⦨ ⦩ ⦪ ⦫ ⦬ ⦭ ⦮ ⦯ ⦰ ⦱ ⦲ ⦳ ⦴ ⦵ ⦶ ⦷ ⦸ ⦹ ⦺ ⦻ ⦼ ⦽ ⦾ ⦿ ⧀ ⧁ ⧂ ⧃ ⧄ ⧅ ⧆ ⧇ ⧈ ⧉ ⧊ ⧋ ⧌ ⧍ ⧎ ⧏ ⧐ ⧑ ⧒ ⧓ ⧔ ⧕ ⧖ ⧗"
        self.descTxtAllCharsMath2 = "⧘ ⧙ ⧚ ⧛ ⧜ ⧝ ⧞ ⧟ ⧡ ⧢ ⧣ ⧤ ⧥ ⧦ ⧧ ⧨ ⧩ ⧪ ⧫ ⧬ ⧭ ⧮ ⧯ ⧰ ⧱ ⧲ ⧳ ⧴ ⧵ ⧶ ⧷ ⧸ ⧹ ⧺ ⧻ ⧼ ⧽ ⧾ ⧿ ∀ ∁ ∂ ∃ ∄ ∅ ∆ ∇ ∈ ∉ ∊ ∋ ∌ ∍ ∎ ∏ ∐ ∑ − ∓ ∔ ∕ ∖ ∗ ∘ ∙ √ ∛ ∜ ∝ ∟ ∠ ∡ ∢ ∣ ∤ ∥ ∦ ∧ ∨ ∩ ∪ ∫ ∬ ∭ ∮ ∯ ∰ ∱ ∲ ∳ ∴ ∵ ∶ ∷ "
        self.descTxtAllCharsMath3 = "∸ ∹ ∺ ∻ ∼ ∽ ∾ ∿ ≀ ≁ ≂ ≃ ≄ ≅ ≆ ≇ ≈ ≉ ≊ ≋ ≌ ≍ ≎ ≏ ≐ ≑ ≒ ≓ ≔ ≕ ≖ ≗ ≘ ≙ ≚ ≛ ≜ ≝ ≞ ≟ ≠ ≡ ≢ ≣ ≤ ≥ ≦ ≧ ≨ ≩ ≪ ≫ ≬ ≭ ≮ ≯ ≰ ≱ ≲ ≳ ≴ ≵ ≶ ≷ ≸ ≹ ≺ ≻ ≼ ≽ ≾ ≿ ⊀ ⊁ ⊂ ⊃ ⊄ ⊅ ⊆ ⊇ ⊈ ⊉ ⊊ ⊋ ⊌ ⊍ ⊎ ⊏ ⊐ ⊑ ⊒ ⊓ ⊔ ⊕ ⊖ ⊗ ⊘ ⊙ ⊚ ⊛ ⊜ ⊝ ⊞ ⊟ ⊠ ⊡ ⊢ ⊣ ⊤ ⊥ ⊦ ⊧ ⊨ ⊩ ⊪ ⊫ ⊬ ⊭ ⊮ ⊯ ⊰ ⊱ ⊲ ⊳"
        self.descTxtAllCharsMath4 = "⊴ ⊵ ⊶ ⊷ ⊸ ⊹ ⊺ ⊻ ⊼ ⊽ ⊾ ⊿ ⋀ ⋁ ⋂ ⋃ ⋄ ⋅ ⋆ ⋇ ⋈ ⋉ ⋊ ⋋ ⋌ ⋍ ⋎ ⋏ ⋐ ⋑ ⋒ ⋓ ⋔ ⋕ ⋖ ⋗ ⋘ ⋙ ⋚ ⋛ ⋜ ⋝ ⋞ ⋟ ⋠ ⋡ ⋢ ⋣ ⋤ ⋥ ⋦ ⋧ ⋨ ⋩ ⋪ ⋫ ⋬ ⋭ ⋮ ⋯ ⋰ ⋱ ⋲ ⋳ ⋴ ⋵ ⋶ ⋷ ⋸ ⋹ ⋺ ⋻ ⋼ ⋽ ⋾ ⋿ ✕ ✖ ✚"
        self.descTxtAllCharsNumerals1 = "⓵ ⓶ ⓷ ⓸ ⓹ ⓺ ⓻ ⓼ ⓽ ⓾ ⒈ ⒉ ⒊ ⒋ ⒌ ⒍ ⒎ ⒏ ⒐ ⒑ ⒒ ⒓ ⒔ ⒕ ⒖ ⒗ ⒘ ⒙ ⒚ ⒛ ⓪ ① ② ③ ④ ⑤ ⑥ ⑦ ⑧ ⑨ ⑩ ➀ ➁ ➂ ➃ ➄ ➅ ➆ ➇ ➈ ➉ ⑪ ⑫ ⑬ ⑭ ⑮ ⑯ ⑰ ⑱ ⑲ ⑳"
        self.descTxtAllCharsNumerals2 = "⓿ ❶ ❷ ❸ ❹ ❺ ❻ ❼ ❽ ❾ ❿ ➊ ➋ ➌ ➍ ➎ ➏ ➐ ➑ ➒ ➓ ⓫ ⓬ ⓭ ⓮ ⓯ ⓰ ⓱ ⓲ ⓳ ⓴ ⑴ ⑵ ⑶ ⑷ ⑸ ⑹ ⑺ ⑻ ⑼ ⑽ ⑾ ⑿ ⒀ ⒁ ⒂ ⒃ ⒄ ⒅ ⒆ ⒇ ¹ ² ³ ↉ ½ ⅓ ¼ ⅕ ⅙ ⅐ ⅛ ⅑ ⅒ ⅔ ⅖ ¾ ⅗ ⅜ ⅘ ⅚ ⅝ ⅞"
        self.descTxtAllCharsPunch1 = "❝ ❞ ❛ ❜ ‘ ’ ‛ ‚ “ ” „ ‟ « » ‹ › Ꞌ  < > @ × ‧ ¨ ․ ꞉ : ⁚ ⁝ ⁞ ‥ … ⁖ ⸪ ⸬ ⸫ ⸭ ⁛ ⁘ ⁙ ⁏ ; ⦂ ⁃ ‐ ‑ ‒ - – ⎯ — ― _ ~ ⁓ ⸛ ⸞ ⸟ ⸯ ¬ / \ ⁄ \ ⁄ | ⎜ ¦ ‖ ‗ "
        self.descTxtAllCharsPunch2 = "† ‡ · • ⸰ ° ‣ ⁒ % ‰ ‱ & ⅋ § ÷ + ± = ꞊ ′ ″ ‴ ⁗ ‵ ‶ ‷ ‸ * ⁑ ⁎ ⁕ ※ ⁜ ⁂ ! ‼ ¡ ? ¿ ⸮ ⁇ ⁉ ⁈ ‽ ⸘ ¼ ½ ¾ ² ³ © ® ™ ℠ ℻ ℅ ℁ ⅍ ℄ ¶ ⁋ ❡ ⁌ ⁍ ⸖ ⸗ ⸚ ⸓ ( ) [ ] { } ⸨ ⸩ ❨ ❩ ❪ ❫ ⸦ ⸧ ❬ ❭ ❮ ❯ ❰ ❱ ❴ ❵ ❲ ❳ ⦗ ⦘ ⁅ ⁆ 〈 〉 ⏜ ⏝ ⏞ ⏟ ⸡ ⸠ ⸢ ⸣ ⸤ ⸥ ⎡ ⎤ ⎣ ⎦ ⎨ ⎬ ⌠ ⌡ ⎛ ⎠ ⎝ ⎞ ⁀ ⁔ ‿ ⁐ ‾ ⎟ ⎢ ⎥ ⎪ ꞁ ⎮ ⎧ ⎫ ⎩ ⎭ ⎰ ⎱ '"
        self.descTxtAllCharsSymbol1 = "☂ ☔ ✈ ☀ ☼ ☁ ⚡ ⌁ ☇ ☈ ❄ ❅ ❆ ☃ ☉ ☄ ★ ☆ ☽ ☾ ⌛ ⌚ ⌂ ✆ ☎ ☏ ✉ ☑ ✓ ✔ ⎷ ⍻ ✖ ✗ ✘ ☒ ✕ ☓ ☕ ♿ ✌ ☚ ☛ ☜ ☝ ☞ ☟ ☹ ☺ ☻ ☯ ⚘ ☮ ⚰ ⚱ ⚠ ☠ ☢ ⚔ ⚓ ⎈ ⚒ ⚑ ⚐ ☡ ❂ ⚕ ⚖ ⚗ ✇ ☣ ⚙ ☤ ⚚ ⚛ ⚜ ☥ ✝ ☦ ☧ ☨ ☩ † ☪ ☫ ☬ ☭ ✁ ✂ ✃ ✄ ✍"
        self.descTxtAllCharsSymbol2 = "✎ ✏ ✐  ✑ ✒ ✙ ✚ ✜ ✛ ♰ ♱ ✞ ✟ ✠ ✡ ☸ ✢ ✣ ✤ ✥ ✦ ✧ ✩ ✪ ✫ ✬ ✭ ✮ ✯ ✰ ✲ ✱ ✳ ✴ ✵ ✶ ✷ ✸ ✹ ✺ ✻ ✼ ✽ ✾ ❀ ✿ ❁ ❃ ❇ ❈ ❉ ❊ ❋ ⁕ ☘ ❦ ❧ ☙ ❢ ❣ ♀ ♂ ⚢ ⚣ ⚤ ⚦ ⚧ ⚨ ⚩ ☿ ♁ ⚯ ♛ ♕ ♚ ♔ ♜ ♖ ♝ ♗ ♞ ♘ ♟ ♙ ☗ ☖ ♠ ♣ ♦ ♥ ❤ ❥ ♡ ♢ ♤ ♧ ⚀ ⚁ ⚂ ⚃ ⚄ ⚅ ⚇ ⚆ ⚈ ⚉ ♨"
        self.descTxtAllCharsSymbol3 = "♩ ♪ ♫ ♬ ♭ ♮ ♯  ⌨ ⏏ ⎗ ⎘ ⎙ ⎚ ⌥ ⎇ ⌘ ⌦ ⌫ ⌧ ♲ ♳ ♴ ♵ ♶ ♷ ♸ ♹ ♺ ♻ ♼ ♽ ⁌ ⁍ ⎌ ⌇ ⌲ ⍝ ⍟ ⍣ ⍤ ⍥ ⍨ ⍩ ⎋ ♃ ♄ ♅ ♆ ♇ ♈ ♉ ♊ ♋ ♌ ♍ ♎ ♏ ♐ ♑ ♒ ♓ ⏚ ⏛"


        self.descTxtWhiteSpace = '''I 
Have
Enters



And      white     space '''

if __name__ == '__main__':
    CommunityFundRawTXCreateProposalTest().main()
