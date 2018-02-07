// Copyright (c) 2017-2018, The Fonero Project.
// Copyright (c) 2014-2017 The Fonero Project.
// Portions Copyright (c) 2012-2013 The Cryptonote developers.
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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "gtest/gtest.h"

#include "wallet/wallet2_api.h"
#include "wallet/wallet2.h"
#include "include_base_utils.h"

#include <boost/chrono/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <iostream>
#include <vector>
#include <atomic>
#include <functional>


using namespace std;
//unsigned int epee::g_test_dbg_lock_sleep = 0;

namespace Consts
{


// TODO: get rid of hardcoded paths

const char * WALLET_NAME = "testwallet";
const char * WALLET_NAME_MAINNET = "testwallet_mainnet";
const char * WALLET_NAME_COPY = "testwallet_copy";
const char * WALLET_NAME_WITH_DIR = "walletdir/testwallet_test";
const char * WALLET_NAME_WITH_DIR_NON_WRITABLE = "/var/walletdir/testwallet_test";
const char * WALLET_PASS = "password";
const char * WALLET_PASS2 = "password22";
const char * WALLET_LANG = "English";

std::string WALLETS_ROOT_DIR = "/var/fonero/testnet_pvt";
std::string TESTNET_WALLET1_NAME;
std::string TESTNET_WALLET2_NAME;
std::string TESTNET_WALLET3_NAME;
std::string TESTNET_WALLET4_NAME;
std::string TESTNET_WALLET5_NAME;
std::string TESTNET_WALLET6_NAME;

const char * TESTNET_WALLET_PASS = "";

std::string CURRENT_SRC_WALLET;
std::string CURRENT_DST_WALLET;

const uint64_t AMOUNT_10FNO =  10000000000000L;
const uint64_t AMOUNT_5FNO  =  5000000000000L;
const uint64_t AMOUNT_1FNO  =  1000000000000L;

const std::string PAYMENT_ID_EMPTY = "";

std::string TESTNET_DAEMON_ADDRESS = "localhost:28181";
std::string MAINNET_DAEMON_ADDRESS = "localhost:18181";


}



using namespace Consts;

struct Utils
{
    static void deleteWallet(const std::string & walletname)
    {
        std::cout << "** deleting wallet: " << walletname << std::endl;
        boost::filesystem::remove(walletname);
        boost::filesystem::remove(walletname + ".address.txt");
        boost::filesystem::remove(walletname + ".keys");
    }

    static void deleteDir(const std::string &path)
    {
        std::cout << "** removing dir recursively: " << path  << std::endl;
        boost::filesystem::remove_all(path);
    }

    static void print_transaction(Fonero::TransactionInfo * t)
    {

        std::cout << "d: "
                  << (t->direction() == Fonero::TransactionInfo::Direction_In ? "in" : "out")
                  << ", pe: " << (t->isPending() ? "true" : "false")
                  << ", bh: " << t->blockHeight()
                  << ", a: " << Fonero::Wallet::displayAmount(t->amount())
                  << ", f: " << Fonero::Wallet::displayAmount(t->fee())
                  << ", h: " << t->hash()
                  << ", pid: " << t->paymentId()
                  << std::endl;
    }

    static std::string get_wallet_address(const std::string &filename, const std::string &password)
    {
        Fonero::WalletManager *wmgr = Fonero::WalletManagerFactory::getWalletManager();
        Fonero::Wallet * w = wmgr->openWallet(filename, password, true);
        std::string result = w->address();
        wmgr->closeWallet(w);
        return result;
    }
};


struct WalletManagerTest : public testing::Test
{
    Fonero::WalletManager * wmgr;


    WalletManagerTest()
    {
        std::cout << __FUNCTION__ << std::endl;
        wmgr = Fonero::WalletManagerFactory::getWalletManager();
        // Fonero::WalletManagerFactory::setLogLevel(Fonero::WalletManagerFactory::LogLevel_4);
        Utils::deleteWallet(WALLET_NAME);
        Utils::deleteDir(boost::filesystem::path(WALLET_NAME_WITH_DIR).parent_path().string());
    }


    ~WalletManagerTest()
    {
        std::cout << __FUNCTION__ << std::endl;
        //deleteWallet(WALLET_NAME);
    }

};

struct WalletManagerMainnetTest : public testing::Test
{
    Fonero::WalletManager * wmgr;


    WalletManagerMainnetTest()
    {
        std::cout << __FUNCTION__ << std::endl;
        wmgr = Fonero::WalletManagerFactory::getWalletManager();
        Utils::deleteWallet(WALLET_NAME_MAINNET);
    }


    ~WalletManagerMainnetTest()
    {
        std::cout << __FUNCTION__ << std::endl;
    }

};

struct WalletTest1 : public testing::Test
{
    Fonero::WalletManager * wmgr;

    WalletTest1()
    {
        wmgr = Fonero::WalletManagerFactory::getWalletManager();
    }


};


struct WalletTest2 : public testing::Test
{
    Fonero::WalletManager * wmgr;

    WalletTest2()
    {
        wmgr = Fonero::WalletManagerFactory::getWalletManager();
    }

};

TEST_F(WalletManagerTest, WalletManagerCreatesWallet)
{

    Fonero::Wallet * wallet = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    ASSERT_TRUE(wallet->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(!wallet->seed().empty());
    std::vector<std::string> words;
    std::string seed = wallet->seed();
    boost::split(words, seed, boost::is_any_of(" "), boost::token_compress_on);
    ASSERT_TRUE(words.size() == 25);
    std::cout << "** seed: " << wallet->seed() << std::endl;
    ASSERT_FALSE(wallet->address().empty());
    std::cout << "** address: " << wallet->address() << std::endl;
    ASSERT_TRUE(wmgr->closeWallet(wallet));

}

TEST_F(WalletManagerTest, WalletManagerOpensWallet)
{

    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Fonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_TRUE(wallet2->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    std::cout << "** seed: " << wallet2->seed() << std::endl;
}


TEST_F(WalletManagerTest, WalletMaxAmountAsString)
{
    LOG_PRINT_L3("max amount: " << Fonero::Wallet::displayAmount(
                     Fonero::Wallet::maximumAllowedAmount()));

}


TEST_F(WalletManagerTest, WalletAmountFromString)
{
    uint64_t amount = Fonero::Wallet::amountFromString("18446740");
    ASSERT_TRUE(amount > 0);
    amount = Fonero::Wallet::amountFromString("11000000000000");
    ASSERT_FALSE(amount > 0);
    amount = Fonero::Wallet::amountFromString("0.0");
    ASSERT_FALSE(amount > 0);
    amount = Fonero::Wallet::amountFromString("10.1");
    ASSERT_TRUE(amount > 0);

}

void open_wallet_helper(Fonero::WalletManager *wmgr, Fonero::Wallet **wallet, const std::string &pass, boost::mutex *mutex)
{
    if (mutex)
        mutex->lock();
    LOG_PRINT_L3("opening wallet in thread: " << boost::this_thread::get_id());
    *wallet = wmgr->openWallet(WALLET_NAME, pass, true);
    LOG_PRINT_L3("wallet address: " << (*wallet)->address());
    LOG_PRINT_L3("wallet status: " << (*wallet)->status());
    LOG_PRINT_L3("closing wallet in thread: " << boost::this_thread::get_id());
    if (mutex)
        mutex->unlock();
}




//TEST_F(WalletManagerTest, WalletManagerOpensWalletWithPasswordAndReopenMultiThreaded)
//{
//    // create password protected wallet
//    std::string wallet_pass = "password";
//    std::string wrong_wallet_pass = "1111";
//    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, wallet_pass, WALLET_LANG, true);
//    std::string seed1 = wallet1->seed();
//    ASSERT_TRUE(wmgr->closeWallet(wallet1));

//    Fonero::Wallet *wallet2 = nullptr;
//    Fonero::Wallet *wallet3 = nullptr;

//    std::mutex mutex;
//    std::thread thread1(open_wallet, wmgr, &wallet2, wrong_wallet_pass, &mutex);
//    thread1.join();
//    ASSERT_TRUE(wallet2->status() != Fonero::Wallet::Status_Ok);
//    ASSERT_TRUE(wmgr->closeWallet(wallet2));

//    std::thread thread2(open_wallet, wmgr, &wallet3, wallet_pass, &mutex);
//    thread2.join();

//    ASSERT_TRUE(wallet3->status() == Fonero::Wallet::Status_Ok);
//    ASSERT_TRUE(wmgr->closeWallet(wallet3));
//}


TEST_F(WalletManagerTest, WalletManagerOpensWalletWithPasswordAndReopen)
{
    // create password protected wallet
    std::string wallet_pass = "password";
    std::string wrong_wallet_pass = "1111";
    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, wallet_pass, WALLET_LANG, true);
    std::string seed1 = wallet1->seed();
    ASSERT_TRUE(wmgr->closeWallet(wallet1));

    Fonero::Wallet *wallet2 = nullptr;
    Fonero::Wallet *wallet3 = nullptr;
    boost::mutex mutex;

    open_wallet_helper(wmgr, &wallet2, wrong_wallet_pass, nullptr);
    ASSERT_TRUE(wallet2 != nullptr);
    ASSERT_TRUE(wallet2->status() != Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wmgr->closeWallet(wallet2));

    open_wallet_helper(wmgr, &wallet3, wallet_pass, nullptr);
    ASSERT_TRUE(wallet3 != nullptr);
    ASSERT_TRUE(wallet3->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wmgr->closeWallet(wallet3));
}


TEST_F(WalletManagerTest, WalletManagerStoresWallet)
{

    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    wallet1->store("");
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Fonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_TRUE(wallet2->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
}


TEST_F(WalletManagerTest, WalletManagerMovesWallet)
{

    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string WALLET_NAME_MOVED = std::string("/tmp/") + WALLET_NAME + ".moved";
    std::string seed1 = wallet1->seed();
    ASSERT_TRUE(wallet1->store(WALLET_NAME_MOVED));

    Fonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME_MOVED, WALLET_PASS);
    ASSERT_TRUE(wallet2->filename() == WALLET_NAME_MOVED);
    ASSERT_TRUE(wallet2->keysFilename() == WALLET_NAME_MOVED + ".keys");
    ASSERT_TRUE(wallet2->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
}


TEST_F(WalletManagerTest, WalletManagerChangesPassword)
{
    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    ASSERT_TRUE(wallet1->setPassword(WALLET_PASS2));
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Fonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME, WALLET_PASS2);
    ASSERT_TRUE(wallet2->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
    Fonero::Wallet * wallet3 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_FALSE(wallet3->status() == Fonero::Wallet::Status_Ok);
}



TEST_F(WalletManagerTest, WalletManagerRecoversWallet)
{
    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();
    ASSERT_FALSE(address1.empty());
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Utils::deleteWallet(WALLET_NAME);
    Fonero::Wallet * wallet2 = wmgr->recoveryWallet(WALLET_NAME, seed1);
    ASSERT_TRUE(wallet2->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    ASSERT_TRUE(wallet2->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
}


TEST_F(WalletManagerTest, WalletManagerStoresWallet1)
{
    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();

    ASSERT_TRUE(wallet1->store(""));
    ASSERT_TRUE(wallet1->store(WALLET_NAME_COPY));
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Fonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME_COPY, WALLET_PASS);
    ASSERT_TRUE(wallet2->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    ASSERT_TRUE(wallet2->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
}


TEST_F(WalletManagerTest, WalletManagerStoresWallet2)
{
    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();

    ASSERT_TRUE(wallet1->store(WALLET_NAME_WITH_DIR));
    ASSERT_TRUE(wmgr->closeWallet(wallet1));

    wallet1 = wmgr->openWallet(WALLET_NAME_WITH_DIR, WALLET_PASS);
    ASSERT_TRUE(wallet1->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet1->seed() == seed1);
    ASSERT_TRUE(wallet1->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}


TEST_F(WalletManagerTest, WalletManagerStoresWallet3)
{
    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();

    ASSERT_FALSE(wallet1->store(WALLET_NAME_WITH_DIR_NON_WRITABLE));
    ASSERT_TRUE(wmgr->closeWallet(wallet1));

    wallet1 = wmgr->openWallet(WALLET_NAME_WITH_DIR_NON_WRITABLE, WALLET_PASS);
    ASSERT_FALSE(wallet1->status() == Fonero::Wallet::Status_Ok);

    // "close" always returns true;
    ASSERT_TRUE(wmgr->closeWallet(wallet1));

    wallet1 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_TRUE(wallet1->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet1->seed() == seed1);
    ASSERT_TRUE(wallet1->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));

}


TEST_F(WalletManagerTest, WalletManagerStoresWallet4)
{
    Fonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();

    ASSERT_TRUE(wallet1->store(""));
    ASSERT_TRUE(wallet1->status() == Fonero::Wallet::Status_Ok);

    ASSERT_TRUE(wallet1->store(""));
    ASSERT_TRUE(wallet1->status() == Fonero::Wallet::Status_Ok);

    ASSERT_TRUE(wmgr->closeWallet(wallet1));

    wallet1 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_TRUE(wallet1->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet1->seed() == seed1);
    ASSERT_TRUE(wallet1->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}




TEST_F(WalletManagerTest, WalletManagerFindsWallet)
{
    std::vector<std::string> wallets = wmgr->findWallets(WALLETS_ROOT_DIR);
    ASSERT_FALSE(wallets.empty());
    std::cout << "Found wallets: " << std::endl;
    for (auto wallet_path: wallets) {
        std::cout << wallet_path << std::endl;
    }
}


TEST_F(WalletTest1, WalletGeneratesPaymentId)
{
    std::string payment_id = Fonero::Wallet::genPaymentId();
    ASSERT_TRUE(payment_id.length() == 16);
}


TEST_F(WalletTest1, WalletGeneratesIntegratedAddress)
{
    std::string payment_id = Fonero::Wallet::genPaymentId();

    Fonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    std::string integrated_address = wallet1->integratedAddress(payment_id);
    ASSERT_TRUE(integrated_address.length() == 106);
}


TEST_F(WalletTest1, WalletShowsBalance)
{
    Fonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    ASSERT_TRUE(wallet1->balance() > 0);
    ASSERT_TRUE(wallet1->unlockedBalance() > 0);

    uint64_t balance1 = wallet1->balance();
    uint64_t unlockedBalance1 = wallet1->unlockedBalance();
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Fonero::Wallet * wallet2 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);

    ASSERT_TRUE(balance1 == wallet2->balance());
    std::cout << "wallet balance: " << wallet2->balance() << std::endl;
    ASSERT_TRUE(unlockedBalance1 == wallet2->unlockedBalance());
    std::cout << "wallet unlocked balance: " << wallet2->unlockedBalance() << std::endl;
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
}

TEST_F(WalletTest1, WalletReturnsCurrentBlockHeight)
{
    Fonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    ASSERT_TRUE(wallet1->blockChainHeight() > 0);
    wmgr->closeWallet(wallet1);
}


TEST_F(WalletTest1, WalletReturnsDaemonBlockHeight)
{
    Fonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // wallet not connected to daemon
    ASSERT_TRUE(wallet1->daemonBlockChainHeight() == 0);
    ASSERT_TRUE(wallet1->status() != Fonero::Wallet::Status_Ok);
    ASSERT_FALSE(wallet1->errorString().empty());
    wmgr->closeWallet(wallet1);

    wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // wallet connected to daemon
    wallet1->init(TESTNET_DAEMON_ADDRESS, 0);
    ASSERT_TRUE(wallet1->daemonBlockChainHeight() > 0);
    std::cout << "daemonBlockChainHeight: " << wallet1->daemonBlockChainHeight() << std::endl;
    wmgr->closeWallet(wallet1);
}


TEST_F(WalletTest1, WalletRefresh)
{

    std::cout << "Opening wallet: " << CURRENT_SRC_WALLET << std::endl;
    Fonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    std::cout << "connecting to daemon: " << TESTNET_DAEMON_ADDRESS << std::endl;
    ASSERT_TRUE(wallet1->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet1->refresh());
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}

TEST_F(WalletTest1, WalletConvertsToString)
{
    std::string strAmount = Fonero::Wallet::displayAmount(AMOUNT_5FNO);
    ASSERT_TRUE(AMOUNT_5FNO == Fonero::Wallet::amountFromString(strAmount));

    ASSERT_TRUE(AMOUNT_5FNO == Fonero::Wallet::amountFromDouble(5.0));
    ASSERT_TRUE(AMOUNT_10FNO == Fonero::Wallet::amountFromDouble(10.0));
    ASSERT_TRUE(AMOUNT_1FNO == Fonero::Wallet::amountFromDouble(1.0));

}



TEST_F(WalletTest1, WalletTransaction)

{
    Fonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet1->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet1->refresh());
    uint64_t balance = wallet1->balance();
    ASSERT_TRUE(wallet1->status() == Fonero::PendingTransaction::Status_Ok);

    std::string recepient_address = Utils::get_wallet_address(CURRENT_DST_WALLET, TESTNET_WALLET_PASS);


    Fonero::PendingTransaction * transaction = wallet1->createTransaction(recepient_address,
                                                                             PAYMENT_ID_EMPTY,
                                                                             AMOUNT_10FNO,
                                                                             Fonero::PendingTransaction::Priority_Medium);
    ASSERT_TRUE(transaction->status() == Fonero::PendingTransaction::Status_Ok);
    wallet1->refresh();

    ASSERT_TRUE(wallet1->balance() == balance);
    ASSERT_TRUE(transaction->amount() == AMOUNT_10FNO);
    ASSERT_TRUE(transaction->commit());
    ASSERT_FALSE(wallet1->balance() == balance);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}

TEST_F(WalletTest1, WalletTransactionWithPriority)
{

    std::string payment_id = "";

    Fonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);

    // make sure testnet daemon is running
    ASSERT_TRUE(wallet1->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet1->refresh());
    uint64_t balance = wallet1->balance();
    ASSERT_TRUE(wallet1->status() == Fonero::PendingTransaction::Status_Ok);

    std::string recepient_address = Utils::get_wallet_address(CURRENT_DST_WALLET, TESTNET_WALLET_PASS);
    uint64_t fee   = 0;

    std::vector<Fonero::PendingTransaction::Priority> priorities =  {
         Fonero::PendingTransaction::Priority_Low,
         Fonero::PendingTransaction::Priority_Medium,
         Fonero::PendingTransaction::Priority_High
    };

    for (auto it = priorities.begin(); it != priorities.end(); ++it) {
        std::cerr << "Transaction priority: " << *it << std::endl;
        Fonero::PendingTransaction * transaction = wallet1->createTransaction(
                    recepient_address, payment_id, AMOUNT_5FNO, *it);
        std::cerr << "Transaction status: " << transaction->status() << std::endl;
        std::cerr << "Transaction fee: " << Fonero::Wallet::displayAmount(transaction->fee()) << std::endl;
        std::cerr << "Transaction error: " << transaction->errorString() << std::endl;
        ASSERT_TRUE(transaction->fee() > fee);
        ASSERT_TRUE(transaction->status() == Fonero::PendingTransaction::Status_Ok);
        fee = transaction->fee();
        wallet1->disposeTransaction(transaction);
    }
    wallet1->refresh();
    ASSERT_TRUE(wallet1->balance() == balance);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}



TEST_F(WalletTest1, WalletHistory)
{
    Fonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet1->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet1->refresh());
    Fonero::TransactionHistory * history = wallet1->history();
    history->refresh();
    ASSERT_TRUE(history->count() > 0);


    for (auto t: history->getAll()) {
        ASSERT_TRUE(t != nullptr);
        Utils::print_transaction(t);
    }
}

TEST_F(WalletTest1, WalletTransactionAndHistory)
{
    return;
    Fonero::Wallet * wallet_src = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src->refresh());
    Fonero::TransactionHistory * history = wallet_src->history();
    history->refresh();
    ASSERT_TRUE(history->count() > 0);
    size_t count1 = history->count();

    std::cout << "**** Transactions before transfer (" << count1 << ")" << std::endl;
    for (auto t: history->getAll()) {
        ASSERT_TRUE(t != nullptr);
        Utils::print_transaction(t);
    }

    std::string wallet4_addr = Utils::get_wallet_address(CURRENT_DST_WALLET, TESTNET_WALLET_PASS);


    Fonero::PendingTransaction * tx = wallet_src->createTransaction(wallet4_addr,
                                                                       PAYMENT_ID_EMPTY,
                                                                       AMOUNT_10FNO * 5);

    ASSERT_TRUE(tx->status() == Fonero::PendingTransaction::Status_Ok);
    ASSERT_TRUE(tx->commit());
    history = wallet_src->history();
    history->refresh();
    ASSERT_TRUE(count1 != history->count());

    std::cout << "**** Transactions after transfer (" << history->count() << ")" << std::endl;
    for (auto t: history->getAll()) {
        ASSERT_TRUE(t != nullptr);
        Utils::print_transaction(t);
    }
}


TEST_F(WalletTest1, WalletTransactionWithPaymentId)
{

    Fonero::Wallet * wallet_src = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src->refresh());
    Fonero::TransactionHistory * history = wallet_src->history();
    history->refresh();
    ASSERT_TRUE(history->count() > 0);
    size_t count1 = history->count();

    std::cout << "**** Transactions before transfer (" << count1 << ")" << std::endl;
    for (auto t: history->getAll()) {
        ASSERT_TRUE(t != nullptr);
        Utils::print_transaction(t);
    }

    std::string wallet4_addr = Utils::get_wallet_address(CURRENT_DST_WALLET, TESTNET_WALLET_PASS);

    std::string payment_id = Fonero::Wallet::genPaymentId();
    ASSERT_TRUE(payment_id.length() == 16);


    Fonero::PendingTransaction * tx = wallet_src->createTransaction(wallet4_addr,
                                                                       payment_id,
                                                                       AMOUNT_1FNO);

    ASSERT_TRUE(tx->status() == Fonero::PendingTransaction::Status_Ok);
    ASSERT_TRUE(tx->commit());
    history = wallet_src->history();
    history->refresh();
    ASSERT_TRUE(count1 != history->count());

    bool payment_id_in_history = false;

    std::cout << "**** Transactions after transfer (" << history->count() << ")" << std::endl;
    for (auto t: history->getAll()) {
        ASSERT_TRUE(t != nullptr);
        Utils::print_transaction(t);
        if (t->paymentId() == payment_id) {
            payment_id_in_history = true;
        }
    }

    ASSERT_TRUE(payment_id_in_history);
}


struct MyWalletListener : public Fonero::WalletListener
{

    Fonero::Wallet * wallet;
    uint64_t total_tx;
    uint64_t total_rx;
    boost::mutex  mutex;
    boost::condition_variable cv_send;
    boost::condition_variable cv_receive;
    boost::condition_variable cv_update;
    boost::condition_variable cv_refresh;
    boost::condition_variable cv_newblock;
    bool send_triggered;
    bool receive_triggered;
    bool newblock_triggered;
    bool update_triggered;
    bool refresh_triggered;



    MyWalletListener(Fonero::Wallet * wallet)
        : total_tx(0), total_rx(0)
    {
        reset();

        this->wallet = wallet;
        this->wallet->setListener(this);
    }

    void reset()
    {
        send_triggered = receive_triggered = update_triggered = refresh_triggered = false;
    }

    virtual void moneySpent(const string &txId, uint64_t amount)
    {
        std::cerr << "wallet: " << wallet->address() << "**** just spent money ("
                  << txId  << ", " << wallet->displayAmount(amount) << ")" << std::endl;
        total_tx += amount;
        send_triggered = true;
        cv_send.notify_one();
    }

    virtual void moneyReceived(const string &txId, uint64_t amount)
    {
        std::cout << "wallet: " << wallet->address() << "**** just received money ("
                  << txId  << ", " << wallet->displayAmount(amount) << ")" << std::endl;
        total_rx += amount;
        receive_triggered = true;
        cv_receive.notify_one();
    }

    virtual void unconfirmedMoneyReceived(const string &txId, uint64_t amount)
    {
        std::cout << "wallet: " << wallet->address() << "**** just received unconfirmed money ("
                  << txId  << ", " << wallet->displayAmount(amount) << ")" << std::endl;
        // Don't trigger recieve until tx is mined
        // total_rx += amount;
        // receive_triggered = true;
        // cv_receive.notify_one();
    }

    virtual void newBlock(uint64_t height)
    {
//        std::cout << "wallet: " << wallet->address()
//                  <<", new block received, blockHeight: " << height << std::endl;
        static int bc_height = wallet->daemonBlockChainHeight();
        std::cout << height
                  << " / " << bc_height/* 0*/
                  << std::endl;
        newblock_triggered = true;
        cv_newblock.notify_one();
    }

    virtual void updated()
    {
        std::cout << __FUNCTION__ << "Wallet updated";
        update_triggered = true;
        cv_update.notify_one();
    }

    virtual void refreshed()
    {
        std::cout << __FUNCTION__ <<  "Wallet refreshed";
        refresh_triggered = true;
        cv_refresh.notify_one();
    }

};




TEST_F(WalletTest2, WalletCallBackRefreshedSync)
{

    Fonero::Wallet * wallet_src = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    MyWalletListener * wallet_src_listener = new MyWalletListener(wallet_src);
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src_listener->refresh_triggered);
    ASSERT_TRUE(wallet_src->connected());
    boost::chrono::seconds wait_for = boost::chrono::seconds(60*3);
    boost::unique_lock<boost::mutex> lock (wallet_src_listener->mutex);
    wallet_src_listener->cv_refresh.wait_for(lock, wait_for);
    wmgr->closeWallet(wallet_src);
}




TEST_F(WalletTest2, WalletCallBackRefreshedAsync)
{

    Fonero::Wallet * wallet_src = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    MyWalletListener * wallet_src_listener = new MyWalletListener(wallet_src);

    boost::chrono::seconds wait_for = boost::chrono::seconds(20);
    boost::unique_lock<boost::mutex> lock (wallet_src_listener->mutex);
    wallet_src->init(MAINNET_DAEMON_ADDRESS, 0);
    wallet_src->startRefresh();
    std::cerr << "TEST: waiting on refresh lock...\n";
    wallet_src_listener->cv_refresh.wait_for(lock, wait_for);
    std::cerr << "TEST: refresh lock acquired...\n";
    ASSERT_TRUE(wallet_src_listener->refresh_triggered);
    ASSERT_TRUE(wallet_src->connected());
    std::cerr << "TEST: closing wallet...\n";
    wmgr->closeWallet(wallet_src);
}




TEST_F(WalletTest2, WalletCallbackSent)
{

    Fonero::Wallet * wallet_src = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src->refresh());
    MyWalletListener * wallet_src_listener = new MyWalletListener(wallet_src);
    uint64_t balance = wallet_src->balance();
    std::cout << "** Balance: " << wallet_src->displayAmount(wallet_src->balance()) <<  std::endl;
    Fonero::Wallet * wallet_dst = wmgr->openWallet(CURRENT_DST_WALLET, TESTNET_WALLET_PASS, true);

    uint64_t amount = AMOUNT_1FNO * 5;
    std::cout << "** Sending " << Fonero::Wallet::displayAmount(amount) << " to " << wallet_dst->address();


    Fonero::PendingTransaction * tx = wallet_src->createTransaction(wallet_dst->address(),
                                                                       PAYMENT_ID_EMPTY,
                                                                       amount);
    std::cout << "** Committing transaction: " << Fonero::Wallet::displayAmount(tx->amount())
              << " with fee: " << Fonero::Wallet::displayAmount(tx->fee());

    ASSERT_TRUE(tx->status() == Fonero::PendingTransaction::Status_Ok);
    ASSERT_TRUE(tx->commit());

    boost::chrono::seconds wait_for = boost::chrono::seconds(60*3);
    boost::unique_lock<boost::mutex> lock (wallet_src_listener->mutex);
    std::cerr << "TEST: waiting on send lock...\n";
    wallet_src_listener->cv_send.wait_for(lock, wait_for);
    std::cerr << "TEST: send lock acquired...\n";
    ASSERT_TRUE(wallet_src_listener->send_triggered);
    ASSERT_TRUE(wallet_src_listener->update_triggered);
    std::cout << "** Balance: " << wallet_src->displayAmount(wallet_src->balance()) <<  std::endl;
    ASSERT_TRUE(wallet_src->balance() < balance);
    wmgr->closeWallet(wallet_src);
    wmgr->closeWallet(wallet_dst);
}


TEST_F(WalletTest2, WalletCallbackReceived)
{

    Fonero::Wallet * wallet_src = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src->refresh());
    std::cout << "** Balance src1: " << wallet_src->displayAmount(wallet_src->balance()) <<  std::endl;

    Fonero::Wallet * wallet_dst = wmgr->openWallet(CURRENT_DST_WALLET, TESTNET_WALLET_PASS, true);
    ASSERT_TRUE(wallet_dst->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_dst->refresh());
    uint64_t balance = wallet_dst->balance();
    std::cout << "** Balance dst1: " << wallet_dst->displayAmount(wallet_dst->balance()) <<  std::endl;
    std::unique_ptr<MyWalletListener> wallet_dst_listener (new MyWalletListener(wallet_dst));

    uint64_t amount = AMOUNT_1FNO * 5;
    std::cout << "** Sending " << Fonero::Wallet::displayAmount(amount) << " to " << wallet_dst->address();
    Fonero::PendingTransaction * tx = wallet_src->createTransaction(wallet_dst->address(),
                                                                       PAYMENT_ID_EMPTY,
                                                                       amount);

    std::cout << "** Committing transaction: " << Fonero::Wallet::displayAmount(tx->amount())
              << " with fee: " << Fonero::Wallet::displayAmount(tx->fee());

    ASSERT_TRUE(tx->status() == Fonero::PendingTransaction::Status_Ok);
    ASSERT_TRUE(tx->commit());

    boost::chrono::seconds wait_for = boost::chrono::seconds(60*4);
    boost::unique_lock<boost::mutex> lock (wallet_dst_listener->mutex);
    std::cerr << "TEST: waiting on receive lock...\n";
    wallet_dst_listener->cv_receive.wait_for(lock, wait_for);
    std::cerr << "TEST: receive lock acquired...\n";
    ASSERT_TRUE(wallet_dst_listener->receive_triggered);
    ASSERT_TRUE(wallet_dst_listener->update_triggered);

    std::cout << "** Balance src2: " << wallet_dst->displayAmount(wallet_src->balance()) <<  std::endl;
    std::cout << "** Balance dst2: " << wallet_dst->displayAmount(wallet_dst->balance()) <<  std::endl;

    ASSERT_TRUE(wallet_dst->balance() > balance);

    wmgr->closeWallet(wallet_src);
    wmgr->closeWallet(wallet_dst);
}



TEST_F(WalletTest2, WalletCallbackNewBlock)
{

    Fonero::Wallet * wallet_src = wmgr->openWallet(TESTNET_WALLET5_NAME, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src->refresh());
    uint64_t bc1 = wallet_src->blockChainHeight();
    std::cout << "** Block height: " << bc1 << std::endl;


    std::unique_ptr<MyWalletListener> wallet_listener (new MyWalletListener(wallet_src));

    // wait max 4 min for new block
    boost::chrono::seconds wait_for = boost::chrono::seconds(60*4);
    boost::unique_lock<boost::mutex> lock (wallet_listener->mutex);
    std::cerr << "TEST: waiting on newblock lock...\n";
    wallet_listener->cv_newblock.wait_for(lock, wait_for);
    std::cerr << "TEST: newblock lock acquired...\n";
    ASSERT_TRUE(wallet_listener->newblock_triggered);
    uint64_t bc2 = wallet_src->blockChainHeight();
    std::cout << "** Block height: " << bc2 << std::endl;
    ASSERT_TRUE(bc2 > bc1);
    wmgr->closeWallet(wallet_src);

}

TEST_F(WalletManagerMainnetTest, CreateOpenAndRefreshWalletMainNetSync)
{

    Fonero::Wallet * wallet = wmgr->createWallet(WALLET_NAME_MAINNET, "", WALLET_LANG);
    std::unique_ptr<MyWalletListener> wallet_listener (new MyWalletListener(wallet));
    wallet->init(MAINNET_DAEMON_ADDRESS, 0);
    std::cerr << "TEST: waiting on refresh lock...\n";
    //wallet_listener->cv_refresh.wait_for(lock, wait_for);
    std::cerr << "TEST: refresh lock acquired...\n";
    ASSERT_TRUE(wallet_listener->refresh_triggered);
    ASSERT_TRUE(wallet->connected());
    ASSERT_TRUE(wallet->blockChainHeight() == wallet->daemonBlockChainHeight());
    std::cerr << "TEST: closing wallet...\n";
    wmgr->closeWallet(wallet);
}


TEST_F(WalletManagerMainnetTest, CreateAndRefreshWalletMainNetAsync)
{
    // supposing 120 seconds should be enough for fast refresh
    int SECONDS_TO_REFRESH = 120;

    Fonero::Wallet * wallet = wmgr->createWallet(WALLET_NAME_MAINNET, "", WALLET_LANG);
    std::unique_ptr<MyWalletListener> wallet_listener (new MyWalletListener(wallet));

    boost::chrono::seconds wait_for = boost::chrono::seconds(SECONDS_TO_REFRESH);
    boost::unique_lock<boost::mutex> lock (wallet_listener->mutex);
    wallet->init(MAINNET_DAEMON_ADDRESS, 0);
    wallet->startRefresh();
    std::cerr << "TEST: waiting on refresh lock...\n";
    wallet_listener->cv_refresh.wait_for(lock, wait_for);
    std::cerr << "TEST: refresh lock acquired...\n";
    ASSERT_TRUE(wallet->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet_listener->refresh_triggered);
    ASSERT_TRUE(wallet->connected());
    ASSERT_TRUE(wallet->blockChainHeight() == wallet->daemonBlockChainHeight());
    std::cerr << "TEST: closing wallet...\n";
    wmgr->closeWallet(wallet);
}

TEST_F(WalletManagerMainnetTest, OpenAndRefreshWalletMainNetAsync)
{

    // supposing 120 seconds should be enough for fast refresh
    int SECONDS_TO_REFRESH = 120;
    Fonero::Wallet * wallet = wmgr->createWallet(WALLET_NAME_MAINNET, "", WALLET_LANG);
    wmgr->closeWallet(wallet);
    wallet = wmgr->openWallet(WALLET_NAME_MAINNET, "");

    std::unique_ptr<MyWalletListener> wallet_listener (new MyWalletListener(wallet));

    boost::chrono::seconds wait_for = boost::chrono::seconds(SECONDS_TO_REFRESH);
    boost::unique_lock<boost::mutex> lock (wallet_listener->mutex);
    wallet->init(MAINNET_DAEMON_ADDRESS, 0);
    wallet->startRefresh();
    std::cerr << "TEST: waiting on refresh lock...\n";
    wallet_listener->cv_refresh.wait_for(lock, wait_for);
    std::cerr << "TEST: refresh lock acquired...\n";
    ASSERT_TRUE(wallet->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet_listener->refresh_triggered);
    ASSERT_TRUE(wallet->connected());
    ASSERT_TRUE(wallet->blockChainHeight() == wallet->daemonBlockChainHeight());
    std::cerr << "TEST: closing wallet...\n";
    wmgr->closeWallet(wallet);

}

TEST_F(WalletManagerMainnetTest, RecoverAndRefreshWalletMainNetAsync)
{

    // supposing 120 seconds should be enough for fast refresh
    int SECONDS_TO_REFRESH = 120;
    Fonero::Wallet * wallet = wmgr->createWallet(WALLET_NAME_MAINNET, "", WALLET_LANG);
    std::string seed = wallet->seed();
    std::string address = wallet->address();
    wmgr->closeWallet(wallet);

    // deleting wallet files
    Utils::deleteWallet(WALLET_NAME_MAINNET);
    // ..and recovering wallet from seed

    wallet = wmgr->recoveryWallet(WALLET_NAME_MAINNET, seed);
    ASSERT_TRUE(wallet->status() == Fonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet->address() == address);
    std::unique_ptr<MyWalletListener> wallet_listener (new MyWalletListener(wallet));
    boost::chrono::seconds wait_for = boost::chrono::seconds(SECONDS_TO_REFRESH);
    boost::unique_lock<boost::mutex> lock (wallet_listener->mutex);
    wallet->init(MAINNET_DAEMON_ADDRESS, 0);
    wallet->startRefresh();
    std::cerr << "TEST: waiting on refresh lock...\n";

    // here we wait for 120 seconds and test if wallet doesn't syncrnonize blockchain completely,
    // as it needs much more than 120 seconds for mainnet

    wallet_listener->cv_refresh.wait_for(lock, wait_for);
    ASSERT_TRUE(wallet->status() == Fonero::Wallet::Status_Ok);
    ASSERT_FALSE(wallet_listener->refresh_triggered);
    ASSERT_TRUE(wallet->connected());
    ASSERT_FALSE(wallet->blockChainHeight() == wallet->daemonBlockChainHeight());
    std::cerr << "TEST: closing wallet...\n";
    wmgr->closeWallet(wallet);
    std::cerr << "TEST: wallet closed\n";

}



int main(int argc, char** argv)
{
    // we can override default values for "TESTNET_DAEMON_ADDRESS" and "WALLETS_ROOT_DIR"

    const char * testnet_daemon_addr = std::getenv("TESTNET_DAEMON_ADDRESS");
    if (testnet_daemon_addr) {
        TESTNET_DAEMON_ADDRESS = testnet_daemon_addr;
    }

    const char * mainnet_daemon_addr = std::getenv("MAINNET_DAEMON_ADDRESS");
    if (mainnet_daemon_addr) {
        MAINNET_DAEMON_ADDRESS = mainnet_daemon_addr;
    }



    const char * wallets_root_dir = std::getenv("WALLETS_ROOT_DIR");
    if (wallets_root_dir) {
        WALLETS_ROOT_DIR = wallets_root_dir;
    }


    TESTNET_WALLET1_NAME = WALLETS_ROOT_DIR + "/wallet_01.bin";
    TESTNET_WALLET2_NAME = WALLETS_ROOT_DIR + "/wallet_02.bin";
    TESTNET_WALLET3_NAME = WALLETS_ROOT_DIR + "/wallet_03.bin";
    TESTNET_WALLET4_NAME = WALLETS_ROOT_DIR + "/wallet_04.bin";
    TESTNET_WALLET5_NAME = WALLETS_ROOT_DIR + "/wallet_05.bin";
    TESTNET_WALLET6_NAME = WALLETS_ROOT_DIR + "/wallet_06.bin";

    CURRENT_SRC_WALLET = TESTNET_WALLET5_NAME;
    CURRENT_DST_WALLET = TESTNET_WALLET1_NAME;

    ::testing::InitGoogleTest(&argc, argv);
    Fonero::WalletManagerFactory::setLogLevel(Fonero::WalletManagerFactory::LogLevel_Max);
    return RUN_ALL_TESTS();
}
