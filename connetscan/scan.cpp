#include "log.h"
#include "scan.h"
#include <iostream>
#include <string>
#include <winsock2.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <thread>
#include <mutex>
using namespace std;
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
mutex mx;
condition_variable cv;

void scan::set_scan_option(string I, int P, int T, char TY, logg* L) {
	this->IpAddr.push_back(I);
	this->port.push_back(P);
	this->timeout = T;
	this->type = TY;
	this->log = L;
}

void scan::set_scan_option() {
	string I;
	int P;
	char TY;
	cout << "Enter type of scan(s-simple(1 ip-hostname, 1 port)/##a-auto(range of ip, 1 port)##/r-ranged(1 ip-hostname, range of port))" << endl;
	cin >> TY;
	this->type = TY;
	cout << "Enter Hostname or IP address" << endl;
	cin >> I;
	this->IpAddr.push_back(I);
	if (TY == 's') {
		cout << "Enter port" << endl;
		cin >> P;
		this->port.push_back(P);
	}
	if (TY == 'r') {
		int ptr = 2;
		while (ptr) {
			cout << "Enter port range(80 443|from * to)" << endl;
			cin >> P;
			this->port.push_back(P);
			ptr--;
		}
	}
}

void scan::cout_scan_option() {
	for (auto ipa : this->IpAddr) {
		for (auto ptrport : this->port) {
			cout << ipa << " : " << ptrport << " : " << this->timeout << " : " << this->type << endl;
		}
	}
}

string scan::get_scan_option() {
	string res;
	if (this->IpAddr.size() > 1) {
		res += this->hstnm + " : " + this->IpAddr[0] + "-" + this->IpAddr[this->IpAddr.size()-1];
		return res;
	}
	else {
		for (auto ipa : this->IpAddr) {
			for (auto ptrport : this->port) {
				res += ipa + ":" + to_string(ptrport) + "-";
			}
		}
	}
	return res;
}

void scan::cout_scan_result() {
	cout << this->Result << endl;
}

string scan::get_scan_result() {
	return this->Result;
}

void scan::set_init_log(logg* L) {
	this->log = L;
}

void scan::get_IP_by_hostname(string H) {
	//init
	WSAData Data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &Data);
	if (wsResult != 0) {
		this->Result = "Can't start Winsock, error! " + to_string(wsResult);
		cerr << "Can't start Winsock, error!" << wsResult << endl;
		cout << this->Result << ": thread : 1" << endl;
		WSACleanup();
		return;
	}
	//Create socket
	SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
	if (Clsock == INVALID_SOCKET) {
		cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
		this->Result = "Can't create Client socket, error! " + WSAGetLastError();
		cout << this->Result << ": thread : 1" << endl;
		WSACleanup();
		return;
	}
	//hint
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	int ptr0 = 0;
	char* hostname;
	hostname = (char*)H.c_str();
	this->IpAddr.pop_back();
	this->IpAddr.clear();
	struct hostent* host_info;
	struct in_addr addr;
	host_info = gethostbyname(hostname);
	DWORD dw;
	if (host_info == NULL) {
		dw = WSAGetLastError();
		if (dw != 0) {
			if (dw == WSAHOST_NOT_FOUND) {
				//cout << "Host is not found" << endl;
				this->Result += "Host is not found";
				this->log->add_log_string(this->Result);
				cout << this->Result << ": thread : 1" << endl;
				closesocket(Clsock);
				WSACleanup();
				return;
			}
			else if (dw == WSANO_DATA) {
				//cout << "No data record is found" << endl;
				this->Result += "No data record is found";
				this->log->add_log_string(this->Result);
				cout << this->Result << ": thread : 1" << endl;
				closesocket(Clsock);
				WSACleanup();
				return;
			}
			else {
				//cout << "Function failed with an error : " << dw << endl;
				this->Result += "Function failed with an error : " + to_string(dw);
				this->log->add_log_string(this->Result);
				cout << this->Result << ": thread : 1" << endl;
				closesocket(Clsock);
				WSACleanup();
				return;
			}
		}
	}
	else {
		while (host_info->h_addr_list[ptr0] != 0) {
			addr.s_addr = *(u_long*)host_info->h_addr_list[ptr0++];
			cout << "IP Address: " << inet_ntoa(addr) << endl;
			string IpAdd = inet_ntoa(addr);
			this->IpAddr.push_back(IpAdd);
			inet_pton(AF_INET, IpAdd.c_str(), &hint.sin_addr);
		}
	}
	closesocket(Clsock);
	WSACleanup();
}

void scan::set_copy_hstnm(string H) {
	this->hstnm = H;
}

void scan::thread_start_scan_second() {
	unique_lock<mutex> lock(mx);
	cv.wait(lock);
	if (this->IpAddr.size() == 1) {
		for (int ptrPort = ((this->port[1] - this->port[0]) / 2) + this->port[0]; ptrPort <= this->port[1]; ptrPort++) {
			//init
			WSAData Data;
			WORD ver = MAKEWORD(2, 2);
			int wsResult = WSAStartup(ver, &Data);
			if (wsResult != 0) {
				this->Result = "Can't start Winsock, error! " + to_string(wsResult);
				cerr << "Can't start Winsock, error!" << wsResult << endl;
				cout << this->Result << ": thread : 2" << endl;
				WSACleanup();
				return;
			}
			//Create socket
			SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
			if (Clsock == INVALID_SOCKET) {
				cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
				this->Result = "Can't create Client socket, error! " + WSAGetLastError();
				cout << this->Result << ": thread : 2" << endl;
				WSACleanup();
				return;
			}
			//hint
			sockaddr_in hint;
			hint.sin_family = AF_INET;
			//ports
			hint.sin_port = htons(ptrPort);
			//connect serv
			inet_pton(AF_INET, this->IpAddr[0].c_str(), &hint.sin_addr);
			//-------------
			int connResult = connect(Clsock, (sockaddr*)&hint, sizeof(hint));
			if (connResult == SOCKET_ERROR) {
				if (WSAGetLastError() == 10061) {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "You could not make a connection because the target machine actively refused it";
					cout << this->Result << ": thread : 2" << endl;
					this->log->add_log_string(this->Result);
					shutdown(Clsock, 0);
					WSACleanup();
					continue;
				}
				if (WSAGetLastError() == 10060) {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "Time-out";
					cout << this->Result << ": thread : 2" << endl;
					this->log->add_log_string(this->Result);
					return;
				}
				if (WSAGetLastError() == EWOULDBLOCK) {
					this->Result = "Time out for " + this->IpAddr[0] + ":" + to_string(ptrPort);
					cout << this->Result << ": thread : 2" << endl;
					this->log->add_log_string(this->Result);
					shutdown(Clsock, 0);
					WSACleanup();
					continue;
				}
				else {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError());
					cout << this->Result << ": thread : 2" << endl;
					this->log->add_log_string(this->Result);
					shutdown(Clsock, 0);
					WSACleanup();
					continue;
				}
			}
			else {
				this->Result = "Client connected to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " - Port is open.";
				cout << this->Result << ": thread : 2" << endl;
				this->log->add_log_string(this->Result);
				shutdown(Clsock, 0);
				WSACleanup();
				continue;
			}
			//-------------
		}
	}
	else {
		for (auto ipa : this->IpAddr) {
			for (int ptrPort = ((this->port[1] - this->port[0]) / 2) + this->port[0]; ptrPort <= this->port[1]; ptrPort++) {
				//init
				WSAData Data;
				WORD ver = MAKEWORD(2, 2);
				int wsResult = WSAStartup(ver, &Data);
				if (wsResult != 0) {
					this->Result = "Can't start Winsock, error! " + to_string(wsResult);
					cerr << "Can't start Winsock, error!" << wsResult << endl;
					cout << this->Result << ": thread : 2" << endl;
					WSACleanup();
					return;
				}
				//Create socket
				SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
				if (Clsock == INVALID_SOCKET) {
					cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
					this->Result = "Can't create Client socket, error! " + WSAGetLastError();
					cout << this->Result << ": thread : 2" << endl;
					WSACleanup();
					return;
				}
				//hint
				sockaddr_in hint;
				hint.sin_family = AF_INET;
				//ports
				hint.sin_port = htons(ptrPort);
				//connect serv
				inet_pton(AF_INET, ipa.c_str(), &hint.sin_addr);
				//-------------
				int connResult = connect(Clsock, (sockaddr*)&hint, sizeof(hint));
				if (connResult == SOCKET_ERROR) {
					if (WSAGetLastError() == 10061) {
						this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "You could not make a connection because the target machine actively refused it";
						cout << this->Result << ": thread : 2" << endl;
						this->log->add_log_string(this->Result);
						shutdown(Clsock, 0);
						WSACleanup();
						continue;
					}
					if (WSAGetLastError() == 10060) {
						this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "Time-out";
						cout << this->Result << ": thread : 2" << endl;
						this->log->add_log_string(this->Result);
						return;
					}
					if (WSAGetLastError() == EWOULDBLOCK) {
						this->Result = "Time out for " + ipa + ":" + to_string(ptrPort);
						cout << this->Result << ": thread : 2" << endl;
						this->log->add_log_string(this->Result);
						shutdown(Clsock, 0);
						WSACleanup();
						continue;
					}
					else {
						this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError());
						cout << this->Result << ": thread : 2" << endl;
						this->log->add_log_string(this->Result);
						shutdown(Clsock, 0);
						WSACleanup();
						continue;
					}
				}
				else {
					this->Result = "Client connected to " + ipa + ":" + to_string(ptrPort) + " - Port is open.";
					cout << this->Result << ": thread : 2" << endl;
					this->log->add_log_string(this->Result);
					shutdown(Clsock, 0);
					WSACleanup();
					continue;
				}
				//-------------
			}
		}
	}
	cv.notify_all();
	//close sock
	return;
}
void scan::thread_start_scan_first() {
	//ports
	if (this->IpAddr.size() == 1) {
		for (int ptrPort = this->port[0]; ptrPort <= this->port[1] / 2 - 1; ptrPort++) {
			WSAData Data;
			WORD ver = MAKEWORD(2, 2);
			int wsResult = WSAStartup(ver, &Data);
			if (wsResult != 0) {
				this->Result = "Can't start Winsock, error! " + to_string(wsResult);
				cerr << "Can't start Winsock, error!" << wsResult << endl;
				cout << this->Result << ": thread : 1" << endl;
				WSACleanup();
				return;
			}
			//Create socket
			SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
			if (Clsock == INVALID_SOCKET) {
				cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
				this->Result = "Can't create Client socket, error! " + WSAGetLastError();
				cout << this->Result << ": thread : 1" << endl;
				WSACleanup();
				return;
			}
			//hint
			sockaddr_in hint;
			hint.sin_family = AF_INET;
			hint.sin_port = htons(ptrPort);
			inet_pton(AF_INET, this->IpAddr[0].c_str(), &hint.sin_addr);
			//connect serv
			//-------------
			int connResult = connect(Clsock, (sockaddr*)&hint, sizeof(hint));
			if (connResult == SOCKET_ERROR) {
				if (WSAGetLastError() == 10061) {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "You could not make a connection because the target machine actively refused it";
					cout << this->Result << ": thread : 1" << endl;
					this->log->add_log_string(this->Result);
					shutdown(Clsock, 0);
					WSACleanup();
					continue;
				}
				if (WSAGetLastError() == 10060) {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "Time-out";
					cout << this->Result << ": thread : 1" << endl;
					this->log->add_log_string(this->Result);
					return;
				}
				if (WSAGetLastError() == EWOULDBLOCK) {
					this->Result = "Time out for " + this->IpAddr[0] + ":" + to_string(ptrPort);
					cout << this->Result << ": thread : 1" << endl;
					this->log->add_log_string(this->Result);
					shutdown(Clsock, 0);
					WSACleanup();
					continue;
				}
				else {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError());
					cout << this->Result << ": thread : 1" << endl;
					this->log->add_log_string(this->Result);
					shutdown(Clsock, 0);
					WSACleanup();
					continue;
				}
			}
			else {
				this->Result = "Client connected to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " - Port is open.";
				cout << this->Result << ": thread : 1" << endl;
				this->log->add_log_string(this->Result);
				shutdown(Clsock, 0);
				WSACleanup();
				continue;
			}
			//-------------
		}
	}
	else {
		for (auto ipa : this->IpAddr) {
			for (int ptrPort = ((this->port[1] - this->port[0]) / 2) + this->port[0]; ptrPort <= this->port[1]; ptrPort++) {
				//init
				WSAData Data;
				WORD ver = MAKEWORD(2, 2);
				int wsResult = WSAStartup(ver, &Data);
				if (wsResult != 0) {
					this->Result = "Can't start Winsock, error! " + to_string(wsResult);
					cerr << "Can't start Winsock, error!" << wsResult << endl;
					cout << this->Result << ": thread : 1" << endl;
					WSACleanup();
					return;
				}
				//Create socket
				SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
				if (Clsock == INVALID_SOCKET) {
					cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
					this->Result = "Can't create Client socket, error! " + WSAGetLastError();
					cout << this->Result << ": thread : 1" << endl;
					WSACleanup();
					return;
				}
				//hint
				sockaddr_in hint;
				hint.sin_family = AF_INET;
				//ports
				hint.sin_port = htons(ptrPort);
				//connect serv
				inet_pton(AF_INET, ipa.c_str(), &hint.sin_addr);
				//-------------
				int connResult = connect(Clsock, (sockaddr*)&hint, sizeof(hint));
				if (connResult == SOCKET_ERROR) {
					if (WSAGetLastError() == 10061) {
						this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "You could not make a connection because the target machine actively refused it";
						cout << this->Result << ": thread : 1" << endl;
						this->log->add_log_string(this->Result);
						shutdown(Clsock, 0);
						WSACleanup();
						continue;
					}
					if (WSAGetLastError() == 10060) {
						this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "Time-out";
						cout << this->Result << ": thread : 1" << endl;
						this->log->add_log_string(this->Result);
						return;
					}
					if (WSAGetLastError() == EWOULDBLOCK) {
						this->Result = "Time out for " + ipa + ":" + to_string(ptrPort);
						cout << this->Result << ": thread : 1" << endl;
						this->log->add_log_string(this->Result);
						shutdown(Clsock, 0);
						WSACleanup();
						continue;
					}
					else {
						this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError());
						cout << this->Result << ": thread : 1" << endl;
						this->log->add_log_string(this->Result);
						shutdown(Clsock, 0);
						WSACleanup();
						continue;
					}
				}
				else {
					this->Result = "Client connected to " + ipa + ":" + to_string(ptrPort) + " - Port is open.";
					cout << this->Result << ": thread : 1" << endl;
					this->log->add_log_string(this->Result);
					shutdown(Clsock, 0);
					WSACleanup();
					continue;
				}
				//-------------
			}
		}
	}
	cv.notify_all();
	return;
}
void scan::start_scan() {
	//chek hostname or IP in this->IpAddr string
	//------------------- NEED TO BE REPLACED AS FUNCTION OF CLASS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (ip_or_hostname_check(this->IpAddr[0])) {
		set_copy_hstnm(this->IpAddr[0]);
		get_IP_by_hostname(this->IpAddr[0]);
		this->log->add_log_string("Hostname: "+this->hstnm);
		for (auto ipa : this->IpAddr) {
			this->log->add_log_string("Ip address: " + ipa);
		}
	}
	else {
		this->log->add_log_string("Ip address: " + this->IpAddr[0]);
	}
	//--------------
	if (this->IpAddr.size() == 1) {
		if (this->type == 's') {
			//init
			WSAData Data;
			WORD ver = MAKEWORD(2, 2);
			int wsResult = WSAStartup(ver, &Data);
			if (wsResult != 0) {
				this->Result = "Can't start Winsock, error! " + to_string(wsResult);
				cerr << "Can't start Winsock, error!" << wsResult << endl;
				cout << this->Result << ": thread : 1" << endl;
				WSACleanup();
				return;
			}
			//Create socket
			SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
			if (Clsock == INVALID_SOCKET) {
				cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
				this->Result = "Can't create Client socket, error! " + WSAGetLastError();
				cout << this->Result << ": thread : 1" << endl;
				WSACleanup();
				return;
			}
			//hint
			sockaddr_in hint;
			hint.sin_family = AF_INET;
			inet_pton(AF_INET, this->IpAddr[0].c_str(), &hint.sin_addr);
			hint.sin_port = htons(this->port[0]);
			//connect serv
			//-------------
			int connResult = connect(Clsock, (sockaddr*)&hint, sizeof(hint));
			if (connResult == SOCKET_ERROR) {
				if (WSAGetLastError() == 10061) {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(this->port[0]) + " " + to_string(WSAGetLastError()) + "|" + "You could not make a connection because the target machine actively refused it";
					cout << this->Result << ": only 1 thread : " << endl;
					this->log->add_log_string(this->Result);
					return;
				}
				if (WSAGetLastError() == 10060) {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(this->port[0]) + " " + to_string(WSAGetLastError()) + "|" + "Time-out";
					cout << this->Result << ": only 1 thread : " << endl;
					this->log->add_log_string(this->Result);
					return;
				}
				if (WSAGetLastError() == EWOULDBLOCK) {
					this->Result = "Time out for " + this->IpAddr[0] + ":" + to_string(this->port[0]);
					cout << this->Result << ": only 1 thread : " << endl;
					this->log->add_log_string(this->Result);
					return;
				}
				else {
					this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(this->port[0]) + " " + to_string(WSAGetLastError());
					cout << this->Result << ": only 1 thread : " << endl;
					this->log->add_log_string(this->Result);
					return;
				}
			}
			else {
				this->Result = "Client connected to " + this->IpAddr[0] + ":" + to_string(this->port[0]) + " - Port is open.";
				cout << this->Result << ": only 1 thread : " << endl;
				this->log->add_log_string(this->Result);
				return;
			}
			//-------------
		}
		else if (this->type == 'r' && this->port.size() == 2) {
			if (this->port[1] - this->port[0] >= 50) {
				thread firstScan(&scan::thread_start_scan_first, this);
				thread_start_scan_second();
				firstScan.join();
			}
			else {
				for (int ptrPort = this->port[0]; ptrPort <= this->port[1]; ptrPort++) {
					WSAData Data;
					WORD ver = MAKEWORD(2, 2);
					int wsResult = WSAStartup(ver, &Data);
					if (wsResult != 0) {
						this->Result = "Can't start Winsock, error! " + to_string(wsResult);
						cerr << "Can't start Winsock, error!" << wsResult << endl;
						cout << this->Result << ": thread : 1" << endl;
						WSACleanup();
						return;
					}
					//Create socket
					SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
					if (Clsock == INVALID_SOCKET) {
						cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
						this->Result = "Can't create Client socket, error! " + WSAGetLastError();
						cout << this->Result << ": thread : 1" << endl;
						WSACleanup();
						return;
					}
					//hint
					sockaddr_in hint;
					hint.sin_family = AF_INET;
					hint.sin_port = htons(ptrPort);
					inet_pton(AF_INET, this->IpAddr[0].c_str(), &hint.sin_addr);
					//connect serv
					//-------------
					int connResult = connect(Clsock, (sockaddr*)&hint, sizeof(hint));
					if (connResult == SOCKET_ERROR) {
						if (WSAGetLastError() == 10061) {
							this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "You could not make a connection because the target machine actively refused it";
							cout << this->Result << ": only 1 thread : " << endl;
							this->log->add_log_string(this->Result);
							shutdown(Clsock, 0);
							WSACleanup();
							continue;
						}
						if (WSAGetLastError() == 10060) {
							this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "Time-out";
							cout << this->Result << ": only 1 thread : " << endl;
							this->log->add_log_string(this->Result);
							shutdown(Clsock, 0);
							WSACleanup();
							continue;
						}
						else if (WSAGetLastError() == EWOULDBLOCK) {
							this->Result = "Time out for " + this->IpAddr[0] + ":" + to_string(ptrPort);
							cout << this->Result << ": thread : 1" << endl;
							this->log->add_log_string(this->Result);
							shutdown(Clsock, 0);
							WSACleanup();
							continue;
						}
						else {
							this->Result = "Cant connect to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError());
							cout << this->Result << ": only 1 thread : " << endl;
							this->log->add_log_string(this->Result);
							shutdown(Clsock, 0);
							WSACleanup();
							continue;
						}
					}
					else {
						this->Result = "Client connected to " + this->IpAddr[0] + ":" + to_string(ptrPort) + " - Port is open";
						cout << this->Result << ": only 1 thread : " << endl;
						this->log->add_log_string(this->Result);
						shutdown(Clsock, 0);
						WSACleanup();
						continue;
					}
					//-------------
					closesocket(Clsock);
					WSACleanup();
				}
			}
		}
	}
	else {
	for (auto ipa : this->IpAddr) {
		if (this->type == 's') {
			//init
			WSAData Data;
			WORD ver = MAKEWORD(2, 2);
			int wsResult = WSAStartup(ver, &Data);
			if (wsResult != 0) {
				this->Result = "Can't start Winsock, error! " + to_string(wsResult);
				cerr << "Can't start Winsock, error!" << wsResult << endl;
				cout << this->Result << ": thread : 1" << endl;
				WSACleanup();
				return;
			}
			//Create socket
			SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
			if (Clsock == INVALID_SOCKET) {
				cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
				this->Result = "Can't create Client socket, error! " + WSAGetLastError();
				cout << this->Result << ": thread : 1" << endl;
				WSACleanup();
				continue;
			}
			//hint
			sockaddr_in hint;
			hint.sin_family = AF_INET;
			inet_pton(AF_INET, ipa.c_str(), &hint.sin_addr);
			hint.sin_port = htons(this->port[0]);
			//connect serv
			//-------------
			int connResult = connect(Clsock, (sockaddr*)&hint, sizeof(hint));
			if (connResult == SOCKET_ERROR) {
				if (WSAGetLastError() == 10061) {
					this->Result = "Cant connect to " + ipa + ":" + to_string(this->port[0]) + " " + to_string(WSAGetLastError()) + "|" + "You could not make a connection because the target machine actively refused it";
					cout << this->Result << ": only 1 thread : " << endl;
					this->log->add_log_string(this->Result);
					continue;
				}
				if (WSAGetLastError() == 10060) {
					this->Result = "Cant connect to " + ipa + ":" + to_string(this->port[0]) + " " + to_string(WSAGetLastError()) + "|" + "Time-out";
					cout << this->Result << ": only 1 thread : " << endl;
					this->log->add_log_string(this->Result);
					continue;
				}
				if (WSAGetLastError() == EWOULDBLOCK) {
					this->Result = "Time out for " + ipa + ":" + to_string(this->port[0]);
					cout << this->Result << ": only 1 thread : " << endl;
					this->log->add_log_string(this->Result);
					continue;
				}
				else {
					this->Result = "Cant connect to " + ipa + ":" + to_string(this->port[0]) + " " + to_string(WSAGetLastError());
					cout << this->Result << ": only 1 thread : " << endl;
					this->log->add_log_string(this->Result);
					continue;
				}
			}
			else {
				this->Result = "Client connected to " + ipa + ":" + to_string(this->port[0]) + " - Port is open.";
				cout << this->Result << ": only 1 thread : " << endl;
				this->log->add_log_string(this->Result);
				continue;
			}
			//-------------
		}
		else if (this->type == 'r' && this->port.size() == 2) {
			if (this->port[1] - this->port[0] >= 50) {
				thread firstScan(&scan::thread_start_scan_first, this);
				thread_start_scan_second();
				firstScan.join();
			}
			else {
				for (int ptrPort = this->port[0]; ptrPort <= this->port[1]; ptrPort++) {
					WSAData Data;
					WORD ver = MAKEWORD(2, 2);
					int wsResult = WSAStartup(ver, &Data);
					if (wsResult != 0) {
						this->Result = "Can't start Winsock, error! " + to_string(wsResult);
						cerr << "Can't start Winsock, error!" << wsResult << endl;
						cout << this->Result << ": thread : 1" << endl;
						WSACleanup();
						continue;
					}
					//Create socket
					SOCKET Clsock = socket(AF_INET, SOCK_STREAM, 0);
					if (Clsock == INVALID_SOCKET) {
						cerr << "Can't create Client socket, error! " << WSAGetLastError() << endl;
						this->Result = "Can't create Client socket, error! " + WSAGetLastError();
						cout << this->Result << ": thread : 1" << endl;
						WSACleanup();
						continue;
					}
					//hint
					sockaddr_in hint;
					hint.sin_family = AF_INET;
					hint.sin_port = htons(ptrPort);
					inet_pton(AF_INET, ipa.c_str(), &hint.sin_addr);
					//connect serv
					//-------------
					int connResult = connect(Clsock, (sockaddr*)&hint, sizeof(hint));
					if (connResult == SOCKET_ERROR) {
						if (WSAGetLastError() == 10061) {
							this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "You could not make a connection because the target machine actively refused it";
							cout << this->Result << ": only 1 thread : " << endl;
							this->log->add_log_string(this->Result);
							shutdown(Clsock, 0);
							WSACleanup();
							continue;
						}
						if (WSAGetLastError() == 10060) {
							this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError()) + "|" + "Time-out";
							cout << this->Result << ": only 1 thread : " << endl;
							this->log->add_log_string(this->Result);
							shutdown(Clsock, 0);
							WSACleanup();
							continue;
						}
						else if (WSAGetLastError() == EWOULDBLOCK) {
							this->Result = "Time out for " + ipa + ":" + to_string(ptrPort);
							cout << this->Result << ": thread : 1" << endl;
							this->log->add_log_string(this->Result);
							shutdown(Clsock, 0);
							WSACleanup();
							continue;
						}
						else {
							this->Result = "Cant connect to " + ipa + ":" + to_string(ptrPort) + " " + to_string(WSAGetLastError());
							cout << this->Result << ": only 1 thread : " << endl;
							this->log->add_log_string(this->Result);
							shutdown(Clsock, 0);
							WSACleanup();
							continue;
						}
					}
					else {
						this->Result = "Client connected to " + ipa + ":" + to_string(ptrPort) + " - Port is open";
						cout << this->Result << ": only 1 thread : " << endl;
						this->log->add_log_string(this->Result);
						shutdown(Clsock, 0);
						WSACleanup();
						continue;
					}
					//-------------
					closesocket(Clsock);
					WSACleanup();
				}
			}
		}
	}
}
	return;
}

//check ip or hostname give to us (true if hostname)
bool scan::ip_or_hostname_check(string IpAd) {
	for (char ptr0 : IpAd) {
		if ((ptr0 > 64 && ptr0 < 91) || (ptr0 > 96 && ptr0 < 123)) {
			return 1;
		}
	}
	return 0;
}

