#include "common.h"

void asyncReadBuffer(asio::ip::tcp::socket &socket, uint32_t timeout, std::function<void(std::error_code,std::shared_ptr<Buffer>)> handler)
{
  auto len = std::make_shared<uint32_t>();
  asio::async_read(socket, asio::buffer(len.get(), sizeof(uint32_t)), [len,handler,&socket](std::error_code ec, std::size_t length) {
      if(ec) {
        handler(ec, std::make_shared<Buffer>());
      } else {
        assert(length == sizeof(uint32_t));
        auto buffer = std::make_shared<Buffer>(*len);
        asio::async_read(socket, asio::buffer(buffer->data(), *len), [buffer,handler](std::error_code ec, std::size_t length) {
            if(ec) {
              std::cerr << "asyncRead2 error " << ec.value() << std::endl;
            }
            handler(ec, buffer);
          });
      }
    });
}

void asyncWriteBuffer(asio::ip::tcp::socket &socket, std::shared_ptr<Buffer> s, uint32_t timeout) {
  std::vector<asio::const_buffer> buffers;
  uint32_t len = uint32_t(s->size());
  buffers.emplace_back(asio::buffer(&len, sizeof(len)));
  buffers.emplace_back(asio::buffer(s->data(), len));
  uint32_t total = len+sizeof(len);
  asio::async_write(socket, buffers,
                    [total,s](std::error_code ec, std::size_t length) {
                      if(ec) {
                        std::cerr << "Grouped Async write error, wrote " << length << " expected " << total << std::endl;
                      }});
}

void asyncWriteBuffer(asio::ip::tcp::socket &socket, std::shared_ptr<Buffer> s, uint32_t timeout, std::function<void(std::error_code, std::size_t length)> handler) {
  std::vector<asio::const_buffer> buffers;
  uint32_t len = uint32_t(s->size());
  buffers.emplace_back(asio::buffer(&len, sizeof(len)));
  buffers.emplace_back(asio::buffer(s->data(), len));
  uint32_t total = len+sizeof(len);
  asio::async_write(socket, buffers,
                    [total,s,handler](std::error_code ec, std::size_t length) {
                      if(ec) {
                        std::cerr << "Grouped Async write error, wrote " << length << " expected " << total << std::endl;
                      } else {
                        handler(ec, length);
                      }
                    });
}
