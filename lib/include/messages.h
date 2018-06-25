#pragma once
#include "serialize.h"
#include "uuid.h"
#include <unordered_map>
#include "schema.h"

template <typename T>
std::string toJsonString(const T &v) {
	rapidjson::StringBuffer sb;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer{sb};
	JSONOutputArchive<rapidjson::PrettyWriter<rapidjson::StringBuffer>> ar{writer};
	v.serialize(ar);
	return sb.GetString();
}

template <typename T>
T fromJsonString(const std::string &v) {
  rapidjson::StringStream s(v.c_str());
  JSONInputArchive js{s};
  return T{js};
}

class Id {
 private:
  std::string _id;
 public:
  explicit Id(const std::string &id) : _id{id} {}
	template <typename Archive>
	explicit Id(const Archive &ar) : _id{ar.getString("ID")} {}
  std::string id() const { return _id; }
	template <typename Archive>
	void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("ID", _id);
	}
};

class Phrase {
 private:
  std::string _phrase;
 public:
  explicit Phrase(const std::string &phrase = "RandomlyGeneratedString") : _phrase{phrase} {}
	template <typename Archive>
	explicit Phrase(const Archive &ar) : _phrase{ar.getString("phrase")} {}
	template <typename Archive>
	void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("phrase", _phrase);
	}
  std::string phrase() const { return _phrase; }
};

class Answer {
 private:
  std::string _answer;
 public:
  explicit Answer(const std::string &answer) : _answer{answer} {}
	template <typename Archive>
	explicit Answer(const Archive &ar) : _answer{ar.getString("answer")} {}
	template <typename Archive>
	void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("answer", _answer);
	}
  std::string answer() const { return _answer; }
};

class Connected {
 private:
	bool _status;
 public:
  explicit Connected(bool status) : _status{status} {}
	template <typename Archive>
	explicit Connected(const Archive &ar) : _status{ar.getBool("connected")} {}
	template <typename Archive>
		void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("connected", _status);
	}
  bool status() const { return _status; }
};

class Registered {
 private:
	bool _status;
 public:
  explicit Registered(bool status) : _status{status} {}
	template <typename Archive>
	explicit Registered(const Archive &ar) : _status{ar.getBool("registered")} {}
	template <typename Archive>
		void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("registered", _status);
	}
  bool status() const { return _status; }
};

class MessageBase {
 public:
	virtual ~MessageBase() {}
};

class QueryAnswer {
 private:
  std::vector<std::string> _agents;
 public:
	explicit QueryAnswer(const std::vector<std::string> &agents) : _agents{agents} {}
	template <typename Archive>
	explicit QueryAnswer(const Archive &ar) {
    ar.template getObjects<std::string>("agents", std::back_inserter(_agents));
  }
	template <typename Archive>
		void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
    ar.write("agents", std::begin(_agents), std::end(_agents));
	}
  std::vector<std::string> agents() const { return _agents; }
};

class AgentDescription : public MessageBase {
 private:
  Instance _description;
 public:
  explicit AgentDescription(const Instance &description) : _description{description} {}
  template <typename Archive>
    explicit AgentDescription(const Archive &ar) : _description{ar.getObject("description")} {}
  template <typename Archive>
    void serialize(Archive &ar) const {
    ar.write("description", _description);
  }
  Instance description() const { return _description; }
};

class AgentSearch : public MessageBase {
 private:
  QueryModel _query;
 public:
  explicit AgentSearch(const QueryModel &query) : _query{query} {}
  template <typename Archive>
    explicit AgentSearch(const Archive &ar) : _query{ar.getObject("query")} {}
  template <typename Archive>
    void serialize(Archive &ar) const {
    ar.write("query", _query);
  }
  QueryModel query() const { return _query; }
};

class Message : public MessageBase {
 private:
  Uuid _conversationId;
	std::string _destination;
	std::string _content;
 public:
	explicit Message(const Uuid &conversationId, const std::string &destination, const std::string &content)
		: _conversationId{conversationId}, _destination{destination}, _content{content} {}
	
	template <typename Archive>
    explicit Message(const Archive &ar) : _conversationId{ar}, _destination{ar.getString("destination")},
                                                                                   _content{ar.getString("content")} {}
	template <typename Archive>
		void serialize(Archive &ar) const {
    _conversationId.serialize(ar);
		ar.write("destination", _destination);
		ar.write("content", _content);
	}
  Uuid uuid() const { return _conversationId; }
	std::string destination() const { return _destination; }
	std::string content() const { return _content; }
};

class AgentMessage {
 private:
  Uuid _conversationId;
	std::string _origin;
	std::string _content;
 public:
	explicit AgentMessage(const Uuid &conversationId, const std::string &origin, const std::string &content)
		: _conversationId{conversationId}, _origin{origin}, _content{content} {}
	
	template <typename Archive>
    explicit AgentMessage(const Archive &ar) : _conversationId{ar}, _origin{ar.getString("origin")},
		_content{ar.getString("content")} {}
	template <typename Archive>
		void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
    _conversationId.serialize(ar);
		ar.write("origin", _origin);
		ar.write("content", _content);
	}
	std::string origin() const { return _origin; }
	std::string content() const { return _content; }
};

class Delivered {
 private:
  Uuid _conversationId;
	bool _status;
 public:
  explicit Delivered(const Uuid &conversationId, bool status) : _conversationId{conversationId}, _status{status} {}
	template <typename Archive>
	explicit Delivered(const Archive &ar) : _conversationId{ar},  _status{ar.getBool("delivered")} {}
	template <typename Archive>
		void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
    _conversationId.serialize(ar);
		ar.write("delivered", _status);
	}
  bool status() const { return _status; }
};

class Envelope {
 public:
	enum class MsgTypes {
		Query, Register, Message, Unregister, Description, Search, Error
	};
 private:
	MsgTypes _type;
	std::unique_ptr<MessageBase> _message;
	static std::string msgTypeToString(MsgTypes t) {
		switch(t) {
		case MsgTypes::Query: return "query"; break;
		case MsgTypes::Register: return "register"; break;
		case MsgTypes::Message: return "message"; break;
		case MsgTypes::Unregister: return "unregister"; break;
    case MsgTypes::Description: return "description"; break;
    case MsgTypes::Search: return "search"; break;
		case MsgTypes::Error: return "error"; break;
		}
		return "error";
	}
	static MsgTypes stringToMsgType(const std::string &s) {
		static std::unordered_map<std::string,MsgTypes> types = {
			{ "query", MsgTypes::Query},
			{ "register", MsgTypes::Register},
			{ "message", MsgTypes::Message},
			{ "unregister", MsgTypes::Unregister},
      { "description", MsgTypes::Description},
      { "search", MsgTypes::Search}
		};
		auto iter = types.find(s);
		if(iter == types.end())
			return MsgTypes::Error;
		return iter->second;
	}
 public:
  explicit Envelope(MsgTypes type, std::unique_ptr<MessageBase> msg) : _type{type}, _message{std::move(msg)} {}
  MsgTypes getType() const { return _type; }
	static Envelope makeQuery(const QueryModel &query) {
		return Envelope{MsgTypes::Query, std::unique_ptr<AgentSearch>{new AgentSearch{query}}};
	}
	static Envelope makeRegister(const Instance &description) {
		return Envelope{MsgTypes::Register, std::unique_ptr<AgentDescription>{new AgentDescription{description}}};
	}
	static Envelope makeUnregister(const Instance &description) {
		return Envelope{MsgTypes::Unregister, std::unique_ptr<AgentDescription>{new AgentDescription{description}}};
	}
	static Envelope makeMessage(const Uuid &uuid, const std::string &destination, const std::string &content) {
		return Envelope{MsgTypes::Message, std::unique_ptr<Message>{new Message{uuid, destination, content}}};
	}
	static Envelope makeDescription(const Instance &description) {
		return Envelope{MsgTypes::Description, std::unique_ptr<AgentDescription>{new AgentDescription{description}}};
	}
	static Envelope makeSearch(const QueryModel &query) {
		return Envelope{MsgTypes::Search, std::unique_ptr<AgentSearch>{new AgentSearch{query}}};
	}
  MessageBase *getMessage() const { return _message.get(); }
	template <typename Archive>
	explicit Envelope(const Archive &ar) : _type{stringToMsgType(ar.getString("type"))} {
		switch(_type) {
		case MsgTypes::Query:
			_message = std::unique_ptr<AgentSearch>(new AgentSearch(ar));
			break;
		case MsgTypes::Register:
			_message = std::unique_ptr<AgentDescription>(new AgentDescription(ar));
			break;
		case MsgTypes::Message:
			_message = std::unique_ptr<Message>(new Message(ar));
			break;
		case MsgTypes::Unregister:
			_message = std::unique_ptr<AgentDescription>(new AgentDescription(ar));
			break;
		case MsgTypes::Description:
			_message = std::unique_ptr<AgentDescription>(new AgentDescription(ar));
			break;
		case MsgTypes::Search:
			_message = std::unique_ptr<AgentSearch>(new AgentSearch(ar));
			break;
		case MsgTypes::Error:
			break;
		}
	}
	template <typename Archive>
		void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("type", msgTypeToString(_type));
		switch(_type) {
		case MsgTypes::Query:
			{
				AgentSearch *q = dynamic_cast<AgentSearch*>(_message.get());
				q->serialize(ar);
			}
			break;
		case MsgTypes::Register:
			{
				AgentDescription *q = dynamic_cast<AgentDescription*>(_message.get());
				q->serialize(ar);
			}
			break;
		case MsgTypes::Message:
			{
				Message *q = dynamic_cast<Message*>(_message.get());
				q->serialize(ar);
			}
			break;
		case MsgTypes::Unregister:
			{
				AgentDescription *q = dynamic_cast<AgentDescription*>(_message.get());
				q->serialize(ar);
			}
			break;
		case MsgTypes::Description:
			{
				AgentDescription *q = dynamic_cast<AgentDescription*>(_message.get());
				q->serialize(ar);
			}
			break;
		case MsgTypes::Search:
			{
				AgentSearch *q = dynamic_cast<AgentSearch*>(_message.get());
				q->serialize(ar);
			}
			break;
		case MsgTypes::Error:
			break;
		}
	}
};

class Price {
 private:
  float _price;
 public:
  explicit Price(float price) : _price{price} {}
	template <typename Archive>
	explicit Price(const Archive &ar) : _price{ar.getFloat("price")} {}
	template <typename Archive>
	void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("price", _price);
	}
  float price() const { return _price; }
};

class Data {
 private:
  std::string _name;
  std::string _type;
  std::vector<std::string> _values;
 public:
  explicit Data(const std::string &name, const std::string &type, const std::vector<std::string> &values)
    : _name{name}, _type{type}, _values{values} {}
	template <typename Archive>
  explicit Data(const Archive &ar) : _name{ar.getString("name")}, _type{ar.getString("type")} {
    ar.template getObjects<std::string>("values", std::back_inserter(_values));
  }
	template <typename Archive>
	void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("name", _name);
    ar.write("type", _type);
    ar.write("values", std::begin(_values), std::end(_values));
	}
  std::string name() const { return _name; }
  std::string type() const { return _type; }
  std::vector<std::string> values() const { return _values; }
};

class Accepted {
 private:
	bool _status;
 public:
  explicit Accepted(bool status) : _status{status} {}
	template <typename Archive>
	explicit Accepted(const Archive &ar) : _status{ar.getBool("accepted")} {}
	template <typename Archive>
		void serialize(Archive &ar) const {
		ObjectWrapper<Archive> o{ar};
		ar.write("accepted", _status);
	}
  bool status() const { return _status; }
};

