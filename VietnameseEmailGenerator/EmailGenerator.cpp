//
//  EmailGenerator.cpp
//  VietnameseEmailGenerator
//
//  Created by Stefan Dao on 3/19/15.
//  Copyright (c) 2015 Dao Productions. All rights reserved.
//

#include "EmailGenerator.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <map>
#include <vector>
#include <sys/time.h>
#include <stdlib.h>

std::vector<const char *> lastNames;
struct timeval begin;

int createClientToServerConnection()
{
    struct addrinfo hints, *results;
    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int s = getaddrinfo("www.illinois.edu", "80", &hints, &results);
    if( s != 0)
    {
        std::cout << "getaddrinfo failed!\n";
        exit(1);
    }
    if(connect(sockfd, results->ai_addr, results->ai_addrlen) != 0)
    {
        std::cout << "Connection not made!\n";
        exit(1);
    }
    else
        std::cout << "Connection made!\n";
    return sockfd;
}

bool writeToFile(const std::string str, std::map<std::string, std::string> netIDs)
{
    std::fstream file;
    std::string filename = str + ".txt";
    file.open(filename.c_str(), std::ios::out | std::ios::trunc);
    for(auto iter = netIDs.begin(); iter != netIDs.end(); iter++)
    {
        file << iter->second << "," << std::endl;
    }
    return true;
}
bool writeToEmailFile()
{
    std::fstream file;
    std::map<std::string, std::string> netIDs;
    for(int i = 0; i < lastNames.size(); i++)
    {
        std::fstream lastNameFile;
        std::string textfile = std::string(lastNames[i]) + ".txt";
        lastNameFile.open(textfile.c_str(), std::ios::in);
        for(std::string line; getline( lastNameFile, line);)
        {
            netIDs[line] = line;
        }
    }
    
    file.open("email.txt", std::ios::out | std::ios::trunc);
    for(auto iter = netIDs.begin(); iter != netIDs.end(); iter++)
    {
        std::cout << iter->second << std::endl;
        file << iter->second << std::endl;
    }
    return true;
}

//Recursive call for finding netids for last name

int findNetIDsForString(std::string choice, std::string searchString, std::map<std::string, std::string> & mapForLastName)
{
    int count = 0;
    const char * prefix = "GET http://illinois.edu/ds/search?skinId=0&sub=&search=";
    const char * suffix = "&search_type=student HTTP/1.1\r\nHost: illinois.edu\r\nConnection: keep-alive\r\n\r\n";
    
    std::string searchURL = std::string(prefix) + searchString + std::string(suffix);
    int sockfd = createClientToServerConnection(); //illinois.edu automatically closes connection each time
    std::cout << "SENDING: " << searchURL << std::endl;
    int sleepValue = rand() %10 + 60;
    sleep(sleepValue);
    write(sockfd, searchURL.c_str(), strlen(searchURL.c_str()));
    char resp[2000];
    int len;
    while((len = read(sockfd,resp,1999)) > 0)
    {
        resp[len] = '\0';
        char * startID = strstr(resp,"displayIllinois");
        if(startID == NULL && strstr(resp, "Too many results. Please refine your search") != NULL)
        {
            std:: cout << "Too many results found! Expanding search...\n"      << "===\n";
            
            int index = 0, stopIndex = 26;

            for(; index < stopIndex; index++)
            {
                std::string copy = std::string(searchString);
                for(int i = 0; i < copy.length(); i++)
                {
                    
                    if(copy[i] == '*')
                    {
                        char character = 'a' + index;
                        copy.insert(i, &character);
                        count += findNetIDsForString(choice, copy, mapForLastName);
                        break;
                    }
                }
            }
        }
        else if(startID == NULL && strstr(resp, "No results returned") != NULL)
        {
            std::cout << "No results found for " << searchString << "!" << std::endl;
        }
        else if(startID != NULL)
        {
            
            char * endID = NULL;
            while(startID != NULL)
            {
                startID = strstr(startID, "(\"");
                if(startID == NULL)
                    break;
                endID = strstr(startID, "\")");
                if(endID == NULL)
                    break;
                std::string netID;
                for(startID = startID + 2; startID != endID; startID++)
                    netID += *startID;
                if(mapForLastName.find(netID) == mapForLastName.end())
                {
                    std::cout << "Found new NetID: " << netID << std::endl;
                    std::string email = std::string(netID);
                    email += "@illinois.edu";
                    mapForLastName[netID] = email;
                    count++;
                }
                else
                {
                    std::cout << "Found duplicate!" << std::endl;
                }
                startID = endID;
                endID = NULL;
            }
        }
        else if(strstr(resp, "302 Found"))
        {
            std::cout << "We are now blocked!\n";
            struct timeval end;
            gettimeofday(&end, NULL);
            double elapsed_secs = double(end.tv_sec - begin.tv_sec);
            std::cout << "Program blocked after " << elapsed_secs << "!! Please remove block and rerun\n";
            return count;
        }
    }
    std::cout << "===\n";
    return count;
}

int findNetIDs(std::string choice, std::map<std::string, std::string> & netIDs)
{
    int count = 0;
    //Functionality to Search All Last Names at Once
    
    if(choice == "all")
    {
        for(int i = 0; i < (sizeof(lastNames)/sizeof(char *)); i++)
        {
            std::map <std::string, std::string> mapForLastName;
            std::string lastName = std::string("*+") + std::string(lastNames[i]);
            std::cout << "Search String = \n" << lastName << std::endl;
            count += findNetIDsForString(choice, lastName, mapForLastName);
            
            sleep(20);
            writeToFile(std::string(lastNames[i]), mapForLastName);
        }
    }
    else
    {
        std::map <std::string, std::string> mapForLastName;
        std::string lastName = std::string("*+") + choice;
        std::cout << "Search String = \n" << lastName << std::endl;
        count = findNetIDsForString(choice, lastName, mapForLastName);
        writeToFile(std::string(choice), mapForLastName);
    }
    
    return count;
}


int main(int argc, const char * argv[]) {
    // insert code here...
    srand(time(NULL));
    std::cout << "Program started! " << "\n";
    std::string password;
    std::cin >> password;
    if(!(password == "vietnamese" || password == "Vietnamese" || password == "VIETNAMESE"))
    {
        std::cout << "NICE TRY, NSA!\n";
        exit(42);
    }
    int total = 0;

    lastNames.push_back("nguyen");
    lastNames.push_back("tran");
    lastNames.push_back("le");
    lastNames.push_back("pham");
    lastNames.push_back("huynh");
    lastNames.push_back("hoang");
    lastNames.push_back("phan");
    lastNames.push_back("vo");
    lastNames.push_back("dang");
    lastNames.push_back("bui");
    lastNames.push_back("ngo");
    lastNames.push_back("banh");
    lastNames.push_back("duong");
    lastNames.push_back("dam");
    lastNames.push_back("lam");
    lastNames.push_back("quach");
    lastNames.push_back("phung");
    lastNames.push_back("vuong");
    lastNames.push_back("truong");
    lastNames.push_back("dinh");
    lastNames.push_back("trieu");
    lastNames.push_back("all");
    std::cout << "Enter A Last Name for NetIDs (all lowercase letters!)\n";
    
    int i = 0;
    for(i = 0; i < lastNames.size(); i++)
    {
        std::cout << i + 1 << "." << std::string(lastNames[i]) << std::endl;
    }
    
    std::string choice;
    std::cin >> choice;

    std::cout << "Your choice is " << choice << std::endl;
    bool foundLastName = false;
    for(auto iter = lastNames.begin(); iter != lastNames.end(); iter++)
    {
        if(*iter == choice)
        {
            foundLastName = true;
            break;
        }
    }
    if(!foundLastName)
    {
        std::cout << "Did not enter a valid last name!\n";
        exit(1);
    }
    
    std::map<std::string, std::string> netIDs;
    gettimeofday(&begin, NULL);
    std::fstream file;
    
    total = findNetIDs(choice, netIDs);
    if(writeToEmailFile()) std::cout << "Email File Generated!\n";
    
    struct timeval end;
    gettimeofday(&end, NULL);
    double elapsed_secs = double(end.tv_sec - begin.tv_sec);
    std::cout << "Elapsed time in seconds = " << elapsed_secs << std::endl;
    return 0;
}