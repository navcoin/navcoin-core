// Copyright (c) 2014-2017, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "utils/dns_utils.h"
#include "util.h"

#include <boost/filesystem/fstream.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <deque>
#include <stdlib.h>
#include <random>
#include <sstream>
#ifdef HAVE_UNBOUND
#include <unbound.h>
#else
#include <curl/curl.h>
#include <univalue.h>
#endif

namespace bf = boost::filesystem;

static boost::mutex instance_lock;

size_t write_data(
        const char* in,
        std::size_t size,
        std::size_t num,
        std::string* out)
{
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
}

namespace
{

/*
 * The following function was taken from unbound-anchor.c, from
 * the unbound library.
 */

#ifdef HAVE_UNBOUND
/** return the built in root DS trust anchor */
static const char*
get_builtin_ds(void)
{
    return
            ". IN DS 19036 8 2 49AAC11D7B6F6446702E54A1607371607A1A41855200FD2CE1CDDE32F24E8FB5\n";
}
#endif

/************************************************************
 ************************************************************
 ***********************************************************/

} // anonymous namespace

namespace utils
{

// fuck it, I'm tired of dealing with getnameinfo()/inet_ntop/etc
std::string ipv4_to_string(const char* src, size_t len)
{
    assert(len >= 4);

    std::stringstream ss;
    unsigned int bytes[4];
    for (int i = 0; i < 4; i++)
    {
        unsigned char a = src[i];
        bytes[i] = a;
    }
    ss << bytes[0] << "."
                   << bytes[1] << "."
                   << bytes[2] << "."
                   << bytes[3];
    return ss.str();
}

// this obviously will need to change, but is here to reflect the above
// stop-gap measure and to make the tests pass at least...
std::string ipv6_to_string(const char* src, size_t len)
{
    assert(len >= 8);

    std::stringstream ss;
    unsigned int bytes[8];
    for (int i = 0; i < 8; i++)
    {
        unsigned char a = src[i];
        bytes[i] = a;
    }
    ss << bytes[0] << ":"
                   << bytes[1] << ":"
                   << bytes[2] << ":"
                   << bytes[3] << ":"
                   << bytes[4] << ":"
                   << bytes[5] << ":"
                   << bytes[6] << ":"
                   << bytes[7];
    return ss.str();
}

std::string txt_to_string(const char* src, size_t len)
{
    return std::string(src+1, len-1);
}

// custom smart pointer.
// TODO: see if std::auto_ptr and the like support custom destructors
template<typename type, void (*freefunc)(type*)>
class scoped_ptr
{
public:
    scoped_ptr():
        ptr(nullptr)
    {
    }
    scoped_ptr(type *p):
        ptr(p)
    {
    }
    ~scoped_ptr()
    {
        freefunc(ptr);
    }
    operator type *() { return ptr; }
    type **operator &() { return &ptr; }
    type *operator->() { return ptr; }
    operator const type*() const { return &ptr; }

private:
    type* ptr;
};

#ifdef HAVE_UNBOUND
typedef class scoped_ptr<ub_result,ub_resolve_free> ub_result_ptr;
#endif

struct DNSResolverData
{
#ifdef HAVE_UNBOUND
    ub_ctx* m_ub_context;
#else
    char dummy;
#endif
};


// work around for bug https://www.nlnetlabs.nl/bugs-script/show_bug.cgi?id=515 needed for it to compile on e.g. Debian 7
class string_copy {
public:
    string_copy(const char *s): str(strdup(s)) {}
    ~string_copy() { free(str); }
    operator char*() { return str; }

public:
    char *str;
};

DNSResolver::DNSResolver() : m_data(new DNSResolverData())
{
#ifdef HAVE_UNBOUND
    int use_dns_public = 0;
    std::string dns_public_addr = GetArg("-dnsserver","8.8.4.4");
    if (auto res = getenv("DNS_PUBLIC"))
    {
        dns_public_addr = utils::dns_utils::parse_dns_public(res);
        if (!dns_public_addr.empty())
        {
            use_dns_public = 1;
        }
        else
        {
            LogPrintf("[DNSResolver] Failed to parse DNS_PUBLIC");
        }
    }

    // init libunbound context
    m_data->m_ub_context = ub_ctx_create();

    if (use_dns_public)
    {
        ub_ctx_set_fwd(m_data->m_ub_context, dns_public_addr.c_str());
        ub_ctx_set_option(m_data->m_ub_context, string_copy("do-udp:"), string_copy("no"));
        ub_ctx_set_option(m_data->m_ub_context, string_copy("do-tcp:"), string_copy("yes"));
    } else {
        // look for "/etc/resolv.conf" and "/etc/hosts" or platform equivalent
        ub_ctx_resolvconf(m_data->m_ub_context, NULL);
        ub_ctx_hosts(m_data->m_ub_context, NULL);
    }

    ub_ctx_add_ta(m_data->m_ub_context, string_copy(::get_builtin_ds()));
#endif
}

DNSResolver::~DNSResolver()
{
#ifdef HAVE_UNBOUND
    if (m_data)
    {
        if (m_data->m_ub_context != NULL)
        {
            ub_ctx_delete(m_data->m_ub_context);
        }
        delete m_data;
    }
#endif
}

std::vector<std::string> DNSResolver::get_record(const std::string& url, int record_type, std::string (*reader)(const char *,size_t), bool& dnssec_available, bool& dnssec_valid)
{
    std::vector<std::string> addresses;
    dnssec_available = false;
    dnssec_valid = false;

    if (!check_address_syntax(url.c_str()))
    {
        return addresses;
    }

#ifdef HAVE_UNBOUND
    // destructor takes care of cleanup
    ub_result_ptr result;

    // call DNS resolver, blocking.  if return value not zero, something went wrong
    if (!ub_resolve(m_data->m_ub_context, string_copy(url.c_str()), record_type, DNS_CLASS_IN, &result))
    {
        dnssec_available = (result->secure || (!result->secure && result->bogus));
        dnssec_valid = result->secure && !result->bogus;
        if (result->havedata)
        {
            for (size_t i=0; result->data[i] != NULL; i++)
            {
                addresses.push_back((*reader)(result->data[i], result->len[i]));
            }
        }
    }
#else
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (url.length() > 253)
        return addresses;

    if (curl) {
        std::string serverURL = "https://dns.google.com/resolve?name="+url+"&type=16&cd=0&edns_client_subnet=0.0.0.0/0&random_padding=" + random_string(253 - url.length());
        int httpCode(0);
        std::string readBuffer = "";

        curl_easy_setopt(curl, CURLOPT_URL, serverURL.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);

        if (httpCode == 200) {
            UniValue request;
            try {
                UniValue valRequest;
                if (!valRequest.read(readBuffer))
                    return addresses;
                if (valRequest.isObject()) {
                    request = valRequest.get_obj();
                } else {
                    return addresses;
                }
            } catch (const UniValue& objError) {
                return addresses;
            } catch (const std::exception& e) {
                return addresses;
            }
            UniValue status = find_value(request, "Status");
            UniValue data_obj = find_value(request, "Answer");
            UniValue dns_sec = find_value(request, "AD");

            if (!data_obj.isArray()) {
                return addresses;
            } else {
                if (status.get_int() != 0)
                    return addresses;
                dnssec_available = dnssec_valid = dns_sec.getBool();
                UniValue results(UniValue::VARR);
                results = data_obj.get_array();
                for(unsigned int i = 0; i < results.size(); i++)
                {
                    UniValue address = find_value(results[i], "data");
                    if(!address.isStr())
                        continue;
                    addresses.push_back(address.get_str().substr(1, address.get_str().length()-1));
                }
            }
        }
    }
#endif

    return addresses;
}

    std::vector<std::string> DNSResolver::get_ipv4(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
    {
        return get_record(url, DNS_TYPE_A, ipv4_to_string, dnssec_available, dnssec_valid);
    }

    std::vector<std::string> DNSResolver::get_ipv6(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
    {
        return get_record(url, DNS_TYPE_AAAA, ipv6_to_string, dnssec_available, dnssec_valid);
    }

    std::vector<std::string> DNSResolver::get_txt_record(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
    {
        return get_record(url, DNS_TYPE_TXT, txt_to_string, dnssec_available, dnssec_valid);
    }

    std::string DNSResolver::get_dns_format_from_oa_address(const std::string& oa_addr)
    {
        std::string addr(oa_addr);
        auto first_at = addr.find("@");
        if (first_at == std::string::npos)
            return addr;

        // convert name@domain.tld to name.domain.tld
        addr.replace(first_at, 1, ".");

        return addr;
    }

    DNSResolver& DNSResolver::instance()
    {
        boost::lock_guard<boost::mutex> lock(instance_lock);

        static DNSResolver staticInstance;
        return staticInstance;
    }

    DNSResolver DNSResolver::create()
    {
        return DNSResolver();
    }

    bool DNSResolver::check_address_syntax(const char *addr) const
    {
        // if string doesn't contain a dot, we won't consider it a url for now.
        if (strchr(addr,'.') == NULL)
        {
            return false;
        }
        return true;
    }

    namespace dns_utils
    {

    //-----------------------------------------------------------------------
    // TODO: parse the string in a less stupid way, probably with regex
    std::string address_from_txt_record(const std::string& s)
    {
        // make sure the txt record has "oa1:nav recipient_address=" and find it
        auto pos = s.find("oa1:nav recipient_address=");
        if (pos == std::string::npos)
            return {};
        pos += 26; // move past "ao1:nav recipient_address="
        auto pos_end = s.find(";", pos);
        if (pos == std::string::npos)
            return {};
        return s.substr(pos, pos_end-pos);
    }
    /**
 * @brief gets a navcoin address from the TXT record of a DNS entry
 *
 * gets the navcoin address from the  TXT  record of the DNS entry associated
 * with  <url>.   If this lookup fails,  or the TXT record does not contain a
 * NAV address in the correct format, returns an empty string. <dnssec_valid>
 * will be set true or false according to whether or not the DNS query passes
 * DNSSEC validation.
 *
 * @param url the url to look up
 * @param dnssec_valid return-by-reference for DNSSEC status of query
 *
 * @return a navcoin address (as a string) or an empty string
 */
    std::vector<std::string> addresses_from_url(const std::string& url, bool& dnssec_valid)
    {
        std::vector<std::string> addresses;
        // get txt records
        bool dnssec_available, dnssec_isvalid;
        std::string oa_addr = DNSResolver::instance().get_dns_format_from_oa_address(url);
        auto records = DNSResolver::instance().get_txt_record(oa_addr, dnssec_available, dnssec_isvalid);

        // TODO: update this to allow for conveying that dnssec was not available
        if (dnssec_available && dnssec_isvalid)
        {
            dnssec_valid = true;
        }
        else dnssec_valid = false;

        // for each txt record, try to find a navcoin address in it.
        for (auto& rec : records)
        {
            std::string addr = address_from_txt_record(rec);
            if (addr.size())
            {
                addresses.push_back(addr);
            }
        }
        return addresses;
    }

    std::string get_account_address_as_str_from_url(const std::string& url, bool& dnssec_valid, std::function<std::string(const std::string&, const std::vector<std::string>&, bool)> dns_confirm)
    {
        // attempt to get address from dns query
        auto addresses = addresses_from_url(url, dnssec_valid);
        if (addresses.empty())
        {
            LogPrintf("OpenAlias: wrong address: %s");
            return {};
        }
        return dns_confirm(url, addresses, dnssec_valid);
    }

    namespace
    {
    bool dns_records_match(const std::vector<std::string>& a, const std::vector<std::string>& b)
    {
        if (a.size() != b.size()) return false;

        for (const auto& record_in_a : a)
        {
            bool ok = false;
            for (const auto& record_in_b : b)
            {
                if (record_in_a == record_in_b)
                {
                    ok = true;
                    break;
                }
            }
            if (!ok) return false;
        }

        return true;
    }
    }

    bool load_txt_records_from_dns(std::vector<std::string> &good_records, const std::vector<std::string> &dns_urls)
    {
        // Prevent infinite recursion when distributing
        if (dns_urls.empty()) return false;

        std::vector<std::vector<std::string> > records;
        records.resize(dns_urls.size());

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, dns_urls.size() - 1);
        size_t first_index = dis(gen);

        // send all requests in parallel
        std::vector<boost::thread> threads(dns_urls.size());
        std::deque<bool> avail(dns_urls.size(), false), valid(dns_urls.size(), false);
        for (size_t n = 0; n < dns_urls.size(); ++n)
        {
            threads[n] = boost::thread([n, dns_urls, &records, &avail, &valid](){
                records[n] = utils::DNSResolver::instance().get_txt_record(dns_urls[n], avail[n], valid[n]);
            });
        }
        for (size_t n = 0; n < dns_urls.size(); ++n)
            threads[n].join();

        size_t cur_index = first_index;
        do
        {
            const std::string &url = dns_urls[cur_index];
            if (!avail[cur_index])
            {
                records[cur_index].clear();
                LogPrintf("DNSSEC not available for checkpoint update at URL: %s, skipping.", url);
            }
            if (!valid[cur_index])
            {
                records[cur_index].clear();
                LogPrintf("DNSSEC validation failed for checkpoint update at URL: %s, skipping.", url);
            }

            cur_index++;
            if (cur_index == dns_urls.size())
            {
                cur_index = 0;
            }
        } while (cur_index != first_index);

        size_t num_valid_records = 0;

        for( const auto& record_set : records)
        {
            if (record_set.size() != 0)
            {
                num_valid_records++;
            }
        }

        if (num_valid_records < 2)
        {
            LogPrintf("OpenAlias: WARNING: no two valid Pulse DNS checkpoint records were received");
            return false;
        }

        int good_records_index = -1;
        for (size_t i = 0; i < records.size() - 1; ++i)
        {
            if (records[i].size() == 0) continue;

            for (size_t j = i + 1; j < records.size(); ++j)
            {
                if (dns_records_match(records[i], records[j]))
                {
                    good_records_index = i;
                    break;
                }
            }
            if (good_records_index >= 0) break;
        }

        if (good_records_index < 0)
        {
            LogPrintf("OpenAlias: WARNING: no two Pulse DNS checkpoint records matched");
            return false;
        }

        good_records = records[good_records_index];
        return true;
    }

    std::string parse_dns_public(const char *s)
    {
        unsigned ip0, ip1, ip2, ip3;
        char c;
        std::string dns_public_addr;
        if (!strcmp(s, "tcp"))
        {
            dns_public_addr = GetArg("-dnsserver","8.8.4.4");
            LogPrintf("Parse-DNS-Public: Using default public DNS server: %s (TCP)",dns_public_addr);
        }
        else if (sscanf(s, "tcp://%u.%u.%u.%u%c", &ip0, &ip1, &ip2, &ip3, &c) == 4)
        {
            if (ip0 > 255 || ip1 > 255 || ip2 > 255 || ip3 > 255)
            {
                LogPrintf("Parse-DNS-Public: Invalid IP: %s, using default",s);
            }
            else
            {
                dns_public_addr = std::string(s + strlen("tcp://"));
            }
        }
        else
        {
            LogPrintf("Parse-DNS-Public: Invalid DNS_PUBLIC contents, ignored");
        }
        return dns_public_addr;
    }

    }  // namespace utils::dns_utils

}  // namespace utils

