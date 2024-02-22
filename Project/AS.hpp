#ifndef SERVER_H
#define SERVER_H

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
#include <filesystem>
#include <fstream>
#include <vector>
#include <time.h>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <sys/stat.h>
#include <iterator>
#include <sys/time.h>


using namespace std;
#define PORT "58001"
#define max(A,B) ((A)>=(B)?(A):(B))
#define maxFiles 50



string lin_str="LIN";
string lou_str = "LOU";
string unr_str = "UNR";

void login (vector<string> args);
bool check_login(string username, string password);
void create_user(string path, string username, string password);
void logout (vector<string> args);
void unregister(vector<string> args);
void open_auc(vector<string> words);
bool check_open(string name, string asset_fname, string start_value, string timeactive);
void close_auc(vector<string> mess);
void check_endtime(string aid, string user);
string convert_to_date(time_t seconds);
void myauctions(vector<string> mess);
void mybids(vector<string> mess);
void list(vector<string> mess);
string get_auc_user(string aid);
void showrecord(vector<string> mess);
int get_start_fulltime(string aid);
int get_start_timeactive(string aid);
bool check_pass(string file_pass, string pass);
bool check_close(string uid, string pass, string aid);
bool check_aid(string aid);
bool check_auc_owner(string aid, string user);
bool check_end(string aid);
int receive_file(string filename, string file_size);
void getAuctionAid();
void set_auc_aid(int auctionAID);
vector<string> splitWords(const string& buffer);
void create_udp();
void receive_udp();
void create_tcp();
void receive_tcp();
void send_err_udp();
void send_err_tcp();
string get_time();
void move_file(string filename, string dest_path, string name);
void create_file(string path, string content);
void create_dir(string path);
bool compareBids(string previous, string newBid);
string createBidName(string value);
void bid(vector<string> mess);
size_t get_file_size(const std::string& filename);
int send_file(const std::string& filename, string file_size);
bool AID_exists(string AID);
bool user_exists(string user_ID);
bool user_is_logged_in(string user_ID);


namespace fs = std::filesystem;

#endif