#include "AS.hpp"
int fd_udp, errcode;
int fd_tcp, new_fd_tcp;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[1024];
char prot_mess[1024];
char *ptr;
string port;
string asIP;
int verbose;
int auctionAID;



int main(int argc, char *argv[]) {
    string defPort = "58062";  //  porto para testar no tejo
    string defIP = "localhost";
    verbose = 0;

    port = defPort;
    asIP = defIP;

    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 seconds
    timeout.tv_usec = 0;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            // Use the next argument as AS port
            asIP = argv[i + 1];
            i++;  // Skip the next argument
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        }
    }

    create_udp();
    create_tcp();

    /* Loop para receber bytes e processá-los */
    while (true) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(fd_udp, &read_set);
        FD_SET(fd_tcp, &read_set);

        int max_fd = max(fd_udp, fd_tcp) + 1;

        // Wait for activity on any of the sockets
        if (select(max_fd, &read_set, nullptr, nullptr, &timeout) <= 0) {
            exit(1);
        }
        //probably ainda temos de fazer fork depois de percebermos qual dos sockets recebeu um pedido

        //handle udp request
        if(FD_ISSET(fd_udp, &read_set)){
            receive_udp();
        }

        //handle tcp request
        if(FD_ISSET(fd_tcp, &read_set)){
            memset(buffer,0,sizeof(buffer));
            addrlen = sizeof(addr);
            if((new_fd_tcp = accept(fd_tcp, (struct sockaddr *)&addr, &addrlen))==-1){
                exit(1);
            }
            receive_tcp();
        }
    }
    freeaddrinfo(res);
    close(fd_udp);
    close(fd_tcp);
    close(new_fd_tcp);
}

//----------------------------------------------------------------------------login------------------------------------------------------------------------------

void login (vector<string> mess){
    //LIN UID password
    memset(prot_mess,0,sizeof(prot_mess));
    if((mess.size() == 3) && (check_login(mess[1],mess[2]))){
        string path = "users/" + mess[1];

        //user não existe
        if (!user_exists(mess[1])){
            create_user(path, mess[1], mess[2]); 
            if(verbose == 1){cout << "UID=" << mess[1] << ":new user; registered in database" << endl;}

            sprintf(prot_mess,"RLI REG\n");
            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
            if (n == -1) {
                exit(1);
            }
        } else{
            //user existe
            string file_pass = "users/" + mess[1] +"/"+ mess[1]+"_pass.txt";
            
            if (!fs::exists(file_pass)){
                //user foi unregistered
                create_user(path,mess[1], mess[2]);
                if(verbose==1){cout<<"UID="<< mess[1]<< ":previous user; registered again"<<endl;}
                sprintf(prot_mess,"RLI REG\n");
                n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                if (n == -1) {
                    exit(1);
                }

            } else{
            //verificar a pass
                    if(check_pass(file_pass,mess[2])){
                        //pass correta 
                        //enviar mensagem
                        if(verbose==1){cout << "Correct password" << endl;}

                        string file_login = "users/" + mess[1] +"/"+ mess[1]+"_login.txt";
                        ofstream outFile(file_login);
                        if (outFile.is_open()){
                            outFile << 1 << endl; 
                            outFile.close();
                            
                            sprintf(prot_mess,"RLI OK\n");
                            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                            if (n == -1) {
                                exit(1);
                            }
                            if(verbose==1){cout << "User "<< mess[1] <<" is logged in" << endl;}
                            
                        }else {
                            cerr << "Error opening file: " << file_login << endl;
                        }
                    }else{
                        //pass incorreta
                        if(verbose==1){cout << "Incorrect password: user not logged in" << endl;}
                        
                        sprintf(prot_mess,"RLI NOK\n");
                        n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                        if (n == -1) {
                            exit(1);
                        }
                        
                    }
            }
        }
    } else{
        send_err_udp();
    }
    
}

bool check_login(string username, string password) {
    // Check if username has 6 digits

    if (username.length() == 6) {
        bool validUser = true;
        for (char c : username) {
            if (!isdigit(c)) {
                validUser = false;
                break;
            }
        }
        if (validUser) {
            // Check if password has 8 alphanumeric characters
            if (password.length() == 8) {
                bool validPass = true;
                for (char c : password) {
                    if (!isalnum(c)) {
                        validPass = false;
                        break;
                    }
                }
                return validPass;
            }
        }
    }
    return false;
}
//----------------------------------------------------------------------------login------------------------------------------------------------------------------

//----------------------------------------------------------------------------logout------------------------------------------------------------------------------

void logout(vector<string> mess){
    memset(prot_mess,0,sizeof(prot_mess));
    if((mess.size() == 3) && check_login(mess[1],mess[2])){
        string path = "users/" + mess[1];

        //user não existe
        if (!user_exists(mess[1])){
            sprintf(prot_mess,"RLO UNR\n");
            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
            if (n == -1) {
                exit(1);
            }
            if(verbose ==1){cout << "User unknown" << endl;}
            
        }
        else{
            string file_login = "users/" + mess[1] +"/"+ mess[1] +"_login.txt";
                //user está logged in
                if(user_is_logged_in(mess[1])){
                    ofstream outFile(file_login);
                    if (outFile.is_open()) {  
                        outFile << 0 << endl; 
                        outFile.close();
                        sprintf(prot_mess,"RLO OK\n");
                        n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                        if (n == -1) {
                            exit(1);
                        }
                        if(verbose==1){ cout << "User "<<mess[1]<< " logged out successfully" << endl;}
                       
                    }
                }
                else{
                    //user não está logged in
                    sprintf(prot_mess,"RLO NOK\n");
                    n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                    if (n == -1) {
                        exit(1);
                    }
                    if(verbose==1){cout << "User is not logged in" << endl;}
                    
                }
            
        }
    } else {
        send_err_udp();
    }
}

//----------------------------------------------------------------------------logout------------------------------------------------------------------------------

//----------------------------------------------------------------------------unregister------------------------------------------------------------------------------

void unregister(vector<string> mess){
    memset(prot_mess,0,sizeof(prot_mess));
    if((mess.size() == 3) && check_login(mess[1],mess[2])){
        string path = "users/" + mess[1];
        string file_login = "users/" + mess[1] +"/"+ mess[1] +"_login.txt";
        string file_pass = "users/" + mess[1] +"/"+ mess[1]+"_pass.txt";
        
        //user not registered -> pasta não existe ou pasta existe mas files login e pass não-> RUR UNR
        if ((!user_exists(mess[1])) || (user_exists(mess[1]) && !fs::exists(file_pass) && !fs::exists(file_login))){
            sprintf(prot_mess,"RUR UNR\n");
            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
            if (n == -1) {
                exit(1);
            }
            if(verbose==1){cout << "User not registered" << endl;}
            
        }
        else if (fs::exists(file_pass) && fs::exists(file_login)){
            ifstream inFile(file_login);
                if(user_is_logged_in(mess[1])){
                    try {
                        fs::remove(file_login);
                        fs::remove(file_pass);
                    } 
                    catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                    sprintf(prot_mess,"RUR OK\n");
                    n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                    if (n == -1) {
                        exit(1);
                    }
                    if(verbose==1){cout << "User "<<mess[1]<<" successfully unregistered" << endl;}
                    
                }
                //user não está logged in
                else{
                    sprintf(prot_mess,"RUR NOK\n"); 
                    n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                    if (n == -1) {
                        exit(1);
                    }
                    if(verbose==1){cout << "User not logged in" << endl;}     
                }
        } else {
            send_err_udp();
        }
    } else {
        send_err_udp();
    }
}

//----------------------------------------------------------------------------unregister------------------------------------------------------------------------------

bool customSort(const fs::directory_entry& a, const fs::directory_entry& b) {
    // Extract the numeric part of the filenames
    int numberA = stoi(a.path().filename().stem().string());
    int numberB = stoi(b.path().filename().stem().string());

    return numberA < numberB;
}
//----------------------------------------------------------------------------myauction------------------------------------------------------------------------------

void myauctions(vector<string> mess){
    if(mess.size()==2){
        memset(prot_mess,0,sizeof(prot_mess));
        string path = "users/" + mess[1];
        string file_login = "users/" + mess[1] +"/"+ mess[1] +"_login.txt";
        string file_pass = "users/" + mess[1] +"/"+ mess[1]+"_pass.txt";
        string pathHosted = "users/" + mess[1] + "/hosted";
        string formattedString;
        if (fs::exists(file_pass) && fs::exists(file_login)){
                if(user_is_logged_in(mess[1])){
                    
                    if(fs::exists(pathHosted)){
                        
                        if (!fs::is_empty(pathHosted)){
                            
                            vector<fs::directory_entry> entries;
                            
                            for (const auto& entry_host : fs::directory_iterator(pathHosted)){
                                entries.push_back(entry_host);    
                            }

                            sort(entries.begin(), entries.end(), customSort);

                            for (const auto& entry_host : entries){
                                if (fs::is_regular_file(entry_host)) {
                                    
                                    //passa por cada file do hosted, ou seja, cada leilão do user
                                    check_endtime(entry_host.path().filename().stem().string(),mess[1]);
                                    
                                    string file_path = entry_host.path().string();
                                    ifstream inFile(file_path);
                                    if(inFile.is_open()){
                                        string line;
                                        getline(inFile, line);
                                        formattedString+= entry_host.path().filename().stem().string() + " " + line.c_str() + " ";
                                        inFile.close();
                                    }
                                }
                            }
                                  
                            sprintf(prot_mess, "RMA OK %s\n", formattedString.c_str());
                            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                            if (n == -1) {
                                exit(1);
                            }
                            if(verbose==1){cout << "List sent" << endl;}
                        }
                        else{
                            sprintf(prot_mess,"RMA NOK\n");
                            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                            if (n == -1) {
                                exit(1);
                            }
                            if(verbose==1){cout << "User has no ongoing auctions" << endl;}
                        }   
                    }
                }
                else{
                    sprintf(prot_mess,"RMA NLG\n");
                    n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                    if (n == -1) {
                        exit(1);
                    }
                    if(verbose==1){cout << "User is not logged in" << endl;}
                }
            }else{
                send_err_udp();
            }
        }
        else{
            send_err_udp();
        }
    }

//----------------------------------------------------------------------------myauction------------------------------------------------------------------------------

//----------------------------------------------------------------------------mybids------------------------------------------------------------------------------

void mybids(vector<string> mess){
    cout << "mybids: " << mess[1]<< endl;
    if(mess.size()==2){
        memset(prot_mess,0,sizeof(prot_mess));
        string path = "users/" + mess[1];
        string file_login = "users/" + mess[1] +"/"+ mess[1] +"_login.txt";
        string file_pass = "users/" + mess[1] +"/"+ mess[1]+"_pass.txt";
        string pathBidded = "users/" + mess[1] + "/bidded";
        string formattedString;
        if (fs::exists(file_pass) && fs::exists(file_login)){
                
                //user está logged in
                if(user_is_logged_in(mess[1])){
                    
                    if(fs::exists(pathBidded)){
                        
                        if (!fs::is_empty(pathBidded)){
                            vector<fs::directory_entry> entries;
                            for (const auto& entry_host : fs::directory_iterator(pathBidded)){
                                cout << pathBidded << endl;
                                entries.push_back(entry_host);    
                            }
                            sort(entries.begin(), entries.end(), customSort);
            
                            for (const auto& entry_host : entries){
                                if (fs::is_regular_file(entry_host)) {
                                    
                                    check_endtime(entry_host.path().filename().stem().string(),mess[1]);
                                    
                                    string path_auctions = "auctions/" + entry_host.path().filename().stem().string() + "/end_" + entry_host.path().filename().stem().string() + ".txt";
                                    if(fs::exists(path_auctions)){
                                        //auction já terminou
                                        formattedString += entry_host.path().filename().stem().string() + " 0 "; 
                                    } else{
                                        //auction ainda está ativo
                                        formattedString += entry_host.path().filename().stem().string() + " 1 "; 
                                    }
                                }
                            }   
                            sprintf(prot_mess, "RMB OK %s\n", formattedString.c_str());
                            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                            if (n == -1) {
                                exit(1);
                            }
                            if(verbose==1){cout << "List sent" << endl;}
                        }
                        else{
                            sprintf(prot_mess,"RMB NOK\n");
                            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                            if (n == -1) {
                                exit(1);
                            }
                            if(verbose==1){cout << "User has no ongoing auctions" << endl;}
                        }   
                    }
                }
                else{
                    sprintf(prot_mess,"RMB NLG\n");
                    n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
                    if (n == -1) {
                        exit(1);
                    }
                    if(verbose==1){cout << "User is not logged in" << endl;}
                }
            }else{
                send_err_udp();
            }
        }
        else{
            send_err_udp();
        }
}
    

//----------------------------------------------------------------------------mybids------------------------------------------------------------------------------


//-----------------------------------------------------------------------------list--------------------------------------------------------------------------------

void list(vector<string> mess){
    if(mess.size()==1){
        memset(prot_mess,0,sizeof(prot_mess));
        string path = "auctions";
        string formattedString;
        
        if(fs::is_empty(path)){
            sprintf(prot_mess, "RLS NOK");
            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
            if (n == -1) {
                exit(1);
            }
            if(verbose==1){cout << "There is no ongoing auctions" << endl;}
        }
        else{
            vector<fs::directory_entry> entries;
            for (const auto& entry_auction : fs::directory_iterator(path)){
                entries.push_back(entry_auction);    
            }
            sort(entries.begin(), entries.end(), customSort);

            for (const auto& entry_auction : entries){
                check_endtime(entry_auction.path().filename().string(),get_auc_user(entry_auction.path().stem().string()));
                
                string auctionAID = entry_auction.path().filename().string(); 
                string end_path = entry_auction.path().string() + "/end_" + auctionAID + ".txt";

                if(!fs::exists(end_path)){
                    formattedString+= auctionAID + " 1 ";
                }
                if(fs::exists(end_path)){
                    formattedString+= auctionAID + " 0 ";
                }
            }
            sprintf(prot_mess, "RLS OK %s\n", formattedString.c_str());
            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
            if (n == -1) {
                exit(1);
            }
            
            if(verbose==1){cout << "List sent" << endl;}
        }
    }
    else{
        send_err_udp();
    }
}


string get_auc_user(string aid){
    string path = "auctions/" + aid +"/start_" +aid +".txt";
    cout << "get_auc_user " << path << endl;
    ifstream inFile(path);
    if(inFile.is_open()){
        string line;
        getline(inFile, line);
        vector<string> args = splitWords(line);
        inFile.close();
        return args[0];
    }
    return "";
}

//-----------------------------------------------------------------------------list--------------------------------------------------------------------------------

//-----------------------------------------------------------------------------showrecord--------------------------------------------------------------------------------

bool sortShowRecord(const fs::directory_entry& a, const fs::directory_entry& b) {
    return a.path().filename().string() < b.path().filename().string();
}

void showrecord(vector<string> mess){
    
    if(mess.size()==2){
        
        int fileCount = 0;
        memset(prot_mess,0,sizeof(prot_mess));
        string path_auctions = "auctions";
        string formattedString;
        bool exists = false;
        
        for (const auto& entry_auction : fs::directory_iterator(path_auctions)){
                
                string auctionAID = entry_auction.path().filename().string();
                string path_bids = "auctions/" + auctionAID + "/bids";
                string start_path = entry_auction.path().string() + "/start_" + auctionAID + ".txt";
                
                ifstream inFile(start_path); 
                
                if(auctionAID==mess[1]){
                    //informação sobre o auction
                    exists=true;
                    if(inFile.is_open()){
                        string line1;
                        getline(inFile, line1);
                        istringstream iss(line1);
                        vector<string> info;
                        string word;
                        while (iss >> word) {
                            info.push_back(word);
                        }
                        formattedString += info[0]+ " " + info[1] + " " + info[2]+ " " + info[3]+ " " + info[5] + " "+ info[6]+ " " + info[4];
                        inFile.close();
                    }
                    if(fs::exists(path_bids)){
                        //informação sobre bids
                        vector<fs::directory_entry> entries;
                        for (const auto& entry_bids : fs::directory_iterator(path_bids)) {
                            entries.push_back(entry_bids);
                        }
                        sort(entries.begin(), entries.end(), sortShowRecord);
                        
                        int startIdx = max(static_cast<int>(entries.size()) - maxFiles, 0);
                        for (int i = startIdx; i < entries.size(); ++i) {
                            // Read content from each file
                            std::ifstream inFile(entries[i].path());
                            if (inFile.is_open()) {
                                std::string line2;
                                std::getline(inFile, line2);
                                if (!line2.empty()) {
                                    formattedString += " B " + line2;
                                }
                                inFile.close();
                            }

                            ++fileCount;
                        }
                    }
                    
                    string end_path = entry_auction.path().string() + "/end_" + auctionAID + ".txt";
                    if(fs::exists(end_path)){
                        ifstream inFile(end_path); 
                        if(inFile.is_open()){
                            string line3;
                            getline(inFile, line3);
                            formattedString+= " E " + line3;
                        } 
                        inFile.close();
                    }
                
                    break;

                }else{
                    exists=false;
                }
        }
        if(exists){
            sprintf(prot_mess, "RRC OK %s\n", formattedString.c_str());
            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
            if (n == -1) {
                exit(1);
            }
            if(verbose==1){cout << "Record sent" << endl;}
        }
        else{
            sprintf(prot_mess, "RRC NOK");
            n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
            if (n == -1) {
                exit(1);
            }
            if(verbose==1){cout << "AID does not exist" << endl;}
        }
    }else{
        send_err_udp();
    }
}


//-----------------------------------------------------------------------------showrecord--------------------------------------------------------------------------------

void close_auc(vector<string> mess){
    char *ptr;
    int nw;
    
    memset(prot_mess,0,sizeof(prot_mess));
    
    if((mess.size() == 4) && (check_close(mess[1],mess[2],mess[3]))){
        string user = "users/"+ mess[1];
        string pass = "users/" +mess[1] + "/" + mess[1] + "_pass.txt";
        string login = "users/" +mess[1] + "/" + mess[1] + "_login.txt";


        if(!user_exists(mess[1]) || (!(fs::exists(pass))) || !(check_pass(pass, mess[2]))){
            //user não existe ou password incorreta
            sprintf(prot_mess,"RCL NOK\n");
            n=strlen(prot_mess);
            ptr = &prot_mess[0];
            while(n>0){
                if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                n-=nw;
                ptr+=nw;
            }
            if(verbose == 1){
                cout << "User doesn't exist or incorrect password" << endl;
            }
            return;
        }
        else if(user_exists(mess[1]) && fs::exists(pass) && check_pass(pass, mess[2])){
            ifstream inFile(login);
            if(inFile.is_open()){
                //user não está logged in
                string line;
                getline(inFile, line);
                if(line == "0"){
                    sprintf(prot_mess,"RCL NLG\n");
                    n=strlen(prot_mess);
                    ptr = &prot_mess[0];
                    while(n>0){
                        if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                        n-=nw;
                        ptr+=nw;
                    }
                    inFile.close();
                    close(new_fd_tcp);
                    return; 
                }
                inFile.close();
            } //erro caso não dê para abrir file
            getAuctionAid();
            if(!check_aid(mess[3])){
                sprintf(prot_mess,"RCL EAU\n");
                n=strlen(prot_mess);
                ptr = &prot_mess[0];
                while(n>0){
                    if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                    n-=nw;
                    ptr+=nw;
                }
            } else if(!check_auc_owner(mess[3],mess[1])){
                sprintf(prot_mess,"RCL EOW\n");
                n=strlen(prot_mess);
                ptr = &prot_mess[0];
                while(n>0){
                    if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                    n-=nw;
                    ptr+=nw;
                }
            } else if(check_end(mess[3])){
                sprintf(prot_mess,"RCL END\n");
                n=strlen(prot_mess);
                ptr = &prot_mess[0];
                while(n>0){
                    if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                    n-=nw;
                    ptr+=nw;
                }
            } else{
                //fechar leilão criando o file end
                string path = "auctions/" + mess[3] +"/end_"+ mess[3]+".txt";
                
                int end_time = time(nullptr) - get_start_fulltime(mess[3]);
                string content = get_time() + " "+ to_string(end_time);
                
                create_file(path, content);
                //mudar content no users
                string path_hosted = "users/" + mess[1] + "/hosted/" + mess[3] + ".txt";
                ofstream outFile1(path_hosted);
                if(outFile1.is_open()){
                    outFile1 << "0" << endl;
                }    
                outFile1.close();

                sprintf(prot_mess,"RCL OK\n");
                n=strlen(prot_mess);
                ptr = &prot_mess[0];
                while(n>0){
                    if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                    n-=nw;
                    ptr+=nw;
                }
            }
        } else{
            send_err_tcp();
        }
        close(new_fd_tcp);
    
    } else{send_err_tcp();}
}

void check_endtime(string aid, string user){
    //fazer com que vá ao user e mude para 0
    int endtime = get_start_fulltime(aid) + get_start_timeactive(aid);

    if(time(nullptr) >= endtime){
        string path = "auctions/" + aid +"/end_"+ aid+".txt";
        
        create_file(path, convert_to_date(endtime) + " "+ to_string(get_start_timeactive(aid)));
        
        string path_hosted = "users/" + user + "/hosted/" + aid + ".txt";

        if(fs::exists(path_hosted)){
            //cout << "path_host: " << path_hosted << endl;
            ofstream outFile1(path_hosted);
            if(outFile1.is_open()){
                outFile1 << "0" << endl;
            }    
            outFile1.close();
        }
        
    }
}


string convert_to_date(time_t seconds) {
    tm *timeinfo = localtime(&seconds); // Convert seconds to local time

    stringstream ss;
    ss << put_time(timeinfo, "%Y-%m-%d %H:%M:%S");

    return ss.str();
}

int get_start_fulltime(string aid){
    string path = "auctions/" + aid +"/start_"+ aid+".txt";
    ifstream inFile(path);
    if(inFile.is_open()){
        string line;
        getline(inFile, line);
        vector<string> words = splitWords(line);
        return stoi(words[7]);
    }
    inFile.close();
    return 0;
}

int get_start_timeactive(string aid){
    string path = "auctions/" + aid +"/start_"+ aid+".txt";
    ifstream inFile(path);
    if(inFile.is_open()){
        string line;
        getline(inFile, line);
        vector<string> words = splitWords(line);
        return stoi(words[4]);
    }
    inFile.close();
    return 0;
}

bool check_pass(string file_pass, string pass){
    bool valid;
    ifstream inFile(file_pass);
    if(inFile.is_open()){
        string line;
        getline(inFile, line);
        if(line == pass){valid = true;}
        else {valid = false;}
    }
    inFile.close();
    cout << "valid: " << valid<< endl;
    return valid;
}

bool check_close(string uid, string pass, string aid){
    //probably tem de ser acrescentar mais coisas aqui
    string user = "users/" + uid;
    string pass_file = "users/" + uid + "/" + uid+"_pass.txt";
    if(!(uid != "") && !(pass != "") && !check_login(uid, pass)) {return false;}
    return true;
}

bool check_aid(string aid){
    //verificar ainda se o aid dado pelo user tem formato válido
    getAuctionAid();
    if((auctionAID >= 1) && (auctionAID <= 999) && (stoi(aid) <= auctionAID)){return true;}
    return false;
}

bool check_auc_owner(string aid, string user){
    string path = "users/" + user + "/hosted/" + aid +".txt";
    cout << "path check: " << path << endl;
    if(fs::exists(path)){return true;}
    return false;
}

bool check_end(string aid){
    string path = "auctions/" + aid + "/end_" + aid + ".txt";
    if(fs::exists(path)){return true;}
    return false;
}

//----------------------------------------------------------------------------open_auc------------------------------------------------------------------------------

void open_auc(vector<string> mess){
    char *ptr;
    int nw;
    memset(prot_mess,0,sizeof(prot_mess));
    string file_pass  = "users/" + mess[1] +"/"+ mess[1]+"_pass.txt";
    if(mess.size() == 9){mess.pop_back();}
    if(mess.size() == 8 && check_open(mess[3],mess[6],mess[4],mess[5]) && user_exists(mess[1]) && user_is_logged_in(mess[1]) && check_pass(file_pass, mess[2])){
        //no ckeck_open verificar se o user existe sequer e se o file de login está a 1
        int n = receive_file(mess[6],mess[7]);
        if(n == 0){
            cout << "Asset file wasn't received: couldn't create auction" << endl;
            sprintf(prot_mess,"ROA NOK\n");
            ptr = &prot_mess[0];
            while(n>0){
                if((nw = write(new_fd_tcp,ptr,n)) <= 0){
                    exit(1);
                    n-=nw;
                    ptr+=nw;
                }
            }
            send_err_tcp();
        } else{
            string path_hosted, path_auctions,path_asset,path_bids,dest_path,path_start,auction_str;
            getAuctionAid();
            auctionAID++;
            set_auc_aid(auctionAID);
            //colocar erro caso AID não esteja entre 000 e 999
            if(auctionAID<1000){
                if(auctionAID<10){
                    auction_str = "00" + to_string(auctionAID);
                }
                else if(auctionAID>=10){
                    auction_str = "0" + to_string(auctionAID);
                }
                else if(auctionAID>99){
                    auction_str = to_string(auctionAID);
                }
                path_hosted = "users/" + mess[1]+ "/hosted/" + auction_str + ".txt";
                path_auctions = "auctions/" + auction_str; 
                path_asset = "auctions/" + auction_str + "/asset";
                path_bids = "auctions/" + auction_str + "/bids";
                dest_path = "auctions/" + auction_str + "/asset/";
                path_start = "auctions/" + auction_str + "/start_" + auction_str +".txt";

                string str = mess[1]+ " " +mess[3]+ " "+ mess[6]+ " "+mess[4]+ " "+mess[5]+ " "+
                    get_time() +" " + to_string(time(nullptr));
                //não tenho a certeza se a última parte da string é só time
                
                int name = getpid();
                create_file(path_hosted,"1");
                create_dir(path_auctions);
                create_dir(path_asset);
                create_dir(path_bids);
                move_file(to_string(name).c_str(),dest_path, mess[6]);
                create_file(path_start,str);

                if(verbose == 1){
                    cout << "Auction " << auction_str << " started successfully by user " << mess[1] << endl;
                }

                sprintf(prot_mess,"ROA OK %s\n", auction_str.c_str());
                n=strlen(prot_mess);
                ptr = &prot_mess[0];
                while(n>0){
                    if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                    n-=nw;
                    ptr+=nw;
                }
            } else{
                sprintf(prot_mess,"ROA NOK\n");
                ptr = &prot_mess[0];
                n=strlen(prot_mess);
                while(n>0){
                    if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                    n-=nw;
                    ptr+=nw;
                    
                }
            }
            close(new_fd_tcp);
        }      
    }else {
        send_err_tcp();
    }
}

bool check_open(string name, string asset_fname, string start_value, string timeactive){
    if((name.length()>=1) && (name.length()<=10) && (asset_fname.length()>=4) && (asset_fname.length()<=24) && 
    (start_value.length() >= 1) && (start_value.length() <= 6) && (timeactive.length() >= 1) && (timeactive.length() <= 5)) {
        for(char c: name){
            if(!isalnum(c)){return false;}
        }
        for(char c: start_value){
            if(!isdigit(c)){return false;}
        }
        for(char c: timeactive){
            if(!isdigit(c)){return false;}
        }
        for(char c: asset_fname){
            if(!isalnum(c) && c!='.' && c!='-' && c!='_'){return false;}
        }
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------open_auc------------------------------------------------------------------------------

//----------------------------------------------------------------------------bid------------------------------------------------------------------------------

void bid(vector<string> mess){
    char *ptr;
    int nw;
    memset(prot_mess,0,sizeof(prot_mess));
    
    if(mess.size()==5){

        string path_auction = "auctions/" + mess[3];
        string end_path = path_auction + "/end_" + mess[3] + ".txt";
        string path_bids = path_auction + "/bids";
        string path_users = "users/" + mess[1];
        string file_login = "users/" + mess[1] +"/"+ mess[1] +"_login.txt";
        string file_pass  = "users/" + mess[1] +"/"+ mess[1]+"_pass.txt";
        string pathHosted = "users/" + mess[1] + "/hosted/" + mess[3] + ".txt";

        if (fs::exists(file_pass) && fs::exists(file_login) && check_pass(file_pass,mess[2])){
            ifstream inFile(file_login);
            
        
                //user está logged in
                if(user_is_logged_in(mess[1])){ //user is active
                    
                    if(!fs::exists(pathHosted)){
                        
                        check_endtime(mess[3], get_auc_user(mess[3]));
                        if(!fs::exists(end_path)){                       
                                                 
                            if(fs::is_empty(path_bids)){
                                
                                string str = mess[1] + " " + mess[4] + " "+ get_time()+ " "+to_string(time(nullptr)-get_start_fulltime(mess[3])); 
                                string bidName=createBidName(mess[4]);
                                string newBid = path_bids + "/" + bidName + ".txt";
                                string user_bid = path_users + "/bidded/" + mess[3] +".txt"; 
                                
                                create_file(newBid,str);
                                create_file(user_bid,"");

                                sprintf(prot_mess,"RBD ACC\n");
                                n=strlen(prot_mess);
                                ptr = &prot_mess[0];
                                while(n>0){
                                    if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                                    n-=nw;
                                    ptr+=nw;
                                }
                            }else{
                                for (const auto& entry_bids : fs::directory_iterator(path_bids)){ 
                                    
                                    string file_name=entry_bids.path().filename().stem().string();
                                    
                                    if(compareBids(file_name,mess[4])){
                                        
                                        string str = mess[1] + " " + mess[4] + " "+ get_time()+ " "+to_string(time(nullptr)-get_start_fulltime(mess[3])); 
                                        string bidName=createBidName(mess[4]);
                                        string newBid = path_bids + "/" + bidName + ".txt";
                                        
                                        create_file(newBid,str);
                                        
                                        sprintf(prot_mess,"RBD ACC\n");
                                        n=strlen(prot_mess);
                                        ptr = &prot_mess[0];
                                        while(n>0){
                                            if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                                            n-=nw;
                                            ptr+=nw;
                                        }                                       
                                    }else{
                                        //REF
                                        sprintf(prot_mess,"RBD REF\n");
                                        ptr = &prot_mess[0];
                                        n=strlen(prot_mess);
                                        while(n>0){
                                            if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                                            n-=nw;
                                            ptr+=nw;                                           
                                        }
                                    }
                                    break;
                                }
                            }                                                             
                        }else{
                            //NOK
                            sprintf(prot_mess,"RBD NOK\n");
                            ptr = &prot_mess[0];
                            n=strlen(prot_mess);
                            while(n>0){
                                if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                                n-=nw;
                                ptr+=nw;
                                
                            }
                        }
                    }else{
                        //ILG
                        sprintf(prot_mess,"RBD ILG\n");
                        ptr = &prot_mess[0];
                        n=strlen(prot_mess);
                        while(n>0){
                            if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                            n-=nw;
                            ptr+=nw;
                            
                        }
                    }

                }else{
                    //NLG
                    sprintf(prot_mess,"RBD NGL\n");
                    ptr = &prot_mess[0];
                    n=strlen(prot_mess);
                    while(n>0){
                        if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                        n-=nw;
                        ptr+=nw;
                        
                    }
                }
                
        }else {
            send_err_tcp();
        }
    }else {
        send_err_tcp();
    }
    close(new_fd_tcp); 
}

bool compareBids(string previous, string newBid){
    bool accepted = false;
    int previous_int = stoi(previous);
    int newBid_int = stoi(newBid);
    if(previous_int<newBid_int){
        accepted=true;
    }
    else{
        accepted=false;
    }
    return accepted;
}

string createBidName(string value){
    if(value.size()==1){
        value = "00000" + value;
        return value;
    }
    else if(value.size()==2){
        value = "0000" + value;
        return value;
    }
    else if(value.size()==3){
        value = "000" + value;
        return value;
    }
    else if(value.size()==4){
        value = "00" + value;
        return value;
    }
    else if(value.size()==5){
        value = "0" + value;
        return value;
    }
    else if(value.size()==6){
        return value;
    }
    else{
        return "";
    }
}

//----------------------------------------------------------------------------bid------------------------------------------------------------------------------

//----------------------------------------------------------------------------show_asset------------------------------------------------------------------------------


void show_asset(vector<string> mess){
    char *ptr;
    int nw;
    memset(prot_mess,0,sizeof(prot_mess));
    if(mess.size()==2){
        string aid_path = "auctions/" + mess[1];
        string start_path = "auctions/" + mess[1] + "/start_" + mess[1] + ".txt";
        if(fs::exists(aid_path)){
            ifstream inFile(start_path);
            if(inFile.is_open()){
                string line;
                getline(inFile, line);
                vector<std::string> stringVector; //perguntar sobre splitWords
                istringstream iss(line);
                copy(std::istream_iterator<std::string>(iss),istream_iterator<std::string>(),back_inserter(stringVector));
                string file_name = stringVector[2];
                string file_path = "auctions/" +mess[1]+"/asset/" + file_name;
                //if ficheiro existe
                size_t size = get_file_size(file_path);
                string file_size=to_string(size);
                sprintf(prot_mess,"RSA OK %s %s ",file_name.c_str(),file_size.c_str());
                cout << "File sent" << endl;
                ptr = &prot_mess[0];
                n=strlen(prot_mess);
                while(n>0){
                    if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                    n-=nw;
                    ptr+=nw;
                }
                
                int n=send_file(file_path,file_size);
                if(n==0){
                    memset(prot_mess,0,sizeof(prot_mess));
                    cout << "Asset file wasn't sent" << endl;
                    sprintf(prot_mess,"RSA NOK\n");
                    ptr = &prot_mess[0];
                    n=strlen(prot_mess);
                    while(n>0){
                        if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
                        n-=nw;
                        ptr+=nw;
                        
                    }
                    send_err_tcp(); 
                }
                close(new_fd_tcp);
            }else{
                send_err_tcp();
            }
        }else{ //o estado fica NOK aqui tambem?
            cout << "AID does not exist" << endl;
            send_err_tcp();
        }
    }else{
        send_err_tcp();
    }
}
    
int send_file(const std::string& file_name, string file_size) {

    // Open the file for reading
    FILE* inputFile = fopen(file_name.c_str(), "rb");
    if (!inputFile) {
        exit(1);
    }

    // Send the file
    ssize_t n;
    
    while ((n = fread(buffer, 1, sizeof(buffer), inputFile)) > 0) {
        if (write(new_fd_tcp, buffer, n) <= 0) {
            exit(1);
        }
        memset(buffer, 0, sizeof(buffer));
    }

    fclose(inputFile);
    return 1;
}

size_t get_file_size(const std::string& filename) {
    struct stat file_info;
    if (stat(filename.c_str(), &file_info) == 0) {
        return file_info.st_size;
    } else {
        perror("Error getting file size");
        return 0;  // Return 0 or another sentinel value to indicate an error
    }
}

//----------------------------------------------------------------------------show_asset------------------------------------------------------------------------------

//----------------------------------------------------------------------------create_user------------------------------------------------------------------------------

void create_user(string path, string username, string password){
    string file_pass = "users/" + username +"/"+ username + "_pass.txt";
    string file_login = "users/" + username +"/"+ username +"_login.txt";
    string hosted = path + "/hosted";
    string bidded = path + "/bidded";
    
    create_dir(path);
    create_dir(hosted);
    create_dir(bidded);

    create_file(file_pass,password);

    //ativo - 1; não ativo - 0
    ofstream outFile2(file_login);
    if(outFile2.is_open()){
        outFile2 << 1 << endl;
        outFile2.close();
    }else{
        cerr << "Error creating file: " << file_login << endl;
    }
}

//----------------------------------------------------------------------------create_user------------------------------------------------------------------------------

//----------------------------------------------------------------------------SplitWords------------------------------------------------------------------------------

vector<string> splitWords(const string& buffer) {
    //fazer com que verifique que só tem um espaço entre palavras
    istringstream iss(buffer);
    vector<string> words;
    string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

//----------------------------------------------------------------------------SplitWords------------------------------------------------------------------------------

//---------------------------------------------------------------------------get_time-----------------------------------------------------

string get_time() {
    time_t fulltime = time(nullptr);
    tm *currenttime = gmtime(&fulltime);

    stringstream ss;
    ss << setfill('0') << setw(4) << currenttime->tm_year + 1900 << '-';
    ss << setw(2) << currenttime->tm_mon + 1 << '-';
    ss << setw(2) << currenttime->tm_mday << ' ';
    ss << setw(2) << currenttime->tm_hour << ':';
    ss << setw(2) << currenttime->tm_min << ':';
    ss << setw(2) << currenttime->tm_sec;

    return ss.str();
}

//---------------------------------------------------------------------------get_time---------------------------------------------------------

//------------------------------------------------------------------------file_and_directory--------------------------------------------------

void move_file(string filename, string dest_path, string name){
    try {
        // Check if the source file exists
        if (fs::exists(filename)) {
            // Construct the destination path by appending the source file name
            fs::path destination = fs::path(dest_path) / fs::path(name).filename();

            // Move the file
            fs::rename(filename, destination);

        } else {
            cerr << "Source file does not exist." << endl;
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error moving file: " << e.what() << endl;
    }
}

void create_file(string path, string content){
    ofstream outFile1(path);
    if(outFile1.is_open()){
        if(!content.empty()){
            outFile1 << content << endl;
        }
        outFile1.close();
    }else{
        cerr << "Error creating file: " << path << endl;
    }
}

void create_dir(string path){
    if(!(fs::exists(path) && fs::is_directory(path))){
        try {
            fs::create_directory(path);
        } catch (const filesystem::filesystem_error& e) {
            cerr << "Error creating directory: " << e.what() << endl;
        }
    } 
}

int receive_file(string filename, string file_size){
    int temp = getpid();
    //FILE* file = fopen(filename.c_str(), "wb");
    FILE* file = fopen(to_string(temp).c_str(), "wb");
    if (!file) {
        perror("Error opening file");
        return 0;
    }
    ssize_t n, bytesRead = 0;
    char buffer[1024];
    n=1;
    while (bytesRead < stoi(file_size) && n!=0) {
        n = read(new_fd_tcp, buffer, sizeof(buffer));
        if(n<0){return 0;}
        bytesRead += n;
        fwrite(buffer, 1, n, file);
    }

    fclose(file);
    return 1;
}

//------------------------------------------------------------------------file_and_directory--------------------------------------------------

//----------------------------------------------------------------------------AID--------------------------------------------------

void set_auc_aid(int auctionAID){
    const string filename = "AID.txt";
    ofstream outputFile(filename);
    if (outputFile.is_open()) {
        outputFile << auctionAID;
        outputFile.close();
    } else {
        cerr << "Error opening file: " << filename << endl;
    }

}

void getAuctionAid(){
    const string filename = "AID.txt";
    
    ifstream inputFile(filename);

    if (!inputFile.is_open()) {
        // The file doesn't exist, so create it and write the default value
        ofstream outputFile(filename);
        if (outputFile.is_open()) {
            outputFile << 0;
            auctionAID = 0;
            outputFile.close();
        } else {
            cerr << "Error creating file.\n";
        }
    } else {
        // The file exists, so read its contents
        inputFile >> auctionAID;
        inputFile.close();
    }
}

//----------------------------------------------------------------------------AID--------------------------------------------------

//----------------------------------------------------------------------------AID_exists------------------------------------------------

bool AID_exists(string AID){
    string aid_path = "auctions/" + AID;
    if(fs::exists(aid_path)){
        return true;
    }
    else{
        return false;
    }
}

//----------------------------------------------------------------------------AID_exists------------------------------------------------

//----------------------------------------------------------------------------user_exists------------------------------------------------

bool user_exists(string user_ID){
    string user_path = "users/" + user_ID;
    if(fs::exists(user_path)){
        return true;
    }
    else{
        return false;
    }

}


//----------------------------------------------------------------------------user_exists------------------------------------------------

//----------------------------------------------------------------------------user_is_logged_in------------------------------------------------

bool user_is_logged_in(string user_ID){
    string user_path = "users/" + user_ID + "/" + user_ID + "_login.txt";
    ifstream inFile(user_path);
            if(inFile.is_open()){
                string line;
                getline(inFile, line);
                //user está logged in
                if(line == "1"){
                    return true;
                }
                else{
                    return false;
                }
            }
}




//----------------------------------------------------------------------------user_is_logged_in-----------------------------------------------



//----------------------------------------------------isNumeric------------------------------------------------------------------------

bool isNumeric(const std::string& str) {
    try {
        size_t pos;
        std::stod(str, &pos);
        return pos == str.length();  // Check if the entire string was consumed
    } catch (const std::invalid_argument&) {
        return false;  // Conversion failed
    } catch (const std::out_of_range&) {
        return false;  // Value is out of range
    }
}


//----------------------------------------------------isNumeric------------------------------------------------------------------------

//-----------------------------------------------------------------------------UDP---------------------------------------------------------

void create_udp(){
    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_udp == -1) {
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if (errcode != 0) {
        exit(1);
    }

    n = bind(fd_udp, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }
}

void receive_udp(){
    vector<string> words;
        
    addrlen = sizeof(addr);
    /*lê do socket para o buffer*/
    n = recvfrom(fd_udp, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
        exit(1);
    }

    char* newlinePos = strchr(buffer, '\n');
    if (newlinePos != nullptr) {
        *newlinePos = '\0';  // Replace '\n' with '\0'
    }
    words = splitWords(buffer);        
    
    if(words[0] == lin_str){
        login(words);
    } else if (words[0] == lou_str){
        logout(words);
    } else if(words[0] == unr_str){
        unregister(words);
    } else if(words[0] == "LMA"){
        myauctions(words);
    } else if(words[0] == "LMB"){
        mybids(words);
    } else if(words[0] == "LST"){
        list(words);
    } else if(words[0] == "SRC"){
        showrecord(words);
    }
    else{
        cout << "Unknown message" << endl;
        send_err_udp();
    }
}

//-----------------------------------------------------------------------------UDP---------------------------------------------------------

//----------------------------------------------------------------------------TCP---------------------------------------------------------

void create_tcp(){
    fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if ((errcode) != 0) {
        exit(1);
    }

    n = bind(fd_tcp, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }

    if (listen(fd_tcp, 5) == -1) {
        exit(1);
    }
}

void receive_tcp(){
    memset(buffer,0,sizeof(buffer));
    int i = 0, args_received = 0;
    char help[256];
    bool opa_com = false;
    string opa = "OPA ";
    while((n=read(new_fd_tcp,buffer,1))!=0){
        
        help[i] = buffer[0];
        //cout << help[i] << endl;
        if(n==-1)/*error*/exit(1);
        if(help[i]==' '){
            args_received++;
        }

        if((i >= 1) && (help[i] == ' ') && (help[i-1] == ' ')){
            send_err_tcp();
        }
        
        //já recebemos todos os argumentos, só falta o ficheiro
        if(args_received == 8){break;}
        if(help[i] == '\n'){break;}
        i++; 
    }

    vector<string> words;
    
    char* newlinePos = strchr(help, '\n');
    if (newlinePos != nullptr) {
        *newlinePos = '\0';  // Replace '\n' with '\0'
    }
    
    //caso dê problemas a ler o open é por causa do split e temos de fazer popback
    words = splitWords(help);

    //if(args_received == 8){words.pop_back();}
    
    //usar o file size para ele ler só até isso
    //se o tamanho fornecido não for aquele do file: file de 10 000 e dizemos que tem 5000->para de ler aí: vemos que não tem /n e dá erro, ou se com virmos que o que lemos do buffer ultrapasssa o tamanho do file tbm dá erro
    //file de 5000 e dizemos que tem 10 000 -> vai ficar à espera para sempre, colocar timeout no socket para isso não acontecer
    //escrever primeiro num ficheiro temporário com um nome qualquer e depois então se correr tudo bem mover para o sítio certo
    
    if(words[0] == "OPA"){
        open_auc(words);
    } else if(words[0] == "CLS"){
        close_auc(words);
    } else if(words[0] == "BID"){
        bid(words);
    } else if(words[0] == "SAS"){
        show_asset(words);
    } else{
        cout << "Unknown message" << endl;
        send_err_tcp();
    }
}

//-----------------------------------------------------------------------------TCP---------------------------------------------------------

void send_err_udp(){
    sprintf(prot_mess,"ERR\n");
    n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, (struct sockaddr*)&addr, addrlen);
    if (n == -1) {
        exit(1);
    }
}

void send_err_tcp(){
    int nw;
    sprintf(prot_mess,"ERR\n");
    ptr = &prot_mess[0];
    n=strlen(prot_mess);
    while(n>0){
        if((nw = write(new_fd_tcp,ptr,n)) <= 0){exit(1);}
        n-=nw;
        ptr+=nw;
        
    }
}