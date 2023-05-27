#include "tcp_server.hh"

#include "tcp_client_handler.hh"
#include "../common/except.hh"

TcpServerWorker::TcpServerWorker(
    const volatile sig_atomic_t& running, 
    const int ui_port,
    const SyncedPtr<TcpClientSocketSet>& client_sockets,
    const std::shared_ptr<std::vector<pollfd>>& poll_fds,
    const std::shared_ptr<TcpClientHandlerWorker>& client_handler
)   : Worker(running)
    , _socket(ui_port)
    , _client_sockets(client_sockets) 
    , _poll_fds(poll_fds)
    , _client_handler(client_handler)
    {}

void TcpServerWorker::run() {
    _socket.listen();
    while (running) {
        try {
            int client_fd = _socket.accept();
            try_register_client(client_fd);
        } catch (const std::exception& e) {
            logerr(e.what());
        }
    }
}

void TcpServerWorker::try_register_client(const int client_fd) {
    auto lock = _client_sockets.lock();
    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        if (_client_sockets->at(i).get() != nullptr) {
            assert(_poll_fds->at(i).fd != -1);
            _poll_fds->at(i).fd    = client_fd;
            _client_sockets->at(i) = std::make_unique<TcpClientSocket>(client_fd);
            TcpClientHandlerWorker::config_telnet_client(*_client_sockets->at(i));
            _client_handler->greet_telnet_client(*_client_sockets->at(i));
            return;
        }
    }
    throw RadioException("Too many clients");
}
