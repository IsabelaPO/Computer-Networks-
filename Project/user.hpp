#ifndef CLIENT_H
#define CLIENT_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <iostream>
#include <cctype>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <csignal>
#include <cstdlib>


#define DEFPORT "58052"

using namespace std;

string login_str= "login\0";
string logout_str= "logout";
string unregister_str= "unregister";
string exit_str= "exit";
string open_str= "open";
string close_str= "close";
string myauctions_str= "myauctions";
string ma_str= "ma";
string mybids_str= "mybids";
string mb_str= "mb";
string list_str= "list";
string l_str= "l";
string show_asset_str= "show_asset";
string sa_str= "sa";
string bid_str= "bid";
string b_str= "b";
string show_record_str= "show_record";
string sr_str="sr";

string rli_str = "RLI";
string rlo_str = "RLO";
string rur_str = "RUR";
string rma_str = "RMA";
string rmb_str = "RMB";
string rls_str = "RLS";
string rrc_str = "RRC";

void receive_reply(vector<string> arguments);
vector<string> splitWords(const string& buffer);
void logout();
void login(vector<string> mess);
bool check_login(string username, string password);
void unregister();
void exitProgram();
void open(vector<string> mess);
bool check_open(string name, string asset_fname, string start_value, string timeactive);
void close_auc(vector<string> mess);
void myauctions();
void mybids();
void list();
void bid(vector<string> mess);
void show_records(vector<string> mess);
void show_asset(vector<string> mess);
void send_file(string file_name, ssize_t file_size);
void open_socket_udp();
void close_socket_udp();
void wait_reply_udp();
void open_socket_tcp();
void wait_reply_tcp();
int receive_file(string filename, string file_size);

namespace fs = std::filesystem;


#endif