#include <curl/curl.h>
#include <json/json.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <iomanip>


class Stock;
class User;
class Database;
class FinnhubClient;

// callback function for the api
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

class Stock{
    private:
    std::string symbol;
    double averagePrice;
    int quantity;


    public:
    Stock() : symbol(""), averagePrice(0.0), quantity(0) {}
    Stock(const std::string& symbol, double averagePrice, int quantity)
    : symbol(symbol), averagePrice(averagePrice), quantity(quantity) {}

    std::string getSymbol() const {return symbol; }
    double getAveragePrice() const { return averagePrice; }
    int getQuantity() const {return quantity; }

    void addShares(int shares, double price) {
        if (shares <= 0) return;
        
        double totalCost = shares * price;
        double currentValue = quantity * averagePrice;
        quantity += shares;
        averagePrice = (currentValue + totalCost) / quantity;
    }

     bool sellShares(int shares, double price, double& proceeds) {
        if (shares <= 0 || shares > quantity) return false;
        
        proceeds = shares * price;
        quantity -= shares;
        
        // If all shares are sold, reset average price
        if (quantity == 0) {
            averagePrice = 0.0;
        }
        
        return true;
    }

     void display() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Symbol: " << symbol 
                  << ", Quantity: " << quantity 
                  << ", Average Price: $" << averagePrice 
                  << ", Total Value: $" << (quantity * averagePrice) << std::endl;
    }


};

class User {
private:
    std::string username;
    std::string password;
    double balance;
    std::map<std::string, Stock> portfolio;

public:
    User() : username(""), password(""), balance(0.0) {}
    
    User(const std::string& username, const std::string& password, double balance)
        : username(username), password(password), balance(balance) {}

    // Getters
    std::string getUsername() const { return username; }
    std::string getPassword() const { return password; }
    double getBalance() const { return balance; }

    void deposit(double amount) {
        if (amount > 0) {
            balance += amount;
            std::cout << "Deposited $" << amount << ". New balance: $" << balance << std::endl;
        }
    }

    bool buyStock(const std::string& symbol, int quantity, double price) {
        double totalCost = price * quantity;
        
        if (balance >= totalCost) {
            balance -= totalCost;
            
            if (portfolio.find(symbol) != portfolio.end()) {
                portfolio[symbol].addShares(quantity, price);
            } else {
                portfolio[symbol] = Stock(symbol, price, quantity); // Fixed constructor call
            }
            
            std::cout << "Purchased " << quantity << " shares of " << symbol 
                      << " at $" << price << " per share." << std::endl;
            return true;
        }
        
        std::cout << "Insufficient funds! Required: $" << totalCost 
                  << ", Available: $" << balance << std::endl;
        return false;
    }

    bool sellStock(const std::string& symbol, int quantity, double price) {
        auto it = portfolio.find(symbol);
        if (it != portfolio.end()) {
            double proceeds;
            if (it->second.sellShares(quantity, price, proceeds)) {
                balance += proceeds;
                
                // Remove stock from portfolio if quantity is 0
                if (it->second.getQuantity() == 0) {
                    portfolio.erase(it);
                }
                
                std::cout << "Sold " << quantity << " shares of " << symbol 
                          << " at $" << price << " per share." << std::endl;
                std::cout << "Proceeds: $" << proceeds << std::endl;
                return true;
            }
        }
        
        std::cout << "Invalid sale: Insufficient shares or symbol not found." << std::endl;
        return false;
    }

    void displayPortfolio() const {
        if (portfolio.empty()) {
            std::cout << "Portfolio is empty." << std::endl;
            return;
        }
        
        std::cout << "\nPortfolio for " << username << ":" << std::endl;
        std::cout << "Current Balance: $" << std::fixed << std::setprecision(2) << balance << std::endl;
        std::cout << "\nStocks:" << std::endl;
        
        double totalPortfolioValue = 0.0;
        for (const auto& pair : portfolio) {
            pair.second.display();
            totalPortfolioValue += pair.second.getQuantity() * pair.second.getAveragePrice();
        }
        
        std::cout << "\nTotal Portfolio Value: $" << totalPortfolioValue << std::endl;
        std::cout << "Total Account Value: $" << (totalPortfolioValue + balance) << std::endl;
    }
};

class Database {
private:
    std::map<std::string, User> users; // Map username to User object

public:
    void addUser(const User& user) {
        users[user.getUsername()] = user;
    }
    
    User* getUser(const std::string& username) {
        if (users.find(username) != users.end()) {
            return &users[username];
        }
        return nullptr; // User not found
    }
};

class FinnhubClient {
public: 
    FinnhubClient(const std::string& api_key) : api_key_(api_key) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_ = curl_easy_init();
        
        // Set up common headers
        headers_ = curl_slist_append(nullptr, ("X-Finnhub-Token: " + api_key_).c_str());
        headers_ = curl_slist_append(headers_, "Content-Type: application/json");
    }

    ~FinnhubClient() {
        if (headers_) {
            curl_slist_free_all(headers_);
        }
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        curl_global_cleanup();
    }

    bool getQuote(const std::string& symbol) {
        std::string url = base_url_ + "/quote?symbol=" + symbol;
        std::string response;

        if (!makeRequest(url, response)) return false;

        try {
            Json::Value root;      // Will hold the entire JSON document
            Json::Reader reader;   // Parser object that reads JSON text
            
            if (!reader.parse(response, root)) {
                std::cerr << "Failed to parse response" << std::endl;
                return false;
            }

            std::cout << "Current price: " << root["c"].asDouble() << std::endl;
            std::cout << "High price: " << root["h"].asDouble() << std::endl;
            std::cout << "Low price: " << root["l"].asDouble() << std::endl;
            std::cout << "Previous close: " << root["pc"].asDouble() << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing quote data: " << e.what() << std::endl;
            return false;
        }
    }

    double getPrice(const std::string& symbol){
        std::string url = base_url_ + "/quote?symbol=" + symbol;
        std::string response;

        if (!makeRequest(url, response)) return false;

        try {
            Json::Value root;      // Will hold the entire JSON document
            Json::Reader reader;   // Parser object that reads JSON text
            
            if (!reader.parse(response, root)) {
                std::cerr << "Failed to parse response" << std::endl;
                return false;
            }
            return root["c"].asDouble();
        } catch (const std::exception& e) {
            std::cerr << "Error parsing quote data: " << e.what() << std::endl;
            return false;
        }

    }

private:
    CURL* curl_;
    struct curl_slist* headers_;
    std::string api_key_;
    const std::string base_url_ = "https://finnhub.io/api/v1";

    bool makeRequest(const std::string& url, std::string& response) {
        if (!curl_) return false;

        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl_);
        if (res != CURLE_OK) {
            std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        long http_code = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) {
            std::cerr << "HTTP error: " << http_code << std::endl;
            return false;
        }

        return true;
    }
};

User* signIn(Database& db, const std::string& username, const std::string& password) {
    User* user = db.getUser(username);
    if (user != nullptr && user->getPassword() == password) {
        std::cout << "Sign in successful!" << std::endl;
        return user;
    }
    std::cout << "Invalid credentials!" << std::endl;
    return nullptr;
}

void signOut(User*& user) {
    user = nullptr;
    std::cout << "Signed out successfully!" << std::endl;
}

void purchaseStock(User& user, FinnhubClient& client, const std::string& symbol) {
    if (client.getQuote(symbol)) {
        std::cout << "Enter quantity to purchase: ";
        int quantity;
        std::cin >> quantity;
        
        if (quantity <= 0) {
            std::cout << "Invalid quantity!" << std::endl;
            return;
        }
        
        // In a real application, you would get the actual price from the API
        double currentPrice = client.getPrice(symbol); // This should come from the API
        user.buyStock(symbol, quantity, currentPrice);
    }
}

void sellStock(User& user, FinnhubClient& client, const std::string& symbol) {
    if (client.getQuote(symbol)) {
        std::cout << "Enter quantity to sell: ";
        int quantity;
        std::cin >> quantity;
        
        if (quantity <= 0) {
            std::cout << "Invalid quantity!" << std::endl;
            return;
        }
        
        // In a real application, you would get the actual price from the API
        double currentPrice = client.getPrice(symbol); // This should come from the API
        user.sellStock(symbol, quantity, currentPrice);
    }
}

int main() {
    FinnhubClient finnhub("your-api-key");
    Database db;
    User* currentUser = nullptr;
    
    // Add some sample users
    db.addUser(User("user1", "password1", 500.0));
    db.addUser(User("user2", "password2", 1000.0));
    
    while (true) {
        if (!currentUser) {
            std::cout << "\n1. Sign In\n2. Create Account\n3. Exit\nChoice: ";
            int choice;
            std::cin >> choice;
            
            if (choice == 1) {
                std::string username, password;
                std::cout << "Enter username: "; std::cin >> username;
                std::cout << "Enter password: "; std::cin >> password;
                currentUser = signIn(db, username, password);
            }
            else if (choice == 2) {
                std::string username, password;
                double balance;
                std::cout << "Enter username: "; std::cin >> username;
                std::cout << "Enter password: "; std::cin >> password;
                std::cout << "Enter initial balance: "; std::cin >> balance;
                db.addUser(User(username, password, balance));
                std::cout << "Account created successfully!" << std::endl;
            }
            else if (choice == 3) {
                break;
            }
        }
        else {
            while(currentUser){
            std::cout << "\n1. Search Stock\n2. Buy Stock\n3. Sell Stock"
                      << "\n4. View Portfolio\n5. Deposit Funds\n6. Sign Out"
                      << "\nChoice: ";
            
            int userChoice;
            std::cin >> userChoice;
            
            switch (userChoice) {
                case 1: {
                    std::string symbol;
                    std::cout << "Enter stock symbol: ";
                    std::cin >> symbol;
                    finnhub.getQuote(symbol);
                    break;
                }
                case 2: {
                    std::string symbol;
                    std::cout << "Enter stock symbol: ";
                    std::cin >> symbol;
                    purchaseStock(*currentUser, finnhub, symbol);
                    break;
                }
                case 3: {
                    std::string symbol;
                    std::cout << "Enter stock symbol: ";
                    std::cin >> symbol;
                    sellStock(*currentUser, finnhub, symbol);
                    break;
                }
                case 4:
                    currentUser->displayPortfolio();
                    break;
                case 5: {
                    double amount;
                    std::cout << "Enter amount to deposit: $";
                    std::cin >> amount;
                    currentUser->deposit(amount);
                    break;
                }
                case 6:
                    signOut(currentUser);
                    break;
                default:
                    std::cout << "Invalid choice!" << std::endl;
            }
        }
     }

    }
    return 0;
}

