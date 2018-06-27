// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18

#include "proxy.h"
#include <google/protobuf/text_format.h>

bool syncWrite(tcp::socket &socket, const Buffer &buffer) {
  std::vector<asio::const_buffer> buffers;
  uint32_t len = buffer.size();
  buffers.emplace_back(asio::buffer(&len, sizeof(len)));
  buffers.emplace_back(asio::buffer(buffer.data(), len));
  uint32_t total = len+sizeof(len);
  auto ret = asio::write(socket, buffers);
  assert(ret == total);
  return true;
}

Buffer syncReadBuffer(asio::ip::tcp::socket &socket)
{
  uint32_t len;
  std::error_code ec;
  Buffer buffer;
  auto length = asio::read(socket, asio::buffer(&len, sizeof(uint32_t)), ec);
  if(ec) {
    std::cerr << "syncRead error " << ec.value() << " read " << length << std::endl;
    return buffer;
  }
  assert(length == sizeof(uint32_t));
  buffer.resize(len);
  std::cerr << "syncRead len " << length << " val " << len << std::endl; 
  length = asio::read(socket, asio::buffer(buffer.data(), len), ec);
  if(ec) {
    std::cerr << "syncRead2 error " << ec << " read " << length << std::endl;
  }
  return buffer;
}

void Proxy::read()
{
  std::cerr << "Proxy::read\n";
  asyncReadBuffer(_socket, 5, [this](std::error_code ec, std::shared_ptr<Buffer> buffer) {
    if(ec) {
      std::cerr << "Proxy::read failure " << ec.value() << std::endl;
    } else {
      std::cerr << "Proxy::read " << std::endl;
      try {
        auto msg = deserialize<fetch::oef::pb::Server_AgentMessage>(*buffer);
        std::string uuid = msg.uuid();
        if(uuid == "") { // coming from OEF
          std::unique_lock<std::mutex> mlock(_mutex);
          _inMsgBox[""].push(std::make_unique<fetch::oef::pb::Server_AgentMessage>(msg));
        } else {           
          bool newUuid = false;
          {
            std::unique_lock<std::mutex> mlock(_mutex);
            newUuid = _inMsgBox.find(uuid) == _inMsgBox.end();
            _inMsgBox[uuid].push(std::make_unique<fetch::oef::pb::Server_AgentMessage>(msg));
          }
          if(newUuid) {
            std::cerr << "New Uuid " << uuid << std::endl;
            assert(msg.has_content());
            std::string dest = msg.content().origin();
            _onNew(std::make_unique<Conversation>(uuid, dest, getQueue(uuid), *this));
          }
        }
      } catch(std::exception &e) {
        std::cerr << "Wrong message sent\n";
      }
      read();
    }
  });
}

bool Proxy::secretHandshake()
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_id(_id);
  std::cerr << "Proxy::secretHandshake\n";
  syncWrite(_socket, *serialize(id));
  std::cerr << "Proxy::secretHandshake id sent\n";

  auto buffer = syncReadBuffer(_socket);
  std::cerr << "Proxy::secretHandshake answer received\n";

  // should be a oneof message
  // try { // connection error ?
  //   std::cerr << "Proxy::secretHandshake check1\n";
    
  //   auto c = deserialize<fetch::oef::pb::Server_Connected>(buffer);
  //   std::cerr << "Proxy::secretHandshake check2\n";
  //   if(c.status()) {
  //     std::cerr << "??? should not happen !!!\n";
  //   }
  //   std::cerr << "Received error:" << _id << std::endl;
  //   return false;
  // } catch(std::exception &e) {/* Not an error !!*/ }
  std::cerr << "Proxy::secretHandshake checking answer size " << buffer.size();
 
  auto p = deserialize<fetch::oef::pb::Server_Phrase>(buffer);
  std::string answer_s = p.phrase();
  std::cerr << "Proxy received phrase: [" << answer_s << "]\n";
  // normally should encrypt with private key
  std::reverse(std::begin(answer_s), std::end(answer_s));
  std::cerr << "Proxy sending back phrase: [" << answer_s << "]\n";
  fetch::oef::pb::Agent_Server_Answer answer;
  answer.set_answer(answer_s);
  syncWrite(_socket, *serialize(answer));
  auto connection = syncReadBuffer(_socket);
  auto c = deserialize<fetch::oef::pb::Server_Connected>(connection);
  return c.status();
}


bool Conversation::send(const std::string &msg)
{
  fetch::oef::pb::Envelope env;
  auto *message = env.mutable_message();
  message->set_cid(_uuid.to_string());
  message->set_destination(_dest);
  message->set_content(msg);
  std::string output;
  assert(google::protobuf::TextFormat::PrintToString(env, &output));
  std::cerr << "Conversation::send from " << _uuid.to_string() << " to " << _dest << " msg: " << msg << " [" << output << "]\n";
  _proxy.push(serialize(env));
  // wait for delivered
  auto delivered = _proxy.pop("");
  std::cerr << "Received delivered\n";
  try {
    assert(delivered->has_status());
    return delivered->status();
  } catch(std::exception &e) {
    std::cerr << "Agent::delivered from " << _uuid.to_string() << " not delivered\n";
  }
  return false;
}

