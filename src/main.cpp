#include <iostream>
#include "../fastCGI/fcgiHandler.h"
#include <pqxx/connection>
#include <pqxx/transaction>


bool isTableExist(string table_name, pqxx::work *transaction) {
    string request = "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME='" + table_name + "';";
    pqxx::result result = transaction->exec(request);
    result = transaction->exec(request);
    return !result.empty();
}

bool isUserExist(string id, pqxx::work *transaction){
    string request = "SELECT * FROM _user WHERE id = " + id + ";";
    pqxx::result res = transaction->exec(request);
    return res.size() != 0;
}

bool isChatExist(string id, pqxx::work *transaction){
    string request = "SELECT * FROM _chat WHERE id = " + id + ";";
    pqxx::result res = transaction->exec(request);
    return res.size() != 0;
}

bool isUserInChat(string user_id, string chat_id, pqxx::work *transaction){
    string request = "SELECT * FROM _user$_chat WHERE _user = " + user_id + " AND _chat = " + chat_id + ";";
    pqxx::result res = transaction->exec(request);
    return res.size() != 0;
}

string addUser(pqxx::work *transaktion, HTTP *request_handler){
    string username = request_handler->json_post["username"];
    string query = "SELECT * FROM _user WHERE username = '" + username + "';";
    pqxx::result res = transaktion->exec(query);
    if (!res.size()){
        query = "INSERT INTO _user (username) VALUES ('" + username + "');";
        transaktion->exec(query);

        query = "SELECT * FROM _user WHERE username = '" + username + "';";
        pqxx::result res = transaktion->exec(query);

        string id = res.at(0).at("id").c_str();
        return id;
    }else{
        return "";
    }

}

string addChat(pqxx::work *transaktion, HTTP *request_handler){
    string name = request_handler->json_post["name"];
    vector<string> users = request_handler->json_post["users"];

    string query = "SELECT * FROM _chat WHERE name = '" + name + "';";
    pqxx::result res = transaktion->exec(query);
    if (!res.size()){

        query = "INSERT INTO _chat (name) VALUES ('" + name + "')";
        transaktion->exec(query);

        query = "SELECT * FROM _chat WHERE name = '" + name + "';";
        pqxx::result res = transaktion->exec(query);

        string id = res.at(0).at("id").c_str();


        for (int i = 0; i < users.size(); i++){
            if (isUserExist(users[i], transaktion)){
                query = "INSERT INTO _user$_chat (_user, _chat) VALUES (" + users[i] + ", " + id + ");";
                transaktion->exec(query);
            }else{
                return "";
            }

        }

        return id;
    }else{
        return "";
    }
}

string addMesage(pqxx::work *transaktion, HTTP *request_handler){

    string chat_id =    request_handler->json_post["chat"];
    string author_id =  request_handler->json_post["author"];
    string text =       request_handler->json_post["text"];

    if (!isChatExist(chat_id, transaktion)){
        return "";
    }
    if (!isUserExist(author_id, transaktion)){
        return "";
    }
    if (!isUserInChat(author_id, chat_id, transaktion)){
        return "";
    }

    string query;
    query = "INSERT INTO _message (_chat, _author, text) VALUES (" + chat_id + ", " + author_id + ", '" + text + "')";
    transaktion->exec(query);

    query = "SELECT * FROM _message ORDER BY created_at DESC;";
    pqxx::result res = transaktion->exec(query);
    string id = res.at(0).at("id").c_str();

    query = "UPDATE _chat SET updated_at = NOW() WHERE id = " + chat_id + ";";
    res = transaktion->exec(query);


    return id;
}

string getChats(pqxx::work *transaktion, HTTP *request_handler){
    string user_id = request_handler->json_post["user"];

    if (!isUserExist(user_id, transaktion)){
        return "";
    }

    json j;
    string result = "";

    string query = "SELECT * FROM _chat LEFT JOIN _user$_chat ON id = _chat WHERE (_user$_chat)._user = " + user_id + " ORDER BY updated_at;";
    pqxx::result res = transaktion->exec(query);


    for (int i = 0; i < res.size(); i++){
        json tmp_json;
        string id = res.at(i).at("id").c_str();;
        tmp_json["id"] = id;
        tmp_json["name"] = res.at(i).at("name").c_str();
        tmp_json["created_at"] = res.at(i).at("created_at").c_str();
        tmp_json["updated_at"] = res.at(i).at("updated_at").c_str();

        query = "SELECT * FROM _user RIGHT JOIN _user$_chat ON id = (_user$_chat)._user WHERE (_user$_chat)._chat = " + id + ";";
        pqxx::result res2 = transaktion->exec(query);
        json tmp_json2;
        for (int j = 0; j < res2.size(); j++){
            tmp_json2.push_back(res2.at(j).at("id").c_str());
        }
        tmp_json["users"] = tmp_json2;
        j.push_back(tmp_json);
    }
    return j.dump();

}

string getMessages(pqxx::work *transaktion, HTTP *request_handler){
    string chat_id = request_handler->json_post["chat"];

    if (!isChatExist(chat_id, transaktion)){
        return "";
    }

    string query = "SELECT * FROM _message WHERE _chat = " + chat_id + ";";
    pqxx::result res = transaktion->exec(query);

    json j;

    for (int i = 0; i < res.size(); i++){
        json tmp_json;
        tmp_json["chat"] = res.at(i).at("_chat").c_str();
        tmp_json["author"] = res.at(i).at("_author").c_str();
        tmp_json["text"] = res.at(i).at("text").c_str();
        tmp_json["created_at"] = res.at(i).at("created_at").c_str();
        j.push_back(tmp_json);
    }
    return j.dump();
}

int main() {



    FCGIHandler *server_handler = new FCGIHandler(ServerParams("*", "9001", 1));
    server_handler->StartServer();
    HTTP *request_handler = nullptr;

    std::ostringstream conn_string("");
    conn_string << "host=" << "localhost"
                << " user=" << "chat"
                << " password=" << "123"
                << " dbname=" << "chat_db";

    pqxx::connection conn(conn_string.str());

    pqxx::work *transaction = new pqxx::work(conn, "");


    string query = "";
    if (!isTableExist("_user", transaction)){
        query = "CREATE TABLE _user (id SERIAL PRIMARY KEY, username text, created_at TIMESTAMPTZ NOT NULL DEFAULT NOW());";
        transaction->exec(query);
    }
    if (!isTableExist("_chat", transaction)){
        query = "CREATE TABLE _chat (id SERIAL PRIMARY KEY, name text, created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(), updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW());";
        transaction->exec(query);
    }
    if (!isTableExist("_user$_chat", transaction)){
        query = "CREATE TABLE _user$_chat (_user bigint, _chat bigint);";
        transaction->exec(query);
    }
    if (!isTableExist("_message", transaction)){
        query = "CREATE TABLE _message (id SERIAL PRIMARY KEY, _chat bigint, _author bigint, text text, created_at TIMESTAMPTZ NOT NULL DEFAULT NOW());";
        transaction->exec(query);
    }


    transaction->commit();
    delete transaction;

    while (1){

        while(!(request_handler = server_handler->getRequest()));

        string uri = request_handler->getEnv("REQUEST_URI");



        string res;

//        try {
            if (uri == "/users/add") {
                transaction = new pqxx::work(conn, "");
                res = addUser(transaction, request_handler);
                if (res.size()) {
                    transaction->commit();
                    request_handler->setHTTPStatus(200);
                    request_handler->setBody(res.c_str());
                } else {
                    transaction->abort();
                    request_handler->setHTTPStatus(404);
                    request_handler->setBody("404");
                }
                delete transaction;
            } else if (uri == "/chats/add") {
                transaction = new pqxx::work(conn, "");
                res = addChat(transaction, request_handler);
                if (res.size()) {
                    transaction->commit();
                    request_handler->setHTTPStatus(200);
                    request_handler->setBody(res.c_str());
                } else {
                    transaction->abort();
                    request_handler->setHTTPStatus(404);
                    request_handler->setBody("404");
                }
                delete transaction;
            } else if (uri == "/messages/add") {
                transaction = new pqxx::work(conn, "");
                res = addMesage(transaction, request_handler);
                if (res.size()) {
                    transaction->commit();
                    request_handler->setHTTPStatus(200);
                    request_handler->setBody(res.c_str());
                } else {
                    transaction->abort();
                    request_handler->setHTTPStatus(404);
                    request_handler->setBody("404");
                }
                delete transaction;
            } else if (uri == "/chats/get") {
                transaction = new pqxx::work(conn, "");
                res = getChats(transaction, request_handler);
                if (res.size()) {
                    transaction->commit();
                    request_handler->setHTTPStatus(200);
                    request_handler->setBody(res.c_str());
                } else {
                    transaction->abort();
                    request_handler->setHTTPStatus(404);
                    request_handler->setBody("404");
                }
                delete transaction;
            } else if (uri == "/messages/get") {
                transaction = new pqxx::work(conn, "");
                res = getMessages(transaction, request_handler);
                if (res.size()) {
                    transaction->commit();
                    request_handler->setHTTPStatus(200);
                    request_handler->setBody(res.c_str());
                } else {
                    transaction->abort();
                    request_handler->setHTTPStatus(404);
                    request_handler->setBody("404");
                }
                delete transaction;
            } else {
                request_handler->setHTTPStatus(404);
            }
//        }catch (...){
//            request_handler->setHTTPStatus(404);
//            request_handler->setBody("404");
//        }



        request_handler->collectData();
        server_handler->uploadResponse(request_handler, "");

    }



    return 0;
}