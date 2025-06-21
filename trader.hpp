#pragma once

#include <string>
#include <mutex>

class Trader {
public:
    Trader(const std::string& api_key, const std::string& secret_key, const std::string& symbol);

    void buy(double quantity, double price);   // Limit Buy
    void sell(double quantity, double price);  // Limit Sell

private:
    std::string api_key;
    std::string secret_key;
    std::string symbol;
    std::string base_url = "https://testnet.binance.vision";

    std::mutex mtx;

    void place_order(const std::string& side, double quantity, double price);
    std::string hmac_sha256(const std::string& data);
};
