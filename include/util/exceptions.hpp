#pragma once
#include <stdexcept>

enum class Error {
    ConnectionClosed,
    NetworkError
};
class UntypicalError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
class SocketError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class NetworkError : public SocketError {
    using SocketError::SocketError;
};

class TCPSocketError : public SocketError {
    using SocketError::SocketError;
};

class ConnectionClosedError : public TCPSocketError {
    using TCPSocketError::TCPSocketError;
};

class SystemMethodError : std::runtime_error {
    using std::runtime_error::runtime_error;
};


