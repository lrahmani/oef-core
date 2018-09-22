#pragma once

#include "agent.pb.h"

class Register {
 private:
  fetch::oef::pb::Envelope _envelope;
 public:
  explicit Register(const Instance &instance) {
    auto *reg = _envelope.mutable_register_();
    auto *inst = reg->mutable_description();
    inst->CopyFrom(instance.handle());
  }
  const fetch::oef::pb::Envelope &handle() const { return _envelope; }
};

class Unregister {
 private:
  fetch::oef::pb::Envelope _envelope;
 public:
  explicit Unregister(const Instance &instance) {
    auto *reg = _envelope.mutable_unregister();
    auto *inst = reg->mutable_description();
    inst->CopyFrom(instance.handle());
  }
  const fetch::oef::pb::Envelope &handle() const { return _envelope; }
};

class Query {
 private:
  fetch::oef::pb::Envelope _envelope;
 public:
  explicit Query(const QueryModel &model) {
    auto *desc = _envelope.mutable_query();
    auto *mod = desc->mutable_query();
    mod->CopyFrom(model.handle());
  }
  const fetch::oef::pb::Envelope &handle() const { return _envelope; }
};

class Message {
 private:
  fetch::oef::pb::Envelope _envelope;
 public:
  explicit Message(const std::string &conversationID, const std::string &dest, const std::string &msg) {
    auto *message = _envelope.mutable_message();
    message->set_cid(conversationID);
    message->set_destination(dest);
    message->set_content(msg);
  }
  const fetch::oef::pb::Envelope &handle() const { return _envelope; }
};

class Search {
 private:
  fetch::oef::pb::Envelope _envelope;
 public:
  explicit Search(const QueryModel &model) {
    auto *desc = _envelope.mutable_search();
    auto *mod = desc->mutable_query();
    mod->CopyFrom(model.handle());
  }
  const fetch::oef::pb::Envelope &handle() const { return _envelope; }
};

class Description {
 private:
  fetch::oef::pb::Envelope _envelope;
 public:
  explicit Description(const Instance &instance) {
    auto *desc = _envelope.mutable_description();
    auto *inst = desc->mutable_description();
    inst->CopyFrom(instance.handle());
  }
  const fetch::oef::pb::Envelope &handle() const { return _envelope; }
};
