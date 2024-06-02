#include "session/code.h"
#include "session/ftp_request.h"
#include "session/ftp_response.h"
#include <algorithm>
#include <any>
#include <argparse/argparse.hpp>
#include <asio/append.hpp>
#include <asio/buffer.hpp>
#include <asio/compose.hpp>
#include <asio/connect.hpp>
#include <asio/deferred.hpp>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/experimental/cancellation_condition.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read_until.hpp>
#include <asio/static_thread_pool.hpp>
#include <asio/steady_timer.hpp>
#include <asio/streambuf.hpp>
#include <asio/use_future.hpp>
#include <asio/write.hpp>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fmt/format.h>
#include <functional>
#include <future>
#include <indicators/progress_bar.hpp>
#include <indicators/setting.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace indicators;
using namespace argparse;
using namespace asio::ip;
using namespace orange;

ArgumentParser args("simple_client");

void init_param(int argc, char** argv) {
  args.add_argument("host")
    .help("remote ftp server addr");
  args.add_argument("port")
    .help("remote ftp server port");
  args.add_argument("file")
    .help("target file");
  args.add_argument("--dummy")
    .help("dummy output")
    .flag();
  args.add_argument("--client_num")
    .help("client num")
    .default_value(1)
    .nargs(1)
    .scan<'i', int>();
  args.add_argument("--thread_num")
    .help("thread num")
    .default_value(1)
    .nargs(1)
    .scan<'i', int>();
  args.parse_args(argc, argv);
}

ProgressBar bar{
  option::BarWidth{120},
  option::Start{"["},
  option::Fill{"="},
  option::Lead{">"},
  option::Remainder{" "},
  option::End{"]"},
  option::ShowPercentage{true},
  option::PostfixText{"Receiving File"},
  option::ForegroundColor{Color::cyan},
  option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
};

std::vector<std::string> split_string(const std::string& str, char dim = ' ') {
  std::stringstream is(str);
  std::vector<std::string> ret;
  std::string line;
  while (std::getline(is, line, dim)) {
    ret.push_back(std::move(line));
  }
  return ret;
}

template <typename It>
std::string join_string(It beg, It end, char dim) {
  std::stringstream is;
  for (It iter = beg; iter != end; iter++) {
    const std::string& str = *iter;
    is << str;
    if (iter != (end - 1)) {
      is << dim;
    }
  }
  return is.str();
}

class RecvFileImpl {
 public:
  RecvFileImpl(const int* pipe, int target_fd, asio::ip::tcp::socket* socket, int identity) :
    pipe_fd_(pipe),
    fd_(target_fd),
    socket_(socket),
    offset_(0),
    identity_(identity) {
  }

  template<typename CompletionToken>
  auto do_recv_file(CompletionToken&& token) {
    status_ = Status::waiting_for_readable;
    return asio::async_compose<CompletionToken, void(bool)>(
      *this, token, *socket_
    );
  }

  template<typename Self>
  void operator()(Self& self, std::error_code ec = std::error_code()) {
    if (ec) {
      spdlog::error("Error occur: {}, received: {} bytes, identity: {}", ec.message(), offset_, identity_);
      self.complete(false);
      return;
    }
    while (true) {
      if (status_ == Status::waiting_for_readable) {
        status_ = Status::do_read;
        socket_->async_wait(asio::ip::tcp::socket::wait_read, std::move(self));
        return;
      } else if (status_ == Status::do_read) {
        status_ = deal_event();
        continue;
      } else if (status_ == Status::finished) {
        self.complete(true);
        return;
      } else if (status_ == Status::unexpected_terminate) {
        self.complete(false);
        return;
      }
    }
  }

  void set_update_hook(std::function<void(off64_t)> fcn) {
    update_hook_ = fcn;
  }

 private:
  enum class Status {
    waiting_for_readable,
    do_read,
    finished,
    unexpected_terminate,
  };

  Status deal_event() {
    if (!socket_->native_non_blocking()) {
      spdlog::warn("Data socket is not non-blocking, setting..");
      socket_->native_non_blocking(true);
    }
    std::error_code ignored;
    size_t available = socket_->available(ignored);
    spdlog::debug("Available reading bytes: {}, identity: {}", available, identity_);
    if (available == 0 || ignored) {
      spdlog::info("Remote peer closed, {}, ideneity: {}", ignored.message(), identity_);
      return RecvFileImpl::Status::finished;
    }
    const size_t len = 65536;
    while(auto splice_ret = ::splice(socket_->native_handle(), nullptr, pipe_fd_[1], nullptr, len, SPLICE_F_NONBLOCK | SPLICE_F_MORE | SPLICE_F_MOVE)) {
      if (splice_ret < 0) {
        std::error_code ec = asio::error_code(errno, asio::error::get_system_category());
        if (ec == asio::error::interrupted) {
          continue;
        } else if (ec == asio::error::would_block || ec == asio::error::try_again) {
          spdlog::debug("No more data can be read, identity: {}", identity_);
          break;
        } else {
          spdlog::error("Error when calling splice: {}, identity: {}", ec.message(), identity_);
          return Status::unexpected_terminate;
        }
      }
      if (splice_ret == 0) {
        break;
      }
      auto bytes_in_pipe = splice_ret;
      spdlog::debug("Read from socket: {} bytes, identity: {}", splice_ret, identity_);
      while (bytes_in_pipe > 0) {
        auto pipe_ret = ::splice(pipe_fd_[0], nullptr, fd_, nullptr, bytes_in_pipe, SPLICE_F_NONBLOCK | SPLICE_F_MORE | SPLICE_F_MOVE);
        offset_ += pipe_ret;
        spdlog::debug("Read from pipe: {} bytes, total: {} bytes, identity: {}", pipe_ret, offset_, identity_);
        bytes_in_pipe -= pipe_ret;
        if (update_hook_) {
          update_hook_(offset_);
        }
      }
    }
    return Status::waiting_for_readable;
  }

  const int* pipe_fd_;
  int fd_;
  asio::ip::tcp::socket* socket_;
  std::function<void(off64_t)> update_hook_;
  off64_t offset_;
  int identity_;
  Status status_;
};

template<typename ContextType>
class Client {
 public:
  Client(ContextType& io_context) :
    control_socket_(io_context),
    timer_(io_context),
    io_context_(io_context),
    resolver_(io_context),
    target_fd_(0),
    pipe_fd_{0} {
  }

  ~Client() {
    if (control_socket_.is_open()) {
      control_socket_.close();
    }
    if (data_socket_ && data_socket_->is_open()) {
      data_socket_->close();
    }
    if (target_fd_ > 0) {
      ::close(target_fd_);
    }
    if (pipe_fd_[0] > 0) {
      ::close(pipe_fd_[0]);
      ::close(pipe_fd_[1]);
    }
  }

  auto get_read_with_timeout_op(int seconds = 10) {
    timer_.expires_after(std::chrono::seconds(seconds));
    auto deferred = asio::experimental::make_parallel_group(
      asio::async_read_until(control_socket_, buf_, "\r\n", asio::deferred),
      timer_.async_wait(asio::deferred)
    ).async_wait(asio::experimental::wait_for_one(), asio::deferred) |
      asio::deferred([this](
      std::array<std::size_t, 2> completion_order,
      std::error_code ec1, std::size_t n1,
      std::error_code ec2) {
        std::optional<ftpResponse> resp_wrap;
        if (completion_order[0] != 0) {
          // timeout
          spdlog::error("Timeout while reading from remote, port: {}", control_socket_.local_endpoint().port());
          return asio::deferred.values(resp_wrap);
        }
        if (ec1) {
          spdlog::error("Error happen while reading from remote: {}, port: {}", ec1.message(), control_socket_.local_endpoint().port());
          return asio::deferred.values(resp_wrap);
        }
        auto msg = std::string(
          asio::buffers_begin(buf_.data()),
          asio::buffers_begin(buf_.data()) + n1
        );
        buf_.consume(n1);
        spdlog::debug("Recieve data: {}", msg);

        ftpResponse resp;
        if (!build_resp(msg, &resp)) {
          return asio::deferred.values(resp_wrap);
        }
        resp_wrap.emplace(resp);
        return asio::deferred.values(resp_wrap);
      });
    return deferred;
  }

  auto async_init(const std::string& addr, const std::string& port) {
    auto forward_op = resolver_.async_resolve(addr, port, asio::deferred) |
      asio::deferred([this](std::error_code ec, const tcp::resolver::results_type& endpoints){
        if (ec) {
          spdlog::error("Resolve remote addr failed, {}", ec.message());
        }
        return asio::deferred
          .when(!ec)
          .then(asio::async_connect(control_socket_, endpoints, asio::deferred))
          .otherwise(asio::deferred.values(ec, tcp::endpoint()));
      }) |
      asio::deferred([this](std::error_code ec, tcp::endpoint et){
        if (!ec) {
          spdlog::info("Connetion establish, local port: {}", control_socket_.local_endpoint().port());
        } else {
          spdlog::error("Coonect remote failed, {}", ec.message());
        }
        return get_read_with_timeout_op();
      }) |
      asio::deferred([](std::optional<ftpResponse> resp){
        if (resp.has_value() && resp.value().code == ret_code::welcome) {
          return asio::deferred.values(true);
        }
        spdlog::error("Unexpected code: {}, msg: {}", resp.has_value() ? (int)resp.value().code : 0, resp.has_value() ? resp.value().msg : "");
        return asio::deferred.values(false);
      });
    return forward_op;
  }

  auto async_call(const FtpRequest req) {
    auto forward_op =
      asio::async_write(control_socket_, build_req(req), asio::deferred) |
      asio::deferred([this](std::error_code ec, size_t n){
        return get_read_with_timeout_op();
      });
    return forward_op;
  }

  auto async_establish_data_connection(const std::string& addr, uint16_t port) {
    data_socket_ = std::make_unique<tcp::socket>(io_context_);
    auto forward_op = resolver_.async_resolve(addr, std::to_string(port), asio::deferred) |
      asio::deferred([this](std::error_code ec, const tcp::resolver::results_type& endpoints){
        return asio::deferred
          .when(!ec)
          .then(asio::async_connect(*data_socket_, endpoints, asio::deferred))
          .otherwise(asio::deferred.values(ec, tcp::endpoint()));
      });
    return forward_op;
  }

  auto do_recv_op(const std::string& file_name, std::function<void(off64_t)> fcn = nullptr) {
    bool cond = true;
    target_fd_ = ::open((args["--dummy"] == true) ? "/dev/null" : file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (target_fd_ < 0) {
      spdlog::error("Create file failed: {}, reason: {}", file_name, asio::error_code(errno, asio::error::get_system_category()).message());
      cond = false;
    }
    if (cond && ::pipe(pipe_fd_) < 0) {
      spdlog::error("Create pipe failed: {}", asio::error_code(errno, asio::error::get_system_category()).message());
      cond = false;
    }
    RecvFileImpl recv_impl(pipe_fd_, target_fd_, data_socket_.get(), control_socket_.local_endpoint().port());
    if (fcn) {
      recv_impl.set_update_hook(fcn);
    }
    auto start = std::chrono::system_clock::now();
    return asio::deferred
      .when(cond)
      .then(recv_impl.do_recv_file(asio::append(asio::deferred, start)))
      .otherwise(asio::deferred.values(false, start));
  }

 private:
  SharedConstBuffer build_req(const FtpRequest& req) {
    std::stringstream ss;
    ss << req.command << " " << req.body << "\r\n";
    return SharedConstBuffer(ss.str());
  }

  bool build_resp(const std::string& msg, ftpResponse* resp) {
    auto ret = split_string(msg);
    if (ret.size() < 2) {
      spdlog::error("Unexpected response: {}", fmt::join(ret, " "));
      return false;
    }
    resp->code = static_cast<ret_code>(std::stoi(ret[0]));
    resp->msg = join_string(ret.begin() + 1, ret.end(), ' ');
    return true;
  }

  tcp::socket control_socket_;
  std::unique_ptr<tcp::socket> data_socket_;
  asio::steady_timer timer_;
  asio::streambuf buf_;
  ContextType& io_context_;
  tcp::resolver resolver_;
  int target_fd_;
  int pipe_fd_[2];
};

template<typename ContextType>
std::future<bool> async_session(Client<ContextType>& client) {
  auto file_size = std::make_shared<int64_t>(0);
  auto op = client.async_init(args.get("host"), args.get("port")) |
    asio::deferred([&client](bool init_ret){
      if (!init_ret) {
        spdlog::error("Init client failed");
      }
      std::optional<ftpResponse> resp_wrap;
      FtpRequest request{
        "user",
        "anonymous",
      };
      return asio::deferred
        .when(init_ret)
        .then(client.async_call(request))
        .otherwise(asio::deferred.values(resp_wrap));
    }) |
    asio::deferred([&client](std::optional<ftpResponse> resp){
      bool cond = resp.has_value() && resp.value().code == ret_code::password_required;
      if (!cond) {
        spdlog::error("USER request failed");
      }
      std::optional<ftpResponse> resp_wrap;
      FtpRequest request{
        "pass",
        "",
      };
      return asio::deferred
        .when(cond)
        .then(client.async_call(request))
        .otherwise(asio::deferred.values(resp_wrap));
    }) |
    asio::deferred([&client](std::optional<ftpResponse> resp){
      bool cond = resp.has_value() && resp.value().code == ret_code::logged_on;
      if (!cond) {
        spdlog::error("PASS request failed");
      }
      std::optional<ftpResponse> resp_wrap;
      FtpRequest request{
        "size",
        args.get("file"),
      };
      return asio::deferred
        .when(cond)
        .then(client.async_call(request))
        .otherwise(asio::deferred.values(resp_wrap));
    }) |
    asio::deferred([&client, file_size](std::optional<ftpResponse> resp){
      std::optional<ftpResponse> resp_wrap;
      bool cond = resp.has_value() && resp.value().code == ret_code::size_or_mdtm_ok;
      if (!cond) {
        spdlog::error("SIZE request failed");
      }
      if (cond) {
        *file_size = std::stol(resp.value().msg);
      }
      FtpRequest request{
        "pasv",
        "",
      };
      return asio::deferred
        .when(cond)
        .then(client.async_call(request))
        .otherwise(asio::deferred.values(resp_wrap));
    }) |
    asio::deferred([&client](std::optional<ftpResponse> resp) {
      bool cond = resp.has_value() && resp.value().code == ret_code::pasv_ok;
      if (!cond) {
        spdlog::error("PASV request failed");
      }
      std::smatch match_results;
      if (cond) {
        std::regex r("\\(\\d+,\\d+,\\d+,\\d+,\\d+,\\d+\\)");
        if (!std::regex_search(resp.value().msg, match_results, r) || match_results.size() != 1) {
          spdlog::error("Invalid pasv resp");
          cond = false;
        }
      }
      uint16_t pasv_port = 0;
      std::string pasv_addr;
      if (cond) {
        const std::string matched_str = match_results.str();
        auto split_ret = split_string(matched_str.substr(1, match_results.length() - 2), ',');
        pasv_port = std::stoi(split_ret[split_ret.size() - 2]) * 256 +  std::stoi(split_ret.back());
        pasv_addr = join_string(split_ret.begin(), split_ret.begin() + 4, '.');
        spdlog::info("data connection host: {}, port: {}", pasv_addr, pasv_port);
      }
      return asio::deferred
        .when(cond)
        .then(client.async_establish_data_connection(pasv_addr, pasv_port))
        .otherwise(asio::deferred.values(asio::error::invalid_argument, tcp::endpoint()));
    }) |
    asio::deferred([&client](std::error_code ec, tcp::endpoint et){
      bool cond = !ec && et.port() > 0;
      if (!cond) {
        spdlog::error("connecting remote data socket failed");
      }
      FtpRequest request = FtpRequest{
        "retr",
        args.get("file"),
      };
      return asio::deferred
        .when(cond)
        .then(client.async_call(request))
        .otherwise(asio::deferred.values(std::optional<ftpResponse>()));
    }) |
    asio::deferred([&client, file_size](std::optional<ftpResponse> resp){
      bool cond = resp.has_value() && resp.value().code == ret_code::data_conn;
      if (!cond) {
        spdlog::error("RETR request failed");
      }
      return asio::deferred
        .when(cond)
        .then(client.do_recv_op(args.get("file"), [file_size](off64_t size){
            if (args["--dummy"] == false) {
              bar.set_progress(size * 100 / *file_size);
            }
          })
        )
        .otherwise(asio::deferred.values(false, std::chrono::system_clock::time_point()));
    }) |
    asio::deferred([&client](bool recv_ret, std::chrono::system_clock::time_point start){
      if (!recv_ret) {
        spdlog::error("Recieve file failed");
      } else {
        spdlog::info("Cost {}ms receiving file", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count());
      }
      return asio::deferred
        .when(recv_ret)
        .then(client.get_read_with_timeout_op())
        .otherwise(asio::deferred.values(std::optional<ftpResponse>()));
    }) |
    asio::deferred([](std::optional<ftpResponse> resp){
      bool cond = resp.has_value() && resp.value().code == ret_code::transfer_complete;
      if (!cond) {
        spdlog::error("Missing transfer complete");
      }
      return asio::deferred
        .when(cond)
        .then(asio::deferred.values(true))
        .otherwise(asio::deferred.values(false));
    });

  return std::move(op)(asio::use_future);
}

class TimeGuard {
 public:
  TimeGuard() : start_(std::chrono::system_clock::now()) {}
  ~TimeGuard() {
    spdlog::info("Elaps: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_).count());
  }
 private:
  std::chrono::system_clock::time_point start_;
};

int main(int argc, char** argv) {
  init_param(argc, argv);

  if (!(args["--dummy"] == true) && args.get<int>("--client_num") > 1) {
    std::cout << "Invalid param, exit\n";
    std::cout << args.help().str() << "\n";
    return -1;
  }

  spdlog::info("Host: {}, port: {}, file: {}, is_dummy: {}, client_num: {}, thread_num: {}",
    args.get("host"),
    args.get("port"),
    args.get("file"),
    args["--dummy"] == true,
    args.get<int>("--client_num"),
    args.get<int>("--thread_num"));

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::err);
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("benchmark.txt", true);
  file_sink->set_level(spdlog::level::info);
  auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{console_sink, file_sink});
  spdlog::set_default_logger(logger);

  asio::static_thread_pool static_thread_pool(args.get<int>("--thread_num"));

  const int client_num = args.get<int>("--client_num");
  std::unique_ptr<Client<asio::static_thread_pool>> clients[client_num];
  for (auto& c : clients) {
    c = std::make_unique<Client<asio::static_thread_pool>>(static_thread_pool);
  }
  TimeGuard guard;
  std::vector<std::future<bool>> f_vec;
  for (int i = 0; i < client_num; i++) {
    f_vec.emplace_back(async_session(*clients[i]));
  }

  for (size_t i = 0; i < f_vec.size(); i++) {
    auto& f = f_vec[i];
    if (!f.get()) {
      spdlog::error("Session failed");
    } else {
      spdlog::critical("Item: {} finished", i);
      clients[i].reset();
    }
  }

  static_thread_pool.join();

  return 0;
}