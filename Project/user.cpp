#include "user.hpp"

string port;
string asIP;
 //0-username, 1-password, 2-logged in

// create a vector with the username, password and a an active bit that says if the user is logged in or not
// we can do that cause this file is for each user
//c_str
//cada vez que mandamos cenas por tcp temos de criar sockets novos
//mudar o tamanho que usamos para as cenas consoante os tamanhos que és suposto ter cada coisa
//fread write
//read de blocos di socket e fwrite no ficheiro

//fazer socket udp cada vez que queremos nova comunicação e fechar quando ela termina

//usar sendfile para enviar ficheiro
int fd_udp, errcode;
int fd_tcp;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[1024]; 
char prot_mess[1024];

string user[3];

int expect_reply;

int main(int argc, char *argv[]){
    
    std::signal(SIGINT, signalHandler);
    
    string defPort = "58062";  // porto para testar no tejo
    string defIP = "localhost";
    
    port = defPort;
    asIP = defIP;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            // Use the next argument as AS IP address
            asIP = argv[i + 1];
            i++;  // Skip the next argument
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            // Use the next argument as AS port
            port = argv[i + 1];
            i++;  // Skip the next argument
        }
    }


    while(true){
        vector<string> words;
        //como fazemos para o programa esperar aqui caso ainda não tenhamos recebido nada?

        while(fgets(buffer,128,stdin)==NULL){};
        //dá seg fault caso só seja \n, resolver
        
        if(strcmp(buffer, "\n") == 0){continue;}

        char* newlinePos = strchr(buffer, '\n');
        if (newlinePos != nullptr) {
            *newlinePos = '\0';  // Replace '\n' with '\0'
        }

        words = splitWords(buffer);

        if(words[0] == login_str){
            login(words);
        } else if (words[0] == logout_str) {
            logout();
        } else if (words[0] == unregister_str) {
            unregister();
        } else if (words[0] == exit_str) {
            exitProgram();
        } else if (words[0] == open_str) {
            open(words);
        } else if (words[0] == close_str) {
            close_auc(words);
        } else if (words[0]== ma_str || words[0]== myauctions_str) {
            myauctions();
        } else if (words[0]== mb_str || words[0]== mybids_str) {
            mybids();
        } else if (words[0]== list_str || words[0]== l_str) {
            list();
        } else if (words[0]== show_record_str || words[0]== sr_str) {
            show_records(words);
        } else if (words[0] == bid_str || words[0] == b_str) {
            bid(words);
        } else if (words[0] == show_asset_str || words[0] == sa_str) {
            show_asset(words);
        } else {
            // Handle the case where the token doesn't match any predefined string
            cout << "Unknown command: " << words[0] << endl;
            continue;
        }

        if(expect_reply==0){continue;}
        //dividir argumentos recebidos
        words = splitWords(buffer);
        cout << "buffer: " << buffer << endl;
        //mostrar mensagem de acordo com a resposta do servidor
        receive_reply(words);
    }
}

//----------------------------------------------------------------------------SplitWords------------------------------------------------------------------------------

vector<string> splitWords(const string& buffer) {
    istringstream iss(buffer);
    vector<string> words;
    string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}
//----------------------------------------------------------------------------SplitWords------------------------------------------------------------------------------

//----------------------------------------------------------------------------receive_reply-------------------------------------------------------------------------------

void receive_reply(vector<string> resp){
    if(resp[0]=="ERR"){
        cout<<"An error has ocurred"<<endl;
    }
    else if(resp[0] == rli_str){
        if(resp[1] == "OK"){
            cout << "User " << user[0] << " is logged in" << endl;
            user[2] = "1";
        }
        else if(resp[1] == "NOK"){
            cout << "User " << user[0] << " not logged in: incorrect password" << endl;
        }
        else if(resp[1] == "REG"){
            cout << "User " << user[0] << " is registered and logged in" << endl;
            user[2] = "1";
        }
    }
    else if(resp[0] == rlo_str){
        if(resp[1] == "OK"){
            cout << "User " << user[0] << " is logged out" << endl;
            user[2] = "0";
        }
        else if(resp[1] == "NOK"){
            cout << "User " << user[0] << " not logged in" << endl;
        }
        else if(resp[1] == "UNR"){
            cout << "User " << user[0] << " is unknown" << endl;
            user[2] = "1";
        }
    } 
    else if(resp[0]== rur_str){
        if(resp[1] == "OK"){
            cout << "User " << user[0] << " is unregistered" << endl;
            user[2] = "0";
        }
        else if(resp[1] == "NOK"){
            cout << "Unknown user, user not logged in" << endl;
        }
        else if(resp[1] == "UNR"){
            cout << "Incorrect unregister attempt" << endl;
            //user[2] = "1";
        }
    }
    else if(resp[0] == "ROA"){
        if(resp[1] == "NOK"){
            cout << "Auction could not be started" << endl;
        }
        else if(resp[1] == "NLG"){
            cout << "User not logged in " << endl;
        }
        else if(resp[1] == "OK"){
            cout << "Auction " << resp[2] << " started" << endl;
        }
    }

    else if(resp[0] == "RCL"){
        if(resp[1] == "OK"){
            cout << "Auction was closed successfully"<< endl;
        }
        else if(resp[1] == "NOK"){
            cout << "Auction could not be closed: user doesn't exist or incorrect password" << endl;
        }
        else if(resp[1] == "NLG"){
            cout << "Auction could not be closed: user not logged in" << endl;
        }
        else if(resp[1] == "EAU"){
            cout << "Auction could not be closed: Invalid auction ID" << endl;
        }
        else if(resp[1] == "EOW"){
            cout << "Auction could not be closed: user is not the auction's owner" << endl;
        } 
        else if(resp[1] == "END"){
            cout << "Auction could not be closed: auction already ended" << endl;
        }
    }
    else if(resp[0] == "RMA"){
        if(resp[1] == "OK"){
            cout << "List of auctions by user " << user[0]<< " : ";
            for (int i = 2; i < resp.size(); ++i) {
                    if(resp[i] == "1"){
                        cout << "active; ";
                    } else if(resp[i] == "0"){
                        cout << "closed; ";
                    } else {
                        cout << resp[i] << ": ";
                    }
                
                }
            cout << endl;
        }
        else if(resp[1] == "NOK"){
            cout << "User " <<user[0] <<" hasn't started any auctions" << endl;
        }
    } 
    else if(resp[0] == "RBD"){
        if(resp[1] == "ACC"){
            cout << "Bid was accepted" << endl;
        }
        else if(resp[1] == "NOK"){
            cout << "Auction AID is not active" << endl;
        }
        else if(resp[1] == "NLG"){
            cout << "User not logged in" << endl;
        }
        else if(resp[1] == "ILG"){
            cout << "Bid is hosted by this user" << endl;
        }
        else if(resp[1] == "REF"){
            cout << "Bid needs to be higher than previous bids" << endl;
        }
    } 
    else if(resp[0] == rmb_str){ 

        if(resp[1] == "OK"){
            cout << "Bids by user " << user[0] << ": ";
            for (int i = 2; i < resp.size(); ++i) {
                if(resp[i] == "1"){
                    cout << "active; ";
                } else if(resp[i] == "0"){
                    cout << "closed; ";
                } else {
                    cout << resp[i] << ": ";
                }
            }
            cout << endl;
        }
        else if(resp[1] == "NOK"){
            cout << "User is not involved in any bids" << endl;
        }
    }
    else if(resp[0] == rls_str){
        if(resp[1] == "OK"){
            cout << "List of auctions: ";
            for (int i = 2; i < resp.size(); ++i) {
                if(resp[i] == "1"){
                    cout << "active; ";
                } else if(resp[i] == "0"){
                    cout << "closed; ";
                } else {
                    cout << resp[i] << ": ";
                }
                
            }
            cout << endl;
        }
        else if(resp[1] == "NOK"){
            cout << "No auction has yet started" << endl;
        }
    }

    else if(resp[0]==rrc_str){
        if(resp[1] == "OK"){
            for (int i = 2; i < resp.size(); ++i) {
                    cout << resp[i] << " ";
                }
            cout << endl;
        }
        else if(resp[1] == "NOK"){
            cout << "Auction ID does not exist" << endl;
        }
    } else if(resp[0] == "RSA"){
        
        if(resp[1] == "OK"){
            cout << "Asset " << resp[2] << " with size " << resp[3] << " received successufully" << endl;
        }
        else if(resp[1] == "NOK"){
            cout << "File was not received" << endl;
        }
    }

}

//----------------------------------------------------------------------------receive_reply-------------------------------------------------------------------------------

//----------------------------------------------------------------------------login-------------------------------------------------------------------------------
void login(vector<string> arguments){  
    memset(prot_mess,0,sizeof(prot_mess));
    if(arguments.size() >= 3){
        //verificar se o formato está correto
        if(check_login(arguments[1],arguments[2])){
            if(user[2] == "1"){
                //user já logged in faz login outra vez
                if(arguments[1] == user[0]){
                    cout << "User " << user[0] << " already logged in" << endl;
                }else{
                    //outro user tenta fazer login
                    cout << "Another user is logged in, please logout first" << endl;
                }
                expect_reply=0;
               
            }
            //nenhum user logged in
            else{
                open_socket_udp();
                sprintf(prot_mess, "LIN %s %s\n", arguments[1].c_str(), arguments[2].c_str());
                n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, res->ai_addr, res->ai_addrlen);
                if (n == -1) {
                    exit(1);
                }
                user[0] = arguments[1];
                user[1] = arguments[2];
                user[2] = "0";
                expect_reply=1;
                wait_reply_udp();
            }
        }
        else{
            cout << "Incorrect login format" << endl;
            expect_reply = 0;
        }
    } else{
        cout << "Incorrect login format 1:" << arguments.size() << endl;
        expect_reply=0;
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

//----------------------------------------------------------------------------login-------------------------------------------------------------------------------

//----------------------------------------------------------------------------logout-------------------------------------------------------------------------------
void logout(){
    memset(prot_mess,0,sizeof(prot_mess));
    if(user[2]=="1"){
        open_socket_udp();
        sprintf(prot_mess, "LOU %s %s\n", user[0].c_str(), user[1].c_str()); 
        n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            exit(1);
        }
        expect_reply=1;
        wait_reply_udp();
    }
    else{
        cout << "User not logged in" << endl; 
        expect_reply=0;
    }
}

//----------------------------------------------------------------------------logout-------------------------------------------------------------------------------

//----------------------------------------------------------------------------unregister-------------------------------------------------------------------------------
void unregister(){
    memset(prot_mess,0,sizeof(prot_mess));
    if(user[2]=="1"){
        //devia fzr aqui logout? se fizer, depois o server não consegue ver se esta logged in e nunca tenho estado ok, será sempre NOK acho eu....
        open_socket_udp();
        sprintf(prot_mess, "UNR %s %s\n", user[0].c_str(), user[1].c_str()); 
        n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            exit(1);
        }
        expect_reply=1;
        wait_reply_udp();
    }
    else{
        cout << "User not logged in" << endl; 
        expect_reply=0;
    }
}

//----------------------------------------------------------------------------unregister-------------------------------------------------------------------------------

//-----------------------------------------------------------------------------exit------------------------------------------------------------------------------
void exitProgram(){
    expect_reply=0;
    if(user[2]=="1"){
        cout << "Please log out first" << endl;
    }
    else{
        exit(1);
    }
}
//-----------------------------------------------------------------------------exit------------------------------------------------------------------------------

//-----------------------------------------------------------------------------open------------------------------------------------------------------------------
void open(vector<string> mess){
    memset(prot_mess,0,sizeof(prot_mess));
    expect_reply=0;
    //colocar expect_reply = 1 onde corre tudo bem
    if(user[2] != "1"){
        cout << "Unsuccessful open: no user logged in" << endl;
        return;
    }
    //name=mess[1]
    //asset_fname=mess[2]
    //start_value=mess[3]
    //timeactive=mess[4]
    if(mess.size() >= 5 && check_open(mess[1], mess[2], mess[3], mess[4])){
        //função file_size para ver tamanho??
        
        uintmax_t file_size = filesystem::file_size(mess[2]);
        if(file_size== 0){
            cout << "Invalid asset file" << endl;
            return;
        }
        
        sprintf(prot_mess, "OPA %s %s %s %s %s %s %s " , user[0].c_str(), user[1].c_str(), mess[1].c_str(), mess[3].c_str(), 
            mess[4].c_str(), mess[2].c_str(), to_string(file_size).c_str());

        cout << prot_mess<<endl;
        open_socket_tcp();
        //send protocol message
        ssize_t nwritten, nleft;
        char* prot_mess_ptr = prot_mess;
        nleft = strlen(prot_mess);
        while (nleft > 0) {
            nwritten = write(fd_tcp, prot_mess_ptr, 1);
            if (nwritten <= 0) { 
                exit(1); 
            }
            nleft -= nwritten;
            prot_mess_ptr += nwritten;  // Move the pointer forward
        }

        //send asset file
        send_file(mess[2].c_str(),file_size);

        expect_reply =1;
        wait_reply_tcp();

    } else{
        cout << "Invalid Open Format" << endl;
    }
}

void send_file(string file_name, ssize_t file_size){
    // Open the file for reading
    FILE* inputFile = fopen(file_name.c_str(), "rb");
    if (!inputFile) {
        exit(1);
    }

    // Send the file
    ssize_t n;
    
    while ((n = fread(buffer, 1, sizeof(buffer), inputFile)) > 0) {
        if (write(fd_tcp, buffer, n) <= 0) {
            exit(1);
        }
        memset(buffer, 0, sizeof(buffer));
    }

    fclose(inputFile);
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

//-----------------------------------------------------------------------------open------------------------------------------------------------------------------

//-----------------------------------------------------------------------------close------------------------------------------------------------------------------

void close_auc(vector<string> mess){
    //close AID
    memset(prot_mess,0,sizeof(prot_mess));
    expect_reply = 0;
    if(user[2] != "1" && (user[0] != "") && (user[1] != "")){
        cout << "Unsuccessful close: no user logged in" << endl;
        return;
    }
    
    if((mess.size() >=2) && (mess[1].length() == 3)){
        for(char c: mess[1]){
            if(!isdigit(c)){
                cout<<"Invalid AID format"<<endl;
                return;
            }
        }
        sprintf(prot_mess,"CLS %s %s %s\n",user[0].c_str(), user[1].c_str(), mess[1].c_str());
        cout << prot_mess << endl;
        open_socket_tcp();

        ssize_t nwritten, nleft;
        char* prot_mess_ptr = prot_mess;
        //fazer if caso o pointer dê erro
        nleft = strlen(prot_mess);
        while (nleft > 0) {
            nwritten = write(fd_tcp, prot_mess_ptr, 1);
            if (nwritten <= 0) { 
                exit(1); 
            }
            nleft -= nwritten;
            prot_mess_ptr += nwritten;  // Move the pointer forward
        }

        expect_reply = 1;
        wait_reply_tcp();
    } else{
        cout << "Invalid close format"<<endl;
    }
}

//-----------------------------------------------------------------------------close------------------------------------------------------------------------------

//----------------------------------------------------------------------------myauctions-------------------------------------------------------------------------------
void myauctions(){
    if(user[2]=="1"){
        open_socket_udp();
        sprintf(prot_mess, "LMA %s\n", user[0].c_str());
        n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            exit(1);
        }
        expect_reply=1;
        wait_reply_udp();
    }
    else{
        cout << "User not logged in" << endl; 
        expect_reply=0;
    }
}

//----------------------------------------------------------------------------myauctions-------------------------------------------------------------------------------

//----------------------------------------------------------------------------mybids------------------------------------------------------------------------------

void mybids(){
    if(user[2]=="1"){
        open_socket_udp();
        sprintf(prot_mess, "LMB %s\n", user[0].c_str());
        n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            exit(1);
        }
        expect_reply=1;
        wait_reply_udp();
    }
    else{
        cout << "User not logged in" << endl; 
        expect_reply=0;
    }
}


//----------------------------------------------------------------------------mybids------------------------------------------------------------------------------

//----------------------------------------------------------------------------list------------------------------------------------------------------------------

void list(){
    open_socket_udp();
    sprintf(prot_mess, "LST\n");
    n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            exit(1);
        }
    expect_reply=1;
    wait_reply_udp();
}

//----------------------------------------------------------------------------list------------------------------------------------------------------------------

//----------------------------------------------------------------------------show_record------------------------------------------------------------------------------

void show_records(vector<string> arguments){ //não se se tenho de verificar alguma coisa antes de mandar a mensagem, acho que não pk o NOK é mesmo para ver se existe aid
    open_socket_udp();
    if(strlen(arguments[1].c_str())==3){
        sprintf(prot_mess, "SRC %s\n", arguments[1].c_str());
        n = sendto(fd_udp, prot_mess, strlen(prot_mess),0, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            exit(1);
        }
        memset(prot_mess,0,sizeof(prot_mess));
        expect_reply=1;
        wait_reply_udp();
    }
    else{
        cout << "Incorrect AID format" << endl;
    }
    
}

//----------------------------------------------------------------------------show_record------------------------------------------------------------------------------

//----------------------------------------------------------------------------bid------------------------------------------------------------------------------

void bid(vector<string> mess){ 
    memset(prot_mess,0,sizeof(prot_mess));
    expect_reply=0;
    if(user[2] != "1"){
        cout << "Unsuccessful bid: no user logged in" << endl;
        return;
    }
    if (mess.size()==3 && mess[2].size()<=6){ 
            sprintf(prot_mess, "BID %s %s %s %s\n", user[0].c_str(), user[1].c_str(), mess[1].c_str(), mess[2].c_str());
            open_socket_tcp();
            ssize_t nwritten, nleft;
            char* prot_mess_ptr = prot_mess;
            nleft = strlen(prot_mess);
            while (nleft > 0) {
                nwritten = write(fd_tcp, prot_mess_ptr, 1);
                if (nwritten <= 0) { 
                    exit(1); 
                }
                nleft -= nwritten;
                prot_mess_ptr += nwritten;  
            }
            expect_reply =1;
            wait_reply_tcp();
    }else{
        cout << "Incorrect input format" << endl;
    }
}

//----------------------------------------------------------------------------bid------------------------------------------------------------------------------

//----------------------------------------------------------------------------show_asset------------------------------------------------------------------------------

void show_asset(vector<string> mess){
    memset(prot_mess,0,sizeof(prot_mess));
    expect_reply=0;
    if (mess.size()==2){ 
            sprintf(prot_mess, "SAS %s\n", mess[1].c_str());
            open_socket_tcp();
            ssize_t nwritten, nleft;
            char* prot_mess_ptr = prot_mess;
            nleft = strlen(prot_mess);
            while (nleft > 0) {
                nwritten = write(fd_tcp, prot_mess_ptr, 1);
                if (nwritten <= 0) { 
                    exit(1); 
                }
                nleft -= nwritten;
                prot_mess_ptr += nwritten;  
            }
            expect_reply =1;
            wait_reply_tcp();
    }else{
        cout << "Incorrect input format" << endl;
    }
}

//----------------------------------------------------------------------------show_asset------------------------------------------------------------------------------

int receive_file(string filename, string file_size){
    
    FILE* file = fopen("text.jpg", "wb");
    if (!file) {
        perror("Error opening file");
        return 0;
    }
    ssize_t n, bytesRead = 0;
    char buffer[1024];
    n=1;
    while (bytesRead < stoi(file_size) && n!=0) {
        n = read(fd_tcp, buffer, sizeof(buffer));
        if(n<0){return 0;}
        bytesRead += n;
        fwrite(buffer, 1, n, file);
    }

    fclose(file);
    return 1;
}

//----------------------------------------------------------------------------aux_udp-------------------------------------------------------------------------------

void open_socket_udp(){
    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_udp == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    //não sei se isto está correto
    errcode = getaddrinfo(asIP.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0) {
        exit(1);
    }
}

void close_socket_udp(){
    freeaddrinfo(res);
    close(fd_udp);
}

void wait_reply_udp(){
    //"limpar" buffer
    memset(buffer, 0, sizeof(buffer));
    //receber resposta do server
    addrlen = sizeof(addr);
    n = recvfrom(fd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
        exit(1);
    }

    //remover \n
    char *newlinePos = strchr(buffer, '\n');
    if (newlinePos != nullptr) {
        *newlinePos = '\0'; 
    }

    //terminar ligação udp
    close_socket_udp();
}

//----------------------------------------------------------------------------aux_udp-------------------------------------------------------------------------------

//----------------------------------------------------------------------------aux_tcp-------------------------------------------------------------------------------

void open_socket_tcp(){
    fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    errcode = getaddrinfo(asIP.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0) {
        exit(1);
    }
    
    //server fica à espera no accept até um pedido de connect do client
    n = connect(fd_tcp, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }
}

void close_socket_tcp(){
    freeaddrinfo(res);
    close(fd_tcp);
}

void wait_reply_tcp(){
    bool rsa_com = false;
    string rsa = "RSA ";
    char help[256];
    int i = 0, args_received;
    memset(buffer, 0, sizeof(buffer));
    
    //receber resposta do server
    while((n = read(fd_tcp, help, 1))!=0){
        if (n == -1) {
            exit(1);
        } else if(n==0){break;}

        buffer[i] = help[0];

        if(buffer[i] == ' '){
            args_received++;
        }

        if(args_received == 4){break;}

        if(buffer[i] == '\n'){break;}
        i++;
    }

    cout << "after read" << endl;

    char *newlinePos = strchr(buffer, '\n');
    if (newlinePos != nullptr) {
        *newlinePos = '\0'; 
    }

    vector<string> words = splitWords(buffer);
    cout << "words:" << endl;
    for(int i =0; i<words.size();i++){
        cout << words[i] << endl;
    }
    if(args_received == 4){
        receive_file(words[2],words[3]);
    }

    close_socket_tcp();
}

//----------------------------------------------------------------------------aux_tcp-------------------------------------------------------------------------------


//----------------------------------------------------------------------------CTRL+C-------------------------------------------------------------------------------

void signalHandler(int signum) {
    std::cout << "Interrupt signal received (" << signum << "). Exiting...\n";

    logout();

    // Terminate the program
    std::exit(signum);
}

//----------------------------------------------------------------------------CTRL+C-------------------------------------------------------------------------------