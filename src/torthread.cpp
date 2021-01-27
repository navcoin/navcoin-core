// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "torthread.h"

extern "C" {
    int tor_main(int argc, char *argv[]);
}

static boost::thread torThread;

void TorRun() {
    boost::filesystem::path tor_dir = GetDataDir() / "tor";
    boost::filesystem::create_directory(tor_dir);
    boost::filesystem::path log_file = tor_dir / "tor.log";
    boost::filesystem::path torrc_file = tor_dir / "torrc";

    LogPrintf("Starting Tor thread...\n");

    std::vector<std::string> args = { "tor",
                                      "--Log",
                                      ("info file " + log_file.string()),
                                      "--SocksPort",
                                      std::to_string(GetArg("-torsocksport", 9044)),
                                      "--ignore-missing-torrc",
                                      "-f",
                                      (tor_dir / "torrc").string(),
                                      "--HiddenServiceDir",
                                      (tor_dir / "onion").string(),
                                      "--ControlPort",
                                      "9051",
                                      "--HiddenServiceVersion",
                                      "3",
                                      "--HiddenServicePort",
                                      "2222",
                                      "--CookieAuthentication",
                                      "1",
                                      "--quiet"
                                    };

    std::vector<char *> argv_c;

    std::transform(args.begin(), args.end(), std::back_inserter(argv_c), [](const std::string s) -> char* {
        char *pc = new char[s.size()+1];
        std::strcpy(pc, s.c_str());
        return pc;
    });

    tor_main(argv_c.size(), (char **)argv_c.data());
}

void TorThreadInit(){
    torThread = boost::thread(boost::bind(&TraceThread<void (*)()>, "torthread", &TorThreadStart));
}

void TorThreadStop()
{
    //torThread.join();
    LogPrintf("Tor thread exited.\n");
}

void TorThreadInterrupt()
{
    LogPrintf("Interrupting Tor thread\n");
}

void TorThreadStart()
{
    // Make this thread recognisable as the tor thread
    RenameThread("TorThread");

    try
    {
        TorRun();
    }
    catch (std::exception& e) {
        printf("%s\n", e.what());
    }
}
