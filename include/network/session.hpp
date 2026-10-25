#include "fs/data_manager.hpp"
#include "network/TCPSocket.hpp"
#include "util/queue.hpp"
#include "util/bytepipe.hpp"
#include <condition_variable>
#include <functional>
#include <string>
#include <sys/types.h>
#include <thread>
#include <variant>

enum class State {
    Idle,
    ReceivedRequest,
    SendingRequest,
    WaitAnswer,
    Connected,
    Denied,
    Error,
    Closed
};

enum class Cmd { Idle, Accept, Deny, Disconnect };

enum class Direction { Incoming, Outgoing };

enum class Step { Initialization, DataTransfering, Finalization, Stopped };

class SmallMsg {
  public:
    SmallMsg(std::string msg) : msg(msg) {}
    std::string msg;
};

class BigMsg {
  public:
    BigMsg(std::function<std::vector<char>(uint64_t)> read_func)
        : read(read_func) {}
    std::function<std::vector<char>(uint64_t)> read;
};

struct HelloMsg{
  std::string uuid;
  std::string name;
  std::string msg;
};

class Session {
  private:
    // storages
    DataManager dm;
    TCPSocket sock;
    SafeQueue<std::variant<SmallMsg, BigMsg>> send_queue;
    BytePipe recv_pipe;
    //locks
    std::mutex mtx;
    std::condition_variable state_and_step_cv;
    std::condition_variable cmd_cv;
    // threads
    std::thread process_cmd_thread;
    std::thread process_msg_thread;
    // states
    State state;
    Cmd command;
    Direction dir;
    Step step = Step::Initialization;
    // messages
    std::string AcceptMessage;
    std::string DenyMessage;
    std::string ConnectRequestMessage;
    // methods
    void send_accept(int timeout);
    void send_deny(int timeout);
    void send_request(int timeout);
    bool is_got_request(int timeout);
    bool get_access_or_deny(int timeout);
    void close_sock();
    void process_cmd();
    void process_msg();
    bool accept_or_deny(std::string s);

  public:
    Session(TCPSocket s, Direction dir);
    void process();
    bool is_stopped();
    bool set_cmd(Cmd cmd);
    bool push_msg(SmallMsg msg);
    bool push_msg(BigMsg msg);
    std::vector<char> recv_bytes(uint64_t n);
};
