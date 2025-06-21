#include "trader.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>
#include <openssl/hmac.h>

Trader::Trader(const std::string& api_key, const std::string& secret_key, const std::string& symbol)
    : api_key(api_key), secret_key(secret_key), symbol(symbol) {}

void Trader::buy(double quantity, double price) {
    std::lock_guard<std::mutex> lock(mtx);
    place_order("BUY", quantity, price);
}

void Trader::sell(double quantity, double price) {
    std::lock_guard<std::mutex> lock(mtx);
    place_order("SELL", quantity, price);
}

std::string Trader::hmac_sha256(const std::string& data) {
    unsigned char* digest;
    digest = HMAC(EVP_sha256(), secret_key.c_str(), secret_key.length(),
                  reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), NULL, NULL);

    std::ostringstream oss;
    for (int i = 0; i < 32; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return oss.str();
}

void Trader::place_order(const std::string& side, double quantity, double price) {
    long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch()).count();

    std::ostringstream params;
    params << std::fixed << std::setprecision(8);
    params << "symbol=" << symbol
           << "&side=" << side
           << "&type=LIMIT"
           << "&timeInForce=GTC"
           << "&quantity=" << quantity
           << "&price=" << price
           << "&timestamp=" << timestamp;

    std::string signature = hmac_sha256(params.str());
    std::string full_url = base_url + "/api/v3/order?" + params.str() + "&signature=" + signature;

    CURL* curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + api_key).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
            std::cout << "Binance Response: " << std::string((char*)contents, size * nmemb) << std::endl;
            return size * nmemb;
        });

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL Error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}
