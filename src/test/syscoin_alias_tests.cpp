#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "util.h"
#include "rpc/server.h"
#include "alias.h"
#include "cert.h"
#include "base58.h"
#include "chainparams.h"
#include <boost/test/unit_test.hpp>
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

BOOST_FIXTURE_TEST_SUITE (syscoin_alias_tests, BasicSyscoinTestingSetup)
const unsigned int MAX_ALIAS_UPDATES_PER_BLOCK = 5;
BOOST_AUTO_TEST_CASE (generate_sysrates_alias)
{
	printf("Running generate_sysrates_alias...\n");
	ECC_Start();
	CreateSysRatesIfNotExist();
}
BOOST_AUTO_TEST_CASE (generate_big_aliasdata)
{
	printf("Running generate_big_aliasdata...\n");
	GenerateBlocks(5);
	// 1024 bytes long
	string gooddata = "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";
	// 1110 bytes long
	string baddata =   "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadddfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsddfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdf";

	AliasNew("node1", "jag1", gooddata, gooddata);
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg jag2 " + gooddata + " " + baddata));
	GenerateBlocks(5);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg jag2"), runtime_error);	
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg jag2 " + baddata + " " + baddata));
	GenerateBlocks(5);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg jag2"), runtime_error);		
}
BOOST_AUTO_TEST_CASE (generate_aliaswitness)
{
	printf("Running generate_aliaswitness...\n");
	GenerateBlocks(5);
	UniValue r;
	AliasNew("node1", "witness1", "pub");
	AliasNew("node2", "witness2", "pub");
	string hex_str = AliasUpdate("node1", "witness1", "newpubdata", "\"\"", "\"\"", "\"\"", "witness2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo witness1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), "pub");
	BOOST_CHECK(!hex_str.empty());
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "syscoinsignrawtransaction " + hex_str));
	GenerateBlocks(5, "node2");
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo witness1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), "newpubdata");
}
BOOST_AUTO_TEST_CASE (generate_big_aliasname)
{
	printf("Running generate_big_aliasname...\n");
	GenerateBlocks(5);
	// 64 bytes long
	string goodname = "sfsdfdfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsdfdd";
	// 1024 bytes long
	string gooddata = "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";	
	// 65 bytes long
	string badname =  "sfsdfdfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsddfda";
	AliasNew("node1", goodname, goodname, gooddata);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg " + badname + " 3d"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_aliasupdate)
{
	printf("Running generate_aliasupdate...\n");
	GenerateBlocks(1);
	AliasNew("node1", "jagupdate", "data");
	AliasNew("node1", "jagupdate1", "data");
	// update an alias that isn't yours
	string hex_str = AliasUpdate("node2", "jagupdate");
	BOOST_CHECK(!hex_str.empty());
	// only update alias, no data
	hex_str = AliasUpdate("node1", "jagupdate");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate1");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate", "newpub");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate", "newpub1");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate1", "newpub2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate1", "newpub3");
	BOOST_CHECK(hex_str.empty());

}
BOOST_AUTO_TEST_CASE (generate_aliasmultiupdate)
{
	printf("Running generate_aliasmultiupdate...\n");
	GenerateBlocks(1);
	UniValue r;
	AliasNew("node1", "jagmultiupdate", "data");
	string hex_str = AliasUpdate("node1", "jagmultiupdate", "changeddata", "privdata");
	BOOST_CHECK(hex_str.empty());
	// can do 5 free updates, 1 above and 4 below
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK-1;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata1"));

	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");

	hex_str = AliasTransfer("node1", "jagmultiupdate", "node2", "changeddata2", "pvtdata2");
	BOOST_CHECK(hex_str.empty());
	// after transfer it can't update alias even though there are utxo's available from old owner
	hex_str = AliasUpdate("node1", "jagmultiupdate", "changedata3", "pvtdata2");
	BOOST_CHECK(!hex_str.empty());

	// new owner can update
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata4"));

	// after generation MAX_ALIAS_UPDATES_PER_BLOCK utxo's should be available
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node2");
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata5"));

	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata6"), runtime_error);
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node2");
	// transfer sends utxo's to new owner
	hex_str = AliasTransfer("node2", "jagmultiupdate", "node1", "changeddata7");
	BOOST_CHECK(hex_str.empty());
	// ensure can't update after transfer
	hex_str = AliasTransfer("node2", "jagmultiupdate", "node1", "changedata8");
	BOOST_CHECK(!hex_str.empty());
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata9"));
	
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata10"), runtime_error);
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");
	hex_str = AliasUpdate("node1", "jagmultiupdate", "changeddata11", "privdata");
	BOOST_CHECK(hex_str.empty());
}

BOOST_AUTO_TEST_CASE (generate_sendmoneytoalias)
{
	printf("Running generate_sendmoneytoalias...\n");
	GenerateBlocks(5, "node2");
	AliasNew("node2", "sendnode2", "changeddata2");
	AliasNew("node3", "sendnode3", "changeddata2");
	UniValue r;
	// get balance of node2 first to know we sent right amount oater
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance sendnode2"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo sendnode2"));
	string node2address = find_value(r.get_obj(), "address").get_str();
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress sendnode2 1.335"), runtime_error);
	GenerateBlocks(1);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo sendnode3"));
	string node3address = find_value(r.get_obj(), "address").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance sendnode2"));
	balanceBefore += 1.335*COIN;
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);
	// after expiry can still send money to it
	GenerateBlocks(101);	
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress sendnode2 1.335"), runtime_error);
	GenerateBlocks(1);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance sendnode2"));
	balanceBefore += 1.335*COIN;
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);

	// pay to node2/node3 wallets for alias funding for tests
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress " + node2address + " 500000"), runtime_error);
	GenerateBlocks(10);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress " + node3address + " 500000"), runtime_error);
	GenerateBlocks(10);

}
BOOST_AUTO_TEST_CASE (generate_aliaspay)
{
	printf("Running generate_aliaspay...\n");
	UniValue r;
	GenerateBlocks(5);
	AliasNew("node1", "alias1.aliaspay.tld", "changeddata2");
	AliasNew("node2", "alias2.aliaspay.tld", "changeddata2");
	AliasNew("node3", "alias3.aliaspay.tld", "changeddata2");
	GenerateBlocks(10);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress alias2.aliaspay.tld 2000"), runtime_error);
	GenerateBlocks(10);

	// get balance of node2 and node3 first to know we sent right amount later
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo alias2.aliaspay.tld"));
	string node2address = find_value(r.get_obj(), "address").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance alias2.aliaspay.tld"));
	CAmount balanceBeforeFrom = AmountFromValue(find_value(r.get_obj(), "balance"));

	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo alias3.aliaspay.tld"));
	string node3address = find_value(r.get_obj(), "address").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasbalance alias3.aliaspay.tld"));
	CAmount balanceBeforeTo = AmountFromValue(find_value(r.get_obj(), "balance"));

	//send amount
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliaspay alias2.aliaspay.tld USD \"{\\\"alias3.aliaspay.tld\\\":0.4}\""));
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node3");

	// I want to convert 0.4 USD to SYS
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasconvertcurrency alias1.aliaspay.tld USD SYS 0.4"));
	CAmount sysDiff = AmountFromValue(find_value(r.get_obj(), "convertedrate"));

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance alias2.aliaspay.tld"));
	CAmount balanceAfterFrom = AmountFromValue(find_value(r.get_obj(), "balance"));
	CAmount balanceTestAfterFrom = balanceBeforeFrom - sysDiff;
	// account for fees from the sender alias (0.1 SYS maximum)
	BOOST_CHECK(abs(balanceAfterFrom - balanceTestAfterFrom) <= 0.1*COIN);

	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasbalance alias3.aliaspay.tld"));
	CAmount balanceAfterTo = AmountFromValue(find_value(r.get_obj(), "balance"));
	CAmount balanceTestAfterTo = balanceBeforeTo + sysDiff;
	BOOST_CHECK_EQUAL(balanceAfterTo, balanceTestAfterTo);

	// pay to node2/node3 wallets for alias funding for tests
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress " + node2address + " 500000"), runtime_error);
	GenerateBlocks(10);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress " + node3address + " 500000"), runtime_error);
	GenerateBlocks(10);

	// there should be atleast 1 UTXO left in the aliases used to update
	AliasUpdate("node1", "alias1.aliaspay.tld", "changeddata11a", "privdata1");
	AliasUpdate("node2", "alias2.aliaspay.tld", "changeddata12a", "privdata2");
	AliasUpdate("node3", "alias3.aliaspay.tld", "changeddata13a", "privdata3");

	// update aliases afterwards, there should be MAX_ALIAS_UPDATES_PER_BLOCK UTXOs again after update
	// alias1 was only funded with 10 sys which gets used in the 5th update, 1 was used above in aliasupdate, while alias2/alias3 have more fund utxos so they can do more updates without a block
	for (unsigned int i = 0; i<MAX_ALIAS_UPDATES_PER_BLOCK-1; i++)
		BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg alias1.aliaspay.tld changedata1"));
	for (unsigned int i = 0; i<MAX_ALIAS_UPDATES_PER_BLOCK; i++)
		BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg alias2.aliaspay.tld changedata2"));
	for (unsigned int i = 0; i<MAX_ALIAS_UPDATES_PER_BLOCK; i++)
		BOOST_CHECK_NO_THROW(CallRPC("node3", "aliasupdate sysrates.peg alias3.aliaspay.tld changedata3"));
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node3");
}
BOOST_AUTO_TEST_CASE (generate_alias_offerexpiry_resync)
{
	printf("Running generate_offer_aliasexpiry_resync...\n");
	UniValue r;
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	// change offer to an older alias, expire the alias and ensure that on resync the offer seems to be expired still
	AliasNew("node1", "aliasold", "changeddata1");
	printf("Sleeping 5 seconds between the creation of the two aliases for this test...\n");
	MilliSleep(5000);
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	GenerateBlocks(50);
	AliasNew("node1", "aliasnew", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo aliasold"));
	int64_t aliasoldexpiry = find_value(r.get_obj(), "expires_on").get_int64();	
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo aliasnew"));
	int64_t aliasnewexpiry = find_value(r.get_obj(), "expires_on").get_int64();	
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	int64_t mediantime = find_value(r.get_obj(), "mediantime").get_int64();	
	BOOST_CHECK(aliasoldexpiry > mediantime);
	BOOST_CHECK(aliasoldexpiry < aliasnewexpiry);
	StopNode("node3");
	GenerateBlocks(5, "node2");

	string offerguid = OfferNew("node1", "aliasnew", "category", "title", "1", "0.05", "description", "USD");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	
	BOOST_CHECK_EQUAL(aliasnewexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());
	OfferUpdate("node1", "aliasold", offerguid, "category", "title", "1", "0.05", "description", "USD");
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(aliasoldexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "alias").get_str(), "aliasold");	
	
	ExpireAlias("aliasold");
	GenerateBlocks(1, "node1");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	mediantime = find_value(r.get_obj(), "mediantime").get_int64();	
	BOOST_CHECK(aliasoldexpiry <= mediantime);
	BOOST_CHECK(aliasnewexpiry > mediantime);

	StopNode("node1");
	StartNode("node1");

	// aliasnew should still be active, but offer was set to aliasold so it should be expired
	ExpireAlias("aliasold");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	mediantime = find_value(r.get_obj(), "mediantime").get_int64();	
	BOOST_CHECK(aliasoldexpiry <= mediantime);
	BOOST_CHECK(aliasnewexpiry > mediantime);

	BOOST_CHECK_THROW(r = CallRPC("node1", "aliasinfo aliasold"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerinfo " + offerguid), runtime_error);

	StartNode("node3");
	ExpireAlias("aliasold");
	GenerateBlocks(5, "node3");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	mediantime = find_value(r.get_obj(), "mediantime").get_int64();	
	BOOST_CHECK(aliasoldexpiry <= mediantime);
	BOOST_CHECK(aliasnewexpiry > mediantime);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo aliasnew"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);	
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo aliasnew"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo aliasnew"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);


	// node 3 doesn't download the offer since it expired while node 3 was offline
	BOOST_CHECK_THROW(r = CallRPC("node3", "offerinfo " + offerguid), runtime_error);
	BOOST_CHECK_EQUAL(OfferFilter("node3", offerguid), false);

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);	
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "alias").get_str(), "aliasold");	
	BOOST_CHECK_EQUAL(aliasoldexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());

}
BOOST_AUTO_TEST_CASE (generate_aliastransfer)
{
	printf("Running generate_aliastransfer...\n");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	UniValue r;
	AliasNew("node1", "jagnode1", "changeddata1");
	AliasNew("node2", "jagnode2", "changeddata2");

	string hex_str = AliasTransfer("node1", "jagnode1", "node2", "changeddata1", "pvtdata");
	BOOST_CHECK(hex_str.empty());

	// xfer an alias that isn't yours
	hex_str = AliasTransfer("node1", "jagnode1", "node2", "changedata1", "pvtdata1");
	BOOST_CHECK(!hex_str.empty());

	// xfer alias and update it at the same time
	hex_str = AliasTransfer("node2", "jagnode2", "node3", "changeddata4", "pvtdata");
	BOOST_CHECK(hex_str.empty());
	// update xferred alias
	hex_str = AliasUpdate("node2", "jagnode1", "changeddata5", "pvtdata1");
	BOOST_CHECK(hex_str.empty());
	// rexfer alias
	hex_str = AliasTransfer("node2", "jagnode1", "node3", "changeddata5", "pvtdata2");
	BOOST_CHECK(hex_str.empty());
	// xfer an alias to another alias is prohibited
	hex_str = AliasTransfer("node2", "jagnode1", "node1", "changeddata5", "pvtdata2");
	BOOST_CHECK(!hex_str.empty());
}
BOOST_AUTO_TEST_CASE (generate_aliasbalance)
{
	printf("Running generate_aliasbalance...\n");
	UniValue r;
	// create alias and check balance is 0
	AliasNew("node2", "jagnodebalance1", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasbalance jagnodebalance1"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - 10 * COIN) < 0.1*COIN);

	// send money to alias and check balance is updated
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 1.5"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 1.6"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 1"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 2"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 3"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 4"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 5"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 2"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 9"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 11"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 10.45"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 10"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 20"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasbalance jagnodebalance1"));
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	balanceBefore += 80.55*COIN;
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);

	// edit and see balance is same
	string hex_str = AliasUpdate("node2", "jagnodebalance1", "pubdata1", "privdata1");
	BOOST_CHECK(hex_str.empty());
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance jagnodebalance1"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore -  balanceAfter) < COIN);
	GenerateBlocks(5);
	ExpireAlias("jagnodebalance1");
	// renew alias, should clear balance
	AliasNew("node2", "jagnodebalance1", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance jagnodebalance1"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceAfter - 10 * COIN) < 0.1*COIN);
}
BOOST_AUTO_TEST_CASE (generate_aliasbalancewithtransfer)
{
	printf("Running generate_aliasbalancewithtransfer...\n");
	UniValue r;
	AliasNew("node2", "jagnodebalance2", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasbalance jagnodebalance2"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - 10 * COIN) < 0.1*COIN);

	// send money to alias and check balance

	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance2 9"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasbalance jagnodebalance2"));
	balanceBefore += 9*COIN;
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);

	// transfer alias to someone else and balance should be same
	string hex_str = AliasTransfer("node2", "jagnodebalance2", "node3", "changeddata4", "pvtdata");
	BOOST_CHECK(hex_str.empty());
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance jagnodebalance2"));
	CAmount balanceAfterTransfer = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(balanceAfterTransfer >= (balanceBefore-COIN));

	// send money to alias and balance updates
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance2 12.1"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasbalance jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceAfter, 12.1*COIN+balanceAfterTransfer);

	// edit and balance should remain the same
	hex_str = AliasUpdate("node3", "jagnodebalance2", "pubdata1", "privdata1");
	BOOST_CHECK(hex_str.empty());
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs((12.1*COIN+balanceAfterTransfer) -  balanceAfter) < COIN);

	// transfer again and balance is same
	hex_str = AliasTransfer("node3", "jagnodebalance2", "node2", "changeddata4", "pvtdata");
	BOOST_CHECK(hex_str.empty());
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasbalance jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(balanceAfter >= (12.1*COIN+balanceAfterTransfer)-COIN);

}
BOOST_AUTO_TEST_CASE (generate_multisigalias)
{
	printf("Running generate_multisigalias...\n");
	UniValue r;
	AliasNew("node1", "jagnodemultisig1", "changeddata1");
	AliasNew("node2", "jagnodemultisig2", "changeddata1");
	AliasNew("node3", "jagnodemultisig3", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	// used for when setting multisig alias back to normal alias
	string oldAddressStr = find_value(r, "address").get_str();
	UniValue arrayParams(UniValue::VARR);
	UniValue arrayOfKeys(UniValue::VARR);

	// create 2 of 2
	UniValue resCreate;
	string redeemScript, addressStr;
	BOOST_CHECK_NO_THROW(resCreate = CallRPC("node1", "createmultisig 2 \"[\\\"jagnodemultisig1\\\",\\\"jagnodemultisig2\\\"]\""));	
	UniValue redeemScript_value = find_value(resCreate, "redeemScript");
	UniValue address_value = find_value(resCreate, "address");
	BOOST_CHECK(redeemScript_value.isStr());
	BOOST_CHECK(address_value.isStr());
	redeemScript = redeemScript_value.get_str();
	addressStr = address_value.get_str();
		
	string tmp = AliasUpdate("node1", "jagnodemultisig1", "pubdata0", "privdata", "\"\"", addressStr);
	BOOST_CHECK_EQUAL(tmp, "");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	// change the multisigs pw and public data
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasaddscript " + redeemScript));
	
	string hex_str = AliasUpdate("node1", "jagnodemultisig1", "pubdata1", "privdata1", "\"\"", addressStr);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	// ensure data did not change because its 2 of 2 and we only got 1 sig so far
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), "pubdata0");
	BOOST_CHECK(!hex_str.empty());
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasaddscript " + redeemScript));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "syscoinsignrawtransaction " + hex_str));
	GenerateBlocks(5, "node2");
	GenerateBlocks(5);
	// pay to multisig and check balance
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	// make sure alias was updated with new public data
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), "pubdata1");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodemultisig1 9"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasbalance jagnodemultisig1"));
	CAmount balanceBefore = 19*COIN;
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK(abs(balanceBefore - balanceAfter) < COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	hex_str = AliasUpdate("node2", "jagnodemultisig1", "\"\"", "\"\"", "\"\"", addressStr);
	BOOST_CHECK(hex_str != "");

	// create 1 of 2
	BOOST_CHECK_NO_THROW(resCreate = CallRPC("node1", "createmultisig 1 \"[\\\"jagnodemultisig1\\\",\\\"jagnodemultisig2\\\"]\""));	
	redeemScript_value = find_value(resCreate, "redeemScript");
	address_value = find_value(resCreate, "address");
	BOOST_CHECK(redeemScript_value.isStr());
	BOOST_CHECK(address_value.isStr());
	redeemScript = redeemScript_value.get_str();
	addressStr = address_value.get_str();
	hex_str = AliasUpdate("node1", "jagnodemultisig1", "pubdata", "privdata", "\"\"", addressStr);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "syscoinsignrawtransaction " + hex_str));
	GenerateBlocks(5, "node2");
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	// ensure only one signature is needed
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasaddscript " + redeemScript));
	hex_str = AliasUpdate("node1", "jagnodemultisig1");
	BOOST_CHECK_EQUAL(hex_str, "");
	// pay to multisig and check balance
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodemultisig1 8"), runtime_error);
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasbalance jagnodemultisig1"));
	balanceBefore += 8*COIN;
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - balanceAfter) < COIN);
	// create 2 of 3
	BOOST_CHECK_NO_THROW(resCreate = CallRPC("node1", "createmultisig 2 \"[\\\"jagnodemultisig1\\\",\\\"jagnodemultisig2\\\", \\\"jagnodemultisig3\\\"]\""));	
	redeemScript_value = find_value(resCreate, "redeemScript");
	address_value = find_value(resCreate, "address");
	BOOST_CHECK(redeemScript_value.isStr());
	BOOST_CHECK(address_value.isStr());
	redeemScript = redeemScript_value.get_str();
	addressStr = address_value.get_str();
	tmp = AliasUpdate("node1", "jagnodemultisig1", "pubdata", "privdata", "\"\"", addressStr);
	BOOST_CHECK_EQUAL(tmp, "");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	// 2 sigs needed, remove redeemScript to make it a normal alias
	BOOST_CHECK_NO_THROW(CallRPC("node3", "aliasaddscript " + redeemScript));
	hex_str = AliasUpdate("node3", "jagnodemultisig1", "\"\"", "\"\"", "\"\"", oldAddressStr);
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasaddscript " + redeemScript));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsignrawtransaction " + hex_str));
	GenerateBlocks(5, "node3");
	GenerateBlocks(5);
	// pay to multisig and check balance
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), oldAddressStr);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodemultisig1 7"), runtime_error);
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasbalance jagnodemultisig1"));
	balanceBefore += 7*COIN;
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - balanceAfter) < COIN);

	// no multisig so update as normal
	hex_str = AliasUpdate("node2", "jagnodemultisig1");
	BOOST_CHECK(!hex_str.empty());
	hex_str = AliasUpdate("node3", "jagnodemultisig1");
	BOOST_CHECK(!hex_str.empty());
	hex_str = AliasUpdate("node1", "jagnodemultisig1");
	BOOST_CHECK_EQUAL(hex_str, "");
	// pay to multisig and check balance
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodemultisig1 6"), runtime_error);
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasbalance jagnodemultisig1"));
	balanceBefore += 6*COIN;
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - balanceAfter) < COIN);
}
BOOST_AUTO_TEST_CASE (generate_aliasbalancewithtransfermultisig)
{
	printf("Running generate_aliasbalancewithtransfermultisig...\n");
	// create 2 of 3 alias
	// send money to alias and check balance
	// transfer alias to non multisig  and balance should be 0
	// send money to alias and balance updates
	// edit and balance should remain the same
	// transfer again and balance is 0 again

}
BOOST_AUTO_TEST_CASE (generate_aliasexpiredbuyback)
{
	printf("Running generate_aliasexpiredbuyback...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	
	AliasNew("node1", "aliasexpirebuyback", "somedata", "data");
	// can't renew aliases that aren't expired
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback data"), runtime_error);
	ExpireAlias("aliasexpirebuyback");
	// expired aliases are searchable since you haven't deleted from indexer yet
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback"), true);
	
	// renew alias and its still searchable
	AliasNew("node1", "aliasexpirebuyback", "somedata1", "data1");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback"), true);

	ExpireAlias("aliasexpirebuyback");
	// try to renew alias again second time
	AliasNew("node1", "aliasexpirebuyback", "somedata2", "data2");
	// run the test with node3 offline to test pruning with renewing alias
	StopNode("node3");
	MilliSleep(500);
	AliasNew("node1", "aliasexpirebuyback1", "somedata1", "data1");
	GenerateBlocks(5, "node1");
	ExpireAlias("aliasexpirebuyback1");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback1"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback1"), false);

	StartNode("node3");
	ExpireAlias("aliasexpirebuyback1");
	GenerateBlocks(5, "node3");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback1"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback1"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node3", "aliasexpirebuyback1"), false);
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "aliasinfo aliasexpirebuyback1"), runtime_error);

	// run the test with node3 offline to test pruning with renewing alias twice
	StopNode("node3");
	AliasNew("node1", "aliasexpirebuyback2", "data", "data1");
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");
	ExpireAlias("aliasexpirebuyback2");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback2"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback2"), true);
	// renew second time
	AliasNew("node1", "aliasexpirebuyback2", "data2", "data2");
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");
	ExpireAlias("aliasexpirebuyback2");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback2"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback2"), true);
	StartNode("node3");
	ExpireAlias("aliasexpirebuyback2");
	GenerateBlocks(5, "node3");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback2"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback2"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node3", "aliasexpirebuyback2"), false);
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "aliasinfo aliasexpirebuyback2"), runtime_error);
	ExpireAlias("aliasexpirebuyback");
	// steal alias after expiry and original node try to recreate or update should fail
	AliasNew("node1", "aliasexpirebuyback", "somedata", "data");
	ExpireAlias("aliasexpirebuyback");
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node2");
	AliasNew("node2", "aliasexpirebuyback", "somedata", "data");
	BOOST_CHECK_THROW(CallRPC("node2", "aliasnew sysrates.peg aliasexpirebuyback data"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback data"), runtime_error);
	string hex_str = AliasUpdate("node2", "aliasexpirebuyback", "changedata1", "pvtdata");
	BOOST_CHECK(hex_str.empty());
	BOOST_CHECK_THROW(CallRPC("node2", "aliasnew sysrates.peg aliasexpirebuyback"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback"), runtime_error);
	// this time steal the alias and try to recreate at the same time
	ExpireAlias("aliasexpirebuyback");
	AliasNew("node1", "aliasexpirebuyback", "somedata", "data");
	hex_str = AliasUpdate("node1", "aliasexpirebuyback", "changedata3", "pvtdata3");
	BOOST_CHECK(hex_str.empty());
	ExpireAlias("aliasexpirebuyback");
	AliasNew("node2", "aliasexpirebuyback", "somedata", "data");
	hex_str = AliasUpdate("node2", "aliasexpirebuyback", "changedata4", "pvtdata4");
	BOOST_CHECK(hex_str.empty());
	GenerateBlocks(5,"node2");
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback data2"), runtime_error);
}

BOOST_AUTO_TEST_CASE (generate_aliaspruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes

	printf("Running generate_aliaspruning...\n");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	AliasNew("node1", "aliasprune", "pubdata", "privdata");
	GenerateBlocks(5, "node1");
	// we can find it as normal first
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune"), true);
	// then we let the service expire
	ExpireAlias("aliasprune");
	StartNode("node2");
	ExpireAlias("aliasprune");
	GenerateBlocks(5, "node2");
	// now we shouldn't be able to search it
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune"), false);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo aliasprune"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);

	// node2 shouldn't find the service at all (meaning node2 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node2", "aliasinfo aliasprune"), runtime_error);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune"), false);

	// stop node3
	StopNode("node3");
	// create a new service
	AliasNew("node1", "aliasprune1", "pubdata", "privdata");
	GenerateBlocks(5, "node1");
	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");
	// ensure you can still update before expiry
	string hex_str = AliasUpdate("node1", "aliasprune1", "newdata", "privdata");
	BOOST_CHECK(hex_str.empty());
	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1"), true);
	GenerateBlocks(5, "node1");
	// ensure service is still active since its supposed to expire at 100 blocks of non updated services
	hex_str = AliasUpdate("node1", "aliasprune1", "newdata1", "privdata1");
	BOOST_CHECK(hex_str.empty());
	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1"), true);
	ExpireAlias("aliasprune1");
	// now it should be expired
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasprune1 newdata2 " + HexStr(vchFromString("privatedata"))), runtime_error);

	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1"), false);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo aliasprune1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);

	StartNode("node3");
	ExpireAlias("aliasprune");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "aliasinfo aliasprune1"), runtime_error);
	BOOST_CHECK_EQUAL(AliasFilter("node3", "aliasprune1"), false);
}
BOOST_AUTO_TEST_CASE (generate_aliasprunewithoffer)
{
	printf("Running generate_aliasprunewithoffer...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	StopNode("node3");
	AliasNew("node1", "aliasprunewithoffer", "pubdata", "privdata");
	AliasNew("node1", "aliasprunewithoffer1", "pubdata", "privdata");
	AliasNew("node2", "aliasprunewithoffer2", "pubdata", "privdata");
	string offerguid = OfferNew("node1", "aliasprunewithoffer", "category", "title", "1", "0.05", "description", "SYS");
	string escrowguid = EscrowNew("node2", "node1", "aliasprunewithoffer2", offerguid, "1", "aliasprunewithoffer1", "aliasprunewithoffer");
	EscrowRelease("node2", "buyer", escrowguid);
	EscrowClaimRelease("node1", escrowguid);
	// last created alias should have furthest expiry
	ExpireAlias("aliasprunewithoffer2");
	StartNode("node3");
	ExpireAlias("aliasprunewithoffer2");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "escrowinfo " + escrowguid), runtime_error);
	BOOST_CHECK_EQUAL(EscrowFilter("node3", escrowguid), false);
}
BOOST_AUTO_TEST_CASE (generate_aliasprunewithcertoffer)
{
	printf("Running generate_aliasprunewithcertoffer...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	StopNode("node3");
	AliasNew("node1", "aliasprunewithcertoffer", "pubdata", "privdata");
	AliasNew("node2", "aliasprunewithcertoffer2", "pubdata", "privdata");
	string certguid = CertNew("node1", "aliasprunewithcertoffer", "jag1", "privdata", "pubdata");
	string certofferguid = OfferNew("node1", "aliasprunewithcertoffer", "certificates", "title", "1", "0.05", "description", "SYS", certguid);	
	string offerguid = OfferNew("node1", "aliasprunewithcertoffer", "category", "title", "1", "0.05", "description", "SYS");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress aliasprunewithcertoffer2 300"), runtime_error);
	GenerateBlocks(10);	
	OfferUpdate("node1", "aliasprunewithcertoffer", offerguid, "category", "titlenew", "1", "0.05", "descriptionnew", "USD");
	OfferUpdate("node1", "aliasprunewithcertoffer", certofferguid, "certificates", "titlenew", "1", "0.05", "descriptionnew", "USD", "false", certguid);
	OfferAccept("node1", "node2", "aliasprunewithcertoffer2", certofferguid, "1");
	OfferAccept("node1", "node2", "aliasprunewithcertoffer2", offerguid, "1");
	ExpireAlias("aliasprunewithcertoffer2");
	StartNode("node3");
	ExpireAlias("aliasprunewithcertoffer2");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "offerinfo " + offerguid), runtime_error);
	BOOST_CHECK_EQUAL(OfferFilter("node3", offerguid), false);
}

BOOST_AUTO_TEST_CASE (generate_aliasprunewithcert)
{
	printf("Running generate_aliasprunewithcert...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	StopNode("node3");
	AliasNew("node1", "aliasprunewithcert", "pubdata", "privdata");
	AliasNew("node2", "aliasprunewithcert2", "pubdata", "privdata");
	string certguid = CertNew("node1", "aliasprunewithcert", "jag1", "privdata", "pubdata");
	CertUpdate("node1", certguid, "\"\"", "\"\"", "newdata");
	CertTransfer("node1", "node2", certguid, "aliasprunewithcert2");
	GenerateBlocks(5, "node1");
	ExpireAlias("aliasprunewithcert2");
	StartNode("node3");
	ExpireAlias("aliasprunewithcert2");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "certinfo " + certguid), runtime_error);
	BOOST_CHECK_EQUAL(OfferFilter("node3", certguid), false);
}
BOOST_AUTO_TEST_CASE (generate_aliasexpired)
{
	printf("Running generate_aliasexpired...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "aliasexpire", "somedata");
	AliasNew("node1", "aliasexpire0", "somedata");
	AliasNew("node2", "aliasexpire1", "somedata");
	string aliasexpirenode2address = AliasNew("node2", "aliasexpirednode2", "somedata");
	string offerguid = OfferNew("node1", "aliasexpire0", "category", "title", "100", "0.01", "description", "USD");
	OfferAddWhitelist("node1", offerguid, "aliasexpirednode2", "5");
	string certguid = CertNew("node1", "aliasexpire", "certtitle", "privdata", "pubdata", "true");
	StopNode("node3");
	printf("Sleeping 5 seconds between the creation of aliases for this test...\n");
	MilliSleep(5000);	
	GenerateBlocks(10);	
	AliasNew("node1", "aliasexpire2", "pubdata", "privdata");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress aliasexpirednode2 300"), runtime_error);
	GenerateBlocks(10);	
	string escrowguid = EscrowNew("node2", "node1", "aliasexpirednode2", offerguid, "1", "aliasexpire", "aliasexpire0", "5");
	string aliasexpire2node2address = AliasNew("node2", "aliasexpire2node2", "pubdata", "privdata");
	string certgoodguid = CertNew("node1", "aliasexpire2", "certtitle", "privdata", "pubdata");
	ExpireAlias("aliasexpirednode2");
	GenerateBlocks(5, "node2");
	
	AliasNew("node1", "aliasexpire", "pubdata", "privdata");
	AliasNew("node1", "aliasexpire0", "pubdata", "privdata");
	string aliasexpire1address =  AliasNew("node2", "aliasexpire1", "pubdata", "privdata");
	// should already exist and not be expired
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpire2"), runtime_error);
	CKey privKey;
	privKey.MakeNewKey(true);
	CPubKey pubKey = privKey.GetPubKey();
	vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
	vector<unsigned char> vchPrivKey(privKey.begin(), privKey.end());	
	BOOST_CHECK(pubKey.IsFullyValid());
	CSyscoinAddress aliasAddress(pubKey.GetID());
	// should fail: alias update on expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasexpirednode2 newdata1"), runtime_error);
	// should fail: alias transfer from expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasexpirednode2 changedata1 \"\" \"\" " + aliasAddress.ToString()), runtime_error);
	// should fail: alias transfer to another non-expired alias address
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire2 changedata1 \"\" \"\" " + aliasexpire1address), runtime_error);

	// should fail: link to an expired alias in offer
	BOOST_CHECK_THROW(CallRPC("node2", "offerlink aliasexpirednode2 " + offerguid + " 5 newdetails"), runtime_error);
	// should fail: generate an offer using expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "offernew aliasexpirednode2 category title 1 0.05 description USD"), runtime_error);

	// should fail: new escrow with expired arbiter alias
	BOOST_CHECK_THROW(CallRPC("node2", "escrownew aliasexpire2node2 " + offerguid + " 1 " + " aliasexpirednode2"), runtime_error);
	// should fail: new escrow with expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "escrownew aliasexpirednode2 " + offerguid + " 1 " + " aliasexpire"), runtime_error);

	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire newdata1"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire2 newdata1"));
	GenerateBlocks(5, "node1");
	
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + certgoodguid + " titlenew privdata pubdata"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate aliasexpire0 " + offerguid + " category title 100 0.05 description"));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + certguid + " jag1 data pubdata"));
	GenerateBlocks(5, "node1");

	StartNode("node3");
	ExpireAlias("aliasexpirednode2");
	
	GenerateBlocks(5, "node3");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node1");
	// since all aliases are expired related to that escrow, the escrow was pruned
	BOOST_CHECK_THROW(CallRPC("node3", "escrowinfo " + escrowguid), runtime_error);
	// and node2
	BOOST_CHECK_NO_THROW(CallRPC("node2", "escrowinfo " + escrowguid));
	// this will recreate the alias and give it a new pubkey.. we need to use the old pubkey to sign the multisig, the escrow rpc call must check for the right pubkey
	BOOST_CHECK(aliasexpirenode2address != AliasNew("node2", "aliasexpirednode2", "somedata"));
	CertUpdate("node1", certgoodguid, "pubdata", "newdata");
	// able to release and claim release on escrow with non-expired aliases with new pubkeys
	EscrowRelease("node2", "buyer", escrowguid);	 
	EscrowClaimRelease("node1", escrowguid); 

	// should cleanup db for node1 and remove aliasexpirenode2address from alias address db
	ExpireAlias("aliasexpirednode2");
	GenerateBlocks(5);

	BOOST_CHECK_NO_THROW(AliasNew("node1", "aliasexpire2", "somedata"));
	// should pass: alias transfer to another expired alias address
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire2 changedata1 \"\" \"\" " + aliasexpire2node2address));
	GenerateBlocks(5);
	BOOST_CHECK(aliasexpirenode2address != AliasNew("node2", "aliasexpirednode2", "somedata"));

	// should fail: cert alias not owned by node1
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certtransfer " + certgoodguid + " aliasexpirednode2"));
	const UniValue& resArray = r.get_array();
	if (resArray.size() > 1)
	{
		const UniValue& complete_value = resArray[1];
		bool bComplete = false;
		if (complete_value.isStr())
			bComplete = complete_value.get_str() == "true";
		BOOST_CHECK(!bComplete);
	}
	ExpireAlias("aliasexpire2");
	// should fail: update cert with expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + certgoodguid + " title pubdata"), runtime_error);
	// should fail: xfer an cert with expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + certgoodguid + " aliasexpire2"), runtime_error);
	// should fail: xfer an cert to an expired alias even though transferring cert is good
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + certguid + " aliasexpire2"), runtime_error);

	AliasNew("node2", "aliasexpire2", "somedata");

	ExpireAlias("aliasexpire2");
	AliasNew("node2", "aliasexpirednode2", "somedataa");
	AliasNew("node1", "aliasexpire2", "somedatab");
	// should pass: confirm that the transferring cert is good by transferring to a good alias
	CertTransfer("node1", "node2", certgoodguid, "aliasexpirednode2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certgoodguid));
	// ensure it got transferred
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "alias").get_str(), "aliasexpirednode2");

	ExpireAlias("aliasexpire2");
	// should fail: generate a cert using expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "certnew aliasexpire2 jag1 data pubdata"), runtime_error);
	// renew alias after expiry
	AliasNew("node2", "aliasexpirednode2", "somedata");
}
BOOST_AUTO_TEST_SUITE_END ()