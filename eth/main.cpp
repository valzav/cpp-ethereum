/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file main.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Ethereum client.
 */

#include <fstream>
#include "Client.h"
#include "PeerNetwork.h"
#include "BlockChain.h"
#include "State.h"
using namespace std;
using namespace eth;

bytes contents(std::string const& _file)
{
	std::ifstream is(_file, std::ifstream::binary);
	if (!is)
		return bytes();
	// get length of file:
	is.seekg (0, is.end);
	int length = is.tellg();
	is.seekg (0, is.beg);
	bytes ret(length);
	is.read((char*)ret.data(), length);
	is.close();
	return ret;
}

void writeFile(std::string const& _file, bytes const& _data)
{
	ofstream(_file, ios::trunc).write((char const*)_data.data(), _data.size());
}

int main(int argc, char** argv)
{
	short listenPort = 30303;
	string remoteHost;
	short remotePort = 30303;
	bool interactive = false;
	string dbPath;
	eth::uint mining = ~(eth::uint)0;

	// Our address.
	KeyPair us = KeyPair::create();
	Address coinbase = us.address();

	string configFile = string(getenv("HOME")) + "/.ethereum/config.rlp";
	bytes b = contents(configFile);
	if (b.size())
	{
		RLP config(b);
		us = KeyPair(config[0].toHash<Secret>());
		coinbase = config[1].toHash<Address>();
	}
	else
	{
		RLPStream config(2);
		config << us.secret() << coinbase;
		writeFile(configFile, config.out());
	}

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-l" && i + 1 < argc)
			listenPort = atoi(argv[++i]);
		else if (arg == "-r" && i + 1 < argc)
			remoteHost = argv[++i];
		else if (arg == "-p" && i + 1 < argc)
			remotePort = atoi(argv[++i]);
		else if (arg == "-a" && i + 1 < argc)
			coinbase = h160(fromUserHex(argv[++i]));
		else if (arg == "-s" && i + 1 < argc)
			us = KeyPair(h256(fromUserHex(argv[++i])));
		else if (arg == "-i")
			interactive = true;
		else if (arg == "-d" && i + 1 < argc)
			dbPath = argv[++i];
		else if (arg == "-m" && i + 1 < argc)
			if (string(argv[++i]) == "on")
				mining = ~(eth::uint)0;
			else if (string(argv[i]) == "off")
				mining = 0;
			else
				mining = atoi(argv[i]);
		else
			remoteHost = argv[i];
	}

	Client c("Ethereum(++)/v0.1", coinbase, dbPath);
	if (interactive)
	{
		cout << "Ethereum (++)" << endl;
		cout << "  Code by Gav Wood, (c) 2013, 2014." << endl;
		cout << "  Based on a design by Vitalik Buterin." << endl << endl;

		while (true)
		{
			cout << "> " << flush;
			std::string cmd;
			cin >> cmd;
			if (cmd == "netstart")
			{
				eth::uint port;
				cin >> port;
				c.startNetwork(port);
			}
			else if (cmd == "connect")
			{
				string addr;
				eth::uint port;
				cin >> addr >> port;
				c.connect(addr, port);
			}
			else if (cmd == "netstop")
			{
				c.stopNetwork();
			}
			else if (cmd == "minestart")
			{
				c.startMining();
			}
			else if (cmd == "minestop")
			{
				c.stopMining();
			}
			else if (cmd == "transact")
			{
				string sechex;
				string rechex;
				u256 amount;
				u256 fee;
				cin >> sechex >> rechex >> amount >> fee;
				Secret secret = h256(fromUserHex(sechex));
				Address dest = h160(fromUserHex(rechex));
				c.transact(secret, dest, amount, fee);
			}
			else if (cmd == "send")
			{
				string rechex;
				u256 amount;
				u256 fee;
				cin >> rechex >> amount >> fee;
				Address dest = h160(fromUserHex(rechex));
				c.transact(us.secret(), dest, amount, fee);
			}
		}
	}
	else
	{
		c.startNetwork(listenPort, remoteHost, remotePort);
		eth::uint n = c.blockChain().details().number;
		while (true)
		{
			if (c.blockChain().details().number - n > mining)
				c.stopMining();
			else
				c.startMining();
			usleep(100000);
		}
	}


	return 0;
}
