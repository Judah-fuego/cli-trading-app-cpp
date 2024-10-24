# cli-trading-app-cpp
 It is a real-time command line tradding application made in C++ using the jsoncpp and curl libraries

###Try it on your own computer (i use macos so some configs might be different)

#Necessary dependices
* Jsoncpp
* Curl

#Compiler Used
*g++ 
You can probably user another compiler but I haven't tried.

##Steps
1. git clone https://github.com/Judah-fuego/cli-trading-app-cpp.git
2. brew install jsoncpp and brew install curl
3. Create free account on Finnhub to get API key and plug into the client contrsuctor in the main function
4. compile the code g++ -o finnhub_client finnhub_client.cpp *-I/opt/homebrew/Cellar/jsoncpp/1.9.6/include/ (path to libary)* *-I/opt/homebrew/Cellar/curl/8.10.1/include/ (path to libary)* -L/opt/homebrew/Cellar/jsoncpp/1.9.6/lib/ -ljsoncpp -L/opt/homebrew/Cellar/curl/8.10.1/lib/ -lcurl -std=c++17
5. Run locally!!
   

