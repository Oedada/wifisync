#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

template <typename T> class SafeDict {
  private:
    std::mutex mtx;
    std::map<std::string, T> dict;

  public:
    std::vector<std::string> keys();
    void set(std::string key, T val);
    std::optional<T> get(const std::string &key);
    bool contains(const std::string &key);
    bool erase(const std::string &key);
    std::map<std::string, T> get_all();
};
template <typename T> 
void SafeDict<T>::set(std::string key, T val){
    std::lock_guard<std::mutex> lock(mtx);
    dict[key] = val;
}
template <typename T>
std::vector<std::string> SafeDict<T>::keys(){
    std::vector<std::string> keys;
    std::lock_guard<std::mutex> lock(mtx);
    for(auto [key, val] : dict){
        keys.push_back(key);
    }
    return keys;
}

template<typename T>
std::optional<T> SafeDict<T>::get(const std::string& key){
    std::lock_guard<std::mutex> lock(mtx);
    if(dict.find(key) == dict.end()){
        return std::nullopt;
    } else {
        return dict[key];
    }
}

template<typename T>
bool SafeDict<T>::contains(const std::string &key){
    std::lock_guard<std::mutex> lock(mtx);
    return dict.contains(key);
}


template<typename T>
bool SafeDict<T>::erase(const std::string &key){
    std::lock_guard<std::mutex> lock(mtx);
    return dict.erase(key) > 0;
}

template<typename T>
std::map<std::string, T> SafeDict<T>::get_all(){
    std::lock_guard<std::mutex> lock(mtx);
    return dict;
}
