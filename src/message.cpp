#include "message.h"
#include "alias.h"
#include "cert.h"
#include "init.h"
#include "main.h"
#include "util.h"
#include "random.h"
#include "base58.h"
#include "core_io.h"
#include "rpc/server.h"
#include "wallet/wallet.h"
#include "chainparams.h"
#include "coincontrol.h"
#include "messagecrypter.h"
#include <boost/algorithm/hex.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <functional> 
#include <boost/range/adaptor/reversed.hpp>
using namespace std;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const vector<unsigned char> &vchAliasPeg, const string &currencyCode, const CRecipient &aliasRecipient, const CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool useOnlyAliasPaymentToFund=true, bool transferAlias=false);
void PutToMessageList(std::vector<CMessage> &messageList, CMessage& index) {
	int i = messageList.size() - 1;
	BOOST_REVERSE_FOREACH(CMessage &o, messageList) {
        if(index.nHeight != 0 && o.nHeight == index.nHeight) {
        	messageList[i] = index;
            return;
        }
        else if(!o.txHash.IsNull() && o.txHash == index.txHash) {
        	messageList[i] = index;
            return;
        }
        i--;
	}
    messageList.push_back(index);
}
bool IsMessageOp(int op) {
    return op == OP_MESSAGE_ACTIVATE;
}

uint64_t GetMessageExpiration(const CMessage& message) {
	uint64_t nTime = chainActive.Tip()->nTime + 1;
	CAliasUnprunable aliasUnprunable;
	if (paliasdb && paliasdb->ReadAliasUnprunable(message.vchAliasTo, aliasUnprunable) && !aliasUnprunable.IsNull())
		nTime = aliasUnprunable.nExpireTime;
	
	return nTime;
}


string messageFromOp(int op) {
    switch (op) {
    case OP_MESSAGE_ACTIVATE:
        return "messageactivate";
    default:
        return "<unknown message op>";
    }
}
bool CMessage::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
        CDataStream dsMessage(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsMessage >> *this;
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	vector<unsigned char> vchMsgData ;
	Serialize(vchMsgData);
	const uint256 &calculatedHash = Hash(vchMsgData.begin(), vchMsgData.end());
	const vector<unsigned char> &vchRandMsg = vchFromValue(calculatedHash.GetHex());
	if(vchRandMsg != vchHash)
	{
		SetNull();
        return false;
	}
	return true;
}
bool CMessage::UnserializeFromTx(const CTransaction &tx) {
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nOut))
	{
		SetNull();
		return false;
	}
	if(!UnserializeFromData(vchData, vchHash))
	{
		return false;
	}
    return true;
}
void CMessage::Serialize(vector<unsigned char>& vchData) {
    CDataStream dsMessage(SER_NETWORK, PROTOCOL_VERSION);
    dsMessage << *this;
	vchData = vector<unsigned char>(dsMessage.begin(), dsMessage.end());

}
bool CMessageDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	vector<CMessage> vtxPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "messagei") {
            	const vector<unsigned char> &vchMyMessage= key.second;         
				pcursor->GetValue(vtxPos);	
				if (vtxPos.empty()){
					servicesCleaned++;
					EraseMessage(vchMyMessage);
					pcursor->Next();
					continue;
				}
				const CMessage &txPos = vtxPos.back();
  				if (chainActive.Tip()->nTime >= GetMessageExpiration(txPos))
				{
					servicesCleaned++;
					EraseMessage(vchMyMessage);
				} 
				
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}

int IndexOfMessageOutput(const CTransaction& tx) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return -1;
    vector<vector<unsigned char> > vvch;
	int op;
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		// find an output you own
		if (pwalletMain->IsMine(out) && DecodeMessageScript(out.scriptPubKey, op, vvch)) {
			return i;
		}
	}
	return -1;
}


bool GetTxOfMessage(const vector<unsigned char> &vchMessage,
        CMessage& txPos, CTransaction& tx) {
    vector<CMessage> vtxPos;
    if (!pmessagedb->ReadMessage(vchMessage, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
    if (chainActive.Tip()->nTime >= GetMessageExpiration(txPos)) {
        string message = stringFromVch(vchMessage);
        LogPrintf("GetTxOfMessage(%s) : expired", message.c_str());
        return false;
    }

    if (!GetSyscoinTransaction(nHeight, txPos.txHash, tx, Params().GetConsensus()))
        return error("GetTxOfMessage() : could not read tx from disk");

    return true;
}
bool DecodeAndParseMessageTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch)
{
	CMessage message;
	bool decode = DecodeMessageTx(tx, op, nOut, vvch);
	bool parse = message.UnserializeFromTx(tx);
	return decode && parse;
}
bool DecodeMessageTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeMessageScript(out.scriptPubKey, op, vvchRead)) {
            nOut = i; found = true; vvch = vvchRead;
            break;
        }
    }
	if (!found) vvch.clear();
    return found;
}

bool DecodeMessageScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch, CScript::const_iterator& pc) {
    opcodetype opcode;
	vvch.clear();
	if (!script.GetOp(pc, opcode)) return false;
	if (opcode < OP_1 || opcode > OP_16) return false;
    op = CScript::DecodeOP_N(opcode);
	bool found = false;
	for (;;) {
		vector<unsigned char> vch;
		if (!script.GetOp(pc, opcode, vch))
			return false;
		if (opcode == OP_DROP || opcode == OP_2DROP)
		{
			found = true;
			break;
		}
		if (!(opcode >= 0 && opcode <= OP_PUSHDATA4))
			return false;
		vvch.push_back(vch);
	}

	// move the pc to after any DROP or NOP
	while (opcode == OP_DROP || opcode == OP_2DROP) {
		if (!script.GetOp(pc, opcode))
			break;
	}

	pc--;
	return found && IsMessageOp(op);
}

bool DecodeMessageScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeMessageScript(script, op, vvch, pc);
}

bool RemoveMessageScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeMessageScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}

bool CheckMessageInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add message in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug)
		LogPrintf("*** MESSAGE %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");
    const COutPoint *prevOutput = NULL;
    const CCoins *prevCoins;

	int prevAliasOp = 0;
	if (tx.nVersion != SYSCOIN_TX_VERSION)
	{
		errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3000 - " + _("Non-Syscoin transaction found");
		return true;
	}
	// unserialize msg from txn, check for valid
	CMessage theMessage;
	CAliasIndex alias;
	CTransaction aliasTx;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || !theMessage.UnserializeFromData(vchData, vchHash))
	{
		errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR ERRCODE: 3001 - " + _("Cannot unserialize data inside of this transaction relating to a message");
		return true;
	}

    vector<vector<unsigned char> > vvchPrevAliasArgs;
	if(fJustCheck)
	{	
		if(vvchArgs.size() != 2)
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3002 - " + _("Message arguments incorrect size");
			return error(errorMessage.c_str());
		}
		if(!theMessage.IsNull())
		{
			if(vvchArgs.size() <= 1 || vchHash != vvchArgs[1])
			{
				errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3003 - " + _("Hash provided doesn't match the calculated hash of the data");
				return true;
			}
		}
		

		// Strict check - bug disallowed
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			vector<vector<unsigned char> > vvch;
			int pop;
			prevOutput = &tx.vin[i].prevout;
			if(!prevOutput)
				continue;
			// ensure inputs are unspent when doing consensus check to add to block
			prevCoins = inputs.AccessCoins(prevOutput->hash);
			if(prevCoins == NULL)
				continue;
			if(!prevCoins->IsAvailable(prevOutput->n) || !IsSyscoinScript(prevCoins->vout[prevOutput->n].scriptPubKey, pop, vvch))
				continue;
			if (IsAliasOp(pop, true) && vvch.size() >= 4 && vvch[3].empty())
			{
				prevAliasOp = pop;
				vvchPrevAliasArgs = vvch;
				break;
			}
		}	
	}

    // unserialize message UniValue from txn, check for valid
   
	string retError = "";
	if(fJustCheck)
	{
		if (vvchArgs.empty() || vvchArgs[0].size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3004 - " + _("Message transaction guid too big");
			return error(errorMessage.c_str());
		}
		if(theMessage.vchData.size() > MAX_ENCRYPTED_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3006 - " + _("Message too long");
			return error(errorMessage.c_str());
		}
		if(theMessage.vchPubData.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3006 - " + _("Message details too long");
			return error(errorMessage.c_str());
		}
		if(theMessage.vchEncryptionPrivateKeyFrom.size() > MAX_ENCRYPTED_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3006 - " + _("Encrypted private from key too long");
			return error(errorMessage.c_str());
		}
		if(theMessage.vchEncryptionPrivateKeyTo.size() > MAX_ENCRYPTED_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3006 - " + _("Encryption private to key too long");
			return error(errorMessage.c_str());
		}
		if(theMessage.vchEncryptionPublicKey.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3006 - " + _("Encryption public key too long");
			return error(errorMessage.c_str());
		}
		if(!IsValidAliasName(theMessage.vchAliasFrom))
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3009 - " + _("Alias name does not follow the domain name specification");
			return error(errorMessage.c_str());
		}
		if(!IsValidAliasName(theMessage.vchAliasTo))
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3010 - " + _("Alias name does not follow the domain name specification");
			return error(errorMessage.c_str());
		}
		if(op == OP_MESSAGE_ACTIVATE)
		{
			if(!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty() || theMessage.vchAliasFrom != vvchPrevAliasArgs[0])
			{
				errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3011 - " + _("Alias not provided as input");
				return error(errorMessage.c_str());
			}
			if (theMessage.vchMessage != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3012 - " + _("Message guid mismatch");
				return error(errorMessage.c_str());
			}

		}
		else{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3013 - " + _("Message transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	// save serialized message for later use
	CMessage serializedMessage = theMessage;


    if (!fJustCheck ) {
		vector<CAliasIndex> vtxAlias;
		bool isExpired = false;
		if(!GetVtxOfAlias(theMessage.vchAliasTo, alias, vtxAlias, isExpired))
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3014 - " + _("Cannot find alias for the recipient of this message. It may be expired");
			return true;
		}

		vector<CMessage> vtxPos;
		if (pmessagedb->ExistsMessage(vvchArgs[0])) {
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3016 - " + _("This message already exists");
			return true;
		}      
        // set the message's txn-dependent values
		theMessage.txHash = tx.GetHash();
		theMessage.nHeight = nHeight;
		PutToMessageList(vtxPos, theMessage);
        // write message  
		if(!dontaddtodb && !pmessagedb->WriteMessage(vvchArgs[0], vtxPos))
		{
			errorMessage = "SYSCOIN_MESSAGE_CONSENSUS_ERROR: ERRCODE: 3016 - " + _("Failed to write to message DB");
            return error(errorMessage.c_str());
		}
      			
        // debug
		if(fDebug)
			LogPrintf( "CONNECTED MESSAGE: op=%s message=%s hash=%s height=%d\n",
                messageFromOp(op).c_str(),
                stringFromVch(vvchArgs[0]).c_str(),
                tx.GetHash().ToString().c_str(),
                nHeight);
	}
    return true;
}

UniValue messagenew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 7 || params.size() > 8)
        throw runtime_error(
		"messagenew <message> <publicdata> <fromalias> <toalias> <encryption_publickey> <encryption_privatekey_from> <encryption_privatekey_to> [witness]\n"
						"<message> Message to send. Encrypted to encryption_publickey.\n"
						"<publicdata> Public message data including title and encryption keys for group viewing access.\n"
						"<fromalias> Alias to send message from.\n"
						"<toalias> Alias to send message to.\n"	
						"<encryption_publickey> Encryption public key. Message is encrypted to this key, anyone who has access to private key can read message.\n"	
						"<encryption_privatekey_from> Encrypted private key to fromalias used for encryption/decryption of this message. Should be encrypted to encryption_publickey of fromalias.\n"	
						"<encryption_privatekey_to> Encrypted private key to toalias used for encryption/decryption of this message. Should be encrypted to encryption_publickey of toalias.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());
	string strMessage = params[0].get_str();
	string strPubData = params[1].get_str();
	string strFromAddress = params[2].get_str();
	boost::algorithm::to_lower(strFromAddress);
	string strToAddress = params[3].get_str();
	boost::algorithm::to_lower(strToAddress);

	string strEncryptionPublicKey = params[4].get_str();
	string strEncryptionPrivateKeyFrom = params[5].get_str();
	string strEncryptionPrivateKeyTo = params[6].get_str();
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 7))
		vchWitness = vchFromValue(params[7]);
	CAliasIndex aliasFrom, aliasTo;
	CTransaction aliastx;
	if (!GetTxOfAlias(vchFromString(strFromAddress), aliasFrom, aliastx))
		throw runtime_error("SYSCOIN_MESSAGE_RPC_ERROR: ERRCODE: 3500 - " + _("Could not find an alias with this name"));
	CScript scriptPubKeyAliasOrig, scriptPubKeyAlias, scriptPubKeyOrig, scriptPubKey;
	CSyscoinAddress fromAddr;
	GetAddress(aliasFrom, &fromAddr, scriptPubKeyAliasOrig);

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << aliasFrom.vchAlias <<  aliasFrom.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;		


	if(!GetTxOfAlias(vchFromString(strToAddress), aliasTo, aliastx))
	{
		throw runtime_error("SYSCOIN_MESSAGE_RPC_ERROR: ERRCODE: 3503 - " + _("Failed to read to alias from alias DB"));
	}
	CSyscoinAddress toAddr;
	GetAddress(aliasTo, &toAddr, scriptPubKeyOrig);


    // gather inputs
	vector<unsigned char> vchMessage = vchFromString(GenerateSyscoinGuid());
    // this is a syscoin transaction
    CWalletTx wtx;

    // build message
    CMessage newMessage;
	newMessage.vchMessage = vchMessage;
	newMessage.vchData = ParseHex(strMessage);
	newMessage.vchPubData = vchFromString(strPubData);
	newMessage.vchAliasFrom = aliasFrom.vchAlias;
	newMessage.vchAliasTo = aliasTo.vchAlias;
	newMessage.nHeight = chainActive.Tip()->nHeight;
	newMessage.vchEncryptionPublicKey = ParseHex(strEncryptionPublicKey);
	newMessage.vchEncryptionPrivateKeyFrom = ParseHex(strEncryptionPrivateKeyFrom);
	newMessage.vchEncryptionPrivateKeyTo = ParseHex(strEncryptionPrivateKeyTo);

	vector<unsigned char> data;
	newMessage.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashMessage = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_MESSAGE_ACTIVATE) << vchMessage << vchHashMessage << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;

	// send the tranasction
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyAliasOrig, aliasFrom.vchAlias, aliasFrom.vchAliasPeg, chainActive.Tip()->nHeight, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, aliasFrom.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);
	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(aliasFrom.vchAlias, vchWitness, aliasFrom.vchAliasPeg, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";
	string txid_str = "";
	const UniValue& hex_value = find_value(so, "hex");
	const UniValue& txid_value = find_value(so, "txid");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	if (txid_value.isStr())
		txid_str = txid_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
	{
		res.push_back(txid_str);
		res.push_back(stringFromVch(vchMessage));
	}
	else
	{
		res.push_back(hex_str);
		res.push_back(stringFromVch(vchMessage));
		res.push_back("false");
	}
	return res;
}

UniValue messageinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 > params.size() || 2 < params.size())
        throw runtime_error("messageinfo <guid> [walletless=false]\n"
                "Show stored values of a single message.\n");

    vector<unsigned char> vchMessage = vchFromValue(params[0]);
	string strWalletless = "false";
	if(CheckParam(params, 1))
		strWalletless = params[1].get_str();

	vector<CMessage> vtxPos;

    UniValue oMessage(UniValue::VOBJ);
    vector<unsigned char> vchValue;

	if (!pmessagedb->ReadMessage(vchMessage, vtxPos) || vtxPos.empty())
		throw runtime_error("SYSCOIN_MESSAGE_RPC_ERROR: ERRCODE: 3506 - " + _("Failed to read from message DB"));

	if(!BuildMessageJson(vtxPos.back(), oMessage, strWalletless))
		throw runtime_error("SYSCOIN_MESSAGE_RPC_ERROR: ERRCODE: 3507 - " + _("Could not find this message"));

    return oMessage;
}

UniValue messagereceivelist(const UniValue& params, bool fHelp) {
    if (fHelp || 5 < params.size())
        throw runtime_error("messagereceivelist [\"alias\",...] [<message>] [walletless=false] [count] [from]\n"
                "list messages that an array of aliases has recieved. Set of aliases to look up based on alias.");
	UniValue aliasesValue(UniValue::VARR);
	vector<string> aliases;
	if(CheckParam(params, 0))
	{
		if(params[0].isArray())
		{
			aliasesValue = params[0].get_array();
			for(unsigned int aliasIndex =0;aliasIndex<aliasesValue.size();aliasIndex++)
			{
				string lowerStr = aliasesValue[aliasIndex].get_str();
				boost::algorithm::to_lower(lowerStr);
				if(!lowerStr.empty())
					aliases.push_back(lowerStr);
			}
		}
	}
	vector<unsigned char> vchNameUniq;
    if(CheckParam(params, 1))
        vchNameUniq = vchFromValue(params[1]);

	string strWalletless = "false";
	if(CheckParam(params, 2))
		strWalletless = params[2].get_str();

	int count = 10;
	int from = 0;
	if (CheckParam(params, 3))
		count = atoi(params[3].get_str());
	if (CheckParam(params, 4))
		from = atoi(params[4].get_str());
	int found = 0;

	UniValue oRes(UniValue::VARR);
	map< vector<unsigned char>, int > vNamesI;
	map< vector<unsigned char>, UniValue > vNamesO;
	if(aliases.size() > 0)
	{
		for(unsigned int aliasIndex =0;aliasIndex<aliases.size();aliasIndex++)
		{
			if (oRes.size() >= count)
				break;
			const string &name = aliases[aliasIndex];
			const vector<unsigned char> &vchAlias = vchFromString(name);
			vector<CAliasIndex> vtxPos;
			if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
				continue;
			CTransaction tx;
			for(auto& it : boost::adaptors::reverse(vtxPos)) {
				if (oRes.size() >= count)
					break;
				const CAliasIndex& theAlias = it;
				if(!GetSyscoinTransaction(theAlias.nHeight, theAlias.txHash, tx, Params().GetConsensus()))
					continue;
				CMessage message(tx);
				if(!message.IsNull())
				{
					if (vNamesI.find(message.vchMessage) != vNamesI.end())
						continue;
					if (vchNameUniq.size() > 0 && vchNameUniq != message.vchMessage)
						continue;
					vector<CMessage> vtxMessagePos;
					if (!pmessagedb->ReadMessage(message.vchMessage, vtxMessagePos) || vtxMessagePos.empty())
						continue;
					const CMessage &theMessage = vtxMessagePos.back();
					if(theMessage.vchAliasTo != theAlias.vchAlias)
						continue;
					
					UniValue oMessage(UniValue::VOBJ);
					vNamesI[message.vchMessage] = theMessage.nHeight;
					found++;
					if (found < from)
						continue;
					if(BuildMessageJson(theMessage, oMessage, strWalletless))
					{
						oRes.push_back(oMessage);
					}
					// if finding specific GUID don't need to look any further
					if (vchNameUniq.size() > 0)
						return oRes;
				}	
			}
		}
	}
    return oRes;
}
bool BuildMessageJson(const CMessage& message, UniValue& oName, const string &strWalletless)
{
	oName.push_back(Pair("GUID", stringFromVch(message.vchMessage)));
	string sTime;
	CBlockIndex *pindex = chainActive[message.nHeight];
	if (pindex) {
		sTime = strprintf("%llu", pindex->nTime);
	}
	string strAddress = "";
	oName.push_back(Pair("txid", message.txHash.GetHex()));
	oName.push_back(Pair("time", sTime));
	oName.push_back(Pair("from", stringFromVch(message.vchAliasFrom)));
	oName.push_back(Pair("to", stringFromVch(message.vchAliasTo)));

	string strEncryptionPrivateKeyFrom = "";
	string strEncryptionPrivateKeyTo = "";
	string strKey = "";
	if(strWalletless == "true")
	{
		strEncryptionPrivateKeyFrom = HexStr(message.vchEncryptionPrivateKeyFrom);
		strEncryptionPrivateKeyTo = HexStr(message.vchEncryptionPrivateKeyTo);
	}
	else
	{
		CAliasIndex aliasFrom, aliasTo;
		CTransaction aliastxtmp;
		bool isExpired = false;
		vector<CAliasIndex> aliasVtxPos;
		if(GetTxAndVtxOfAlias(message.vchAliasFrom, aliasFrom, aliastxtmp, aliasVtxPos, isExpired, true))
		{
			aliasFrom.nHeight = message.nHeight;
			aliasFrom.GetAliasFromList(aliasVtxPos);
		}
		else
			return false;
		aliasVtxPos.clear();
		if(GetTxAndVtxOfAlias(message.vchAliasTo, aliasTo, aliastxtmp, aliasVtxPos, isExpired, true))
		{
			aliasTo.nHeight = message.nHeight;
			aliasTo.GetAliasFromList(aliasVtxPos);
		}
		else
			return false;
		if(DecryptMessage(aliasFrom, message.vchEncryptionPrivateKeyFrom, strKey))
			strEncryptionPrivateKeyFrom = HexStr(strKey);	
		else if(DecryptMessage(aliasTo, message.vchEncryptionPrivateKeyTo, strKey))
			strEncryptionPrivateKeyTo = HexStr(strKey);	
	}
	oName.push_back(Pair("encryption_privatekey_from", strEncryptionPrivateKeyFrom));
	oName.push_back(Pair("encryption_privatekey_to", strEncryptionPrivateKeyTo));
	oName.push_back(Pair("encryption_publickey", HexStr(message.vchEncryptionPublicKey)));

	string strDecrypted = "";
	string strData = "";
	
	if(strWalletless == "true")
		strData = HexStr(message.vchData);
	else
	{
		CMessageCrypter crypter;
		if(!strEncryptionPrivateKeyFrom.empty())
		{
			if(crypter.Decrypt(stringFromVch(ParseHex(strEncryptionPrivateKeyFrom)), stringFromVch(message.vchData), strDecrypted))
				strData = strDecrypted;
		}
		else if(!strEncryptionPrivateKeyTo.empty())
		{
			if(crypter.Decrypt(stringFromVch(ParseHex(strEncryptionPrivateKeyTo)), stringFromVch(message.vchData), strDecrypted))
				strData = strDecrypted;
		}
	}
	oName.push_back(Pair("privatevalue", strData));
	oName.push_back(Pair("publicvalue", stringFromVch(message.vchPubData)));
	return true;
}

UniValue messagesentlist(const UniValue& params, bool fHelp) {
    if (fHelp || 5 < params.size())
        throw runtime_error("messagesentlist [\"alias\",...] [<message>] [walletless=false] [count] [from]\n"
                "list messages that an array of aliases has sent. Set of aliases to look up based on alias.\n"
				"[count]          (numeric, optional, default=10) The number of results to return\n"
				"[from]           (numeric, optional, default=0) The number of results to skip\n");
	UniValue aliasesValue(UniValue::VARR);
	vector<string> aliases;
	if(CheckParam(params, 0))
	{
		if(params[0].isArray())
		{
			aliasesValue = params[0].get_array();
			for(unsigned int aliasIndex =0;aliasIndex<aliasesValue.size();aliasIndex++)
			{
				string lowerStr = aliasesValue[aliasIndex].get_str();
				boost::algorithm::to_lower(lowerStr);
				if(!lowerStr.empty())
					aliases.push_back(lowerStr);
			}
		}
	}
	vector<unsigned char> vchNameUniq;
   if(CheckParam(params, 1))
        vchNameUniq = vchFromValue(params[1]);

	string strWalletless = "false";
	if(CheckParam(params, 2))
		strWalletless = params[2].get_str();

	int count = 10;
	int from = 0;
	if (CheckParam(params, 3))
		count = atoi(params[3].get_str());
	if (CheckParam(params, 4))
		from = atoi(params[4].get_str());
	int found = 0;

	UniValue oRes(UniValue::VARR);
	map< vector<unsigned char>, int > vNamesI;
	map< vector<unsigned char>, UniValue > vNamesO;
	if(aliases.size() > 0)
	{
		for(unsigned int aliasIndex =0;aliasIndex<aliases.size();aliasIndex++)
		{
			if (oRes.size() >= count)
				break;
			const string &name = aliases[aliasIndex];
			const vector<unsigned char> &vchAlias = vchFromString(name);
			vector<CAliasIndex> vtxPos;
			if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
				continue;
			CTransaction tx;
			for(auto& it : boost::adaptors::reverse(vtxPos)) {
				if (oRes.size() >= count)
					break;
				const CAliasIndex& theAlias = it;
				if(!GetSyscoinTransaction(theAlias.nHeight, theAlias.txHash, tx, Params().GetConsensus()))
					continue;
				CMessage message(tx);
				if(!message.IsNull())
				{
					if (vNamesI.find(message.vchMessage) != vNamesI.end())
						continue;
					if (vchNameUniq.size() > 0 && vchNameUniq != message.vchMessage)
						continue;
					vector<CMessage> vtxMessagePos;
					if (!pmessagedb->ReadMessage(message.vchMessage, vtxMessagePos) || vtxMessagePos.empty())
						continue;
					const CMessage &theMessage = vtxMessagePos.back();
					if(theMessage.vchAliasFrom != theAlias.vchAlias)
						continue;
					
					UniValue oMessage(UniValue::VOBJ);
					vNamesI[message.vchMessage] = theMessage.nHeight;
					found++;
					if (found < from)
						continue;
					if(BuildMessageJson(theMessage, oMessage, strWalletless))
					{
						oRes.push_back(oMessage);
					}
					// if finding specific GUID don't need to look any further
					if (vchNameUniq.size() > 0)
						return oRes;
				}	
			}
		}
	}
    return oRes;
}

void MessageTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = messageFromOp(op);
	CMessage message;
	if(!message.UnserializeFromData(vchData, vchHash))
		return;

	bool isExpired = false;
	vector<CAliasIndex> aliasVtxPosFrom;
	vector<CAliasIndex> aliasVtxPosTo;
	CTransaction aliastx;
	CAliasIndex dbAliasFrom, dbAliasTo;
	if(GetTxAndVtxOfAlias(message.vchAliasFrom, dbAliasFrom, aliastx, aliasVtxPosFrom, isExpired, true))
	{
		dbAliasFrom.nHeight = message.nHeight;
		dbAliasFrom.GetAliasFromList(aliasVtxPosFrom);
	}
	if(GetTxAndVtxOfAlias(message.vchAliasTo, dbAliasTo, aliastx, aliasVtxPosTo, isExpired, true))
	{
		dbAliasTo.nHeight = message.nHeight;
		dbAliasTo.GetAliasFromList(aliasVtxPosTo);
	}
	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("GUID", stringFromVch(message.vchMessage)));

	string aliasFromValue = stringFromVch(message.vchAliasFrom);
	entry.push_back(Pair("from", aliasFromValue));

	string aliasToValue = stringFromVch(message.vchAliasTo);
	entry.push_back(Pair("to", aliasToValue));


	string strEncryptionPrivateKeyFrom = "";
	string strEncryptionPrivateKeyTo = "";
	string strKey = "";

	strEncryptionPrivateKeyFrom = HexStr(message.vchEncryptionPrivateKeyFrom);
	strEncryptionPrivateKeyTo = HexStr(message.vchEncryptionPrivateKeyTo);

	entry.push_back(Pair("encryption_privatekey_from", strEncryptionPrivateKeyFrom));
	entry.push_back(Pair("encryption_privatekey_to", strEncryptionPrivateKeyTo));
	entry.push_back(Pair("encryption_publickey", HexStr(message.vchEncryptionPublicKey)));

	string strDecrypted = "";
	string strData = "";
	strData = HexStr(message.vchData);

	entry.push_back(Pair("privatevalue", strData));
	entry.push_back(Pair("publicvalue", stringFromVch(message.vchPubData)));


}
UniValue messagestats(const UniValue& params, bool fHelp) {
	if (fHelp || 2 < params.size())
		throw runtime_error("messagestats unixtime=0 [\"alias\",...]\n"
				"Show statistics for all non-expired messages. Only messages created after unixtime are returned. Set of messages to look up based on array of aliases passed in. Leave empty for all messages.\n");
	vector<string> aliases;
	uint64_t nExpireFilter = 0;
	if(CheckParam(params, 0))
		nExpireFilter = params[0].get_int64();
	if(CheckParam(params, 1))
	{
		if(params[1].isArray())
		{
			UniValue aliasesValue = params[1].get_array();
			for(unsigned int aliasIndex =0;aliasIndex<aliasesValue.size();aliasIndex++)
			{
				string lowerStr = aliasesValue[aliasIndex].get_str();
				boost::algorithm::to_lower(lowerStr);
				if(!lowerStr.empty())
					aliases.push_back(lowerStr);
			}
		}
	}
	UniValue oMessageStats(UniValue::VOBJ);
	std::vector<CMessage> messages;
	if (!pmessagedb->GetDBMessages(messages, nExpireFilter, aliases))
		throw runtime_error("SYSCOIN_MESSAGE_RPC_ERROR ERRCODE: 2521 - " + _("Scan failed"));	
	if(!BuildMessageStatsJson(messages, oMessageStats))
		throw runtime_error("SYSCOIN_MESSAGE_RPC_ERROR ERRCODE: 2522 - " + _("Could not find this message"));

	return oMessageStats;

}
/* Output some stats about messages
	- Total number of messages
*/
bool BuildMessageStatsJson(const std::vector<CMessage> &messages, UniValue& oMessageStats)
{
	uint32_t totalMessages = messages.size();
	oMessageStats.push_back(Pair("totalmessages", (int)totalMessages));
	UniValue oMessages(UniValue::VARR);
	BOOST_REVERSE_FOREACH(const CMessage &message, messages) {
		UniValue oMessage(UniValue::VOBJ);
		if(!BuildMessageJson(message, oMessage, "true"))
			continue;
		oMessages.push_back(oMessage);
	}
	oMessageStats.push_back(Pair("messages", oMessages)); 
	return true;
}
bool CMessageDB::GetDBMessages(std::vector<CMessage>& messages, const uint64_t &nExpireFilter, const std::vector<std::string>& aliasArray)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	vector<CMessage> vtxPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "messagei") {       
				pcursor->GetValue(vtxPos);	
				if (vtxPos.empty())
				{
					pcursor->Next();
					continue;
				}
				const CMessage &txPos = vtxPos.back();
				if(chainActive.Height() <= txPos.nHeight || chainActive[txPos.nHeight]->nTime < nExpireFilter)
				{
					pcursor->Next();
					continue;
				}
  				if (chainActive.Tip()->nTime >= GetMessageExpiration(txPos))
				{
					pcursor->Next();
					continue;
				}
				if(aliasArray.size() > 0)
				{
					if (std::find(aliasArray.begin(), aliasArray.end(), stringFromVch(txPos.vchAliasTo)) == aliasArray.end() &&
						std::find(aliasArray.begin(), aliasArray.end(), stringFromVch(txPos.vchAliasFrom)) == aliasArray.end())
					{
						pcursor->Next();
						continue;
					}
				}
				messages.push_back(txPos);	
            }
			
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}