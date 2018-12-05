#pragma once

#include "oefcoreproxy.hpp"
#include "uuid.hpp"
#include <unordered_map>
#include <memory>
#include <string>

constexpr size_t hash_combine(size_t lhs, size_t rhs ) {
  lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
  return lhs;
}

struct DialogueKey {
  const std::string destination;
  const uint32_t dialogueId;

  DialogueKey(std::string dest, uint32_t id) : destination{std::move(dest)}, dialogueId{id} {} 
  size_t hash() const {
    return hash_combine(std::hash(destination), dialogueId);
  }
};

namespace std
{  
  template<> struct hash<DialogueKey>  {
    size_t operator()(DialogueKey const& s) const noexcept
    {
      return s.hash();
    }
  };
}

namespace fetch {
  namespace oef {
    
    class DialogueDispatcher;
    // TODO protocol class: msgId ...
    class SingleDialogue :  : public std::enable_shared_from_this<SingleDialogue> {
    private:
      DialogueDispatcher &dispatcher;
      const std::string destination_;
      const uint32_t dialogueId_;
      bool buyer_;
    public:
      SingleDialogue(DialogueDispatcher &dispatcher, std::string destination);
      SingleDialogue(DialogueDispatcher &dispatcher, std::string destination, uint32_t dialogueId);
      virtual ~SingleDialogue();
      std::string destination() const { return destination_; }
      uint32_t id() const { return dialogueId_; } 
      virtual void onMessage(const std::string &content) = 0;
      virtual void onCFP(uint32_t msgId, uint32_t target, const CFPType &constraints) = 0;
      virtual void onPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals) = 0;
      virtual void onAccept(uint32_t msgId, uint32_t target) = 0;
      virtual void onDecline(uint32_t msgId, uint32_t target) = 0;
      void sendMessage(const std::string &msg) {
        oefCore_->sendMessage(dialogueId_, destination_, msg);
      }
      void sendCFP(const CFPType &constraints, uint32_t msgId = 1, uint32_t target = 0) {
        oefCore_->sendCFP(dialogueId_, destination_, constraints, msgId, target);
      }
      void sendPropose(const ProposeType &proposals, uint32_t msgId, uint32_t target) {
        oefCore_->sendPropose(dialogueId_, destination_, proposals, msgId, target);
      }
      void sendAccept(uint32_t msgId, uint32_t target) {
        oefCore_->sendAccept(dialogueId_, destination_, msgId, target);
      }
      void sendDecline(uint32_t msgId, uint32_t target) {
        oefCore_->sendDecline(dialogueId_, destination_, msgId, target);
      }
    };
    
    class DialogueDispatcher : public AgentInterface {
    private:
      std::unordered_map<DialogueKey, std::shared_ptr<SingleDialogue>> dialogues_;
      std::unique_ptr<OEFCoreInterface> oefCore_;

      static fetch::oef::Logger logger;

    public:
      explicit DialogueDispatcher(std::unique_ptr<OEFCoreInterface> oefCore) : oefCore_{std::move(oefCore)} {}
      void start() {
        if(oefCore_->handshake())
          oefCore_->loop(*this);
      }
      void onMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {
        auto &iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onMessage: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->onMessage(content);
        }
      }
      void onCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const CFPType &constraints) override {
        auto &iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onCFP: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->onCFP(msgId, target, constraints);
        }
      }
      void onPropose(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const ProposeType &proposals) override {
        auto &iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onPropose: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->onPropose(msgId, target, constraints);
        }
      }
      void onAccept(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {
        auto &iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onAccept: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->onAccept(msgId, target);
        }
      }
      void onDecline(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {
        auto &iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onDecline: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->onDecline(msgId, target);
        }
      }
      bool registerDialogue(std::shared_ptr<SingleDialogue> dialogue) {
        // should be thread safe
        DialogueKey key{dialogue->destination(), dialogue->id()};
        auto &iter = dialogues_.find(key);
        if(iter == dialogues_.end()) {
          dialogues_.insert({key, dialogue});
          return true;
        }
        return false;
      }
      bool unregisterDialogue(SingleDialogue &dialogue) {
        // should be thread safe
        DialogueKey key{dialogue->destination(), dialogue->id()};
        auto &iter = dialogues_.find(key);
        if(iter == dialogues_.end()) {
          return false;
        }
        dialogues_.erase(iter);
        return true;
      }
      std::string getPublicKey() const { return oefCore_->getPublicKey(); }
      void registerDescription(const Instance &instance) {
        oefCore_->registerDescription(instance);
      }
      void registerService(const Instance &instance) {
        oefCore_->registerService(instance);
      }
      void searchAgents(uint32_t search_id, const QueryModel &model) {
        oefCore_->searchAgents(search_id, model);
      }
      void searchServices(uint32_t search_id, const QueryModel &model) {
        oefCore_->searchServices(search_id, model);
      }
      void unregisterService(const Instance &instance) {
        oefCore_->unregisterService(instance);
      }
      void unregisterDescription() {
        oefCore_->unregisterDescription();
      }
      void sendMessage(uint32_t dialogueId, const std::string &dest, const std::string &msg) {
        oefCore_->sendMessage(dialogueId, dest, msg);
      }
      void sendCFP(uint32_t dialogueId, const std::string &dest, const CFPType &constraints, uint32_t msgId = 1, uint32_t target = 0) {
        oefCore_->sendCFP(dialogueId, dest, constraints, msgId, target);
      }
      void sendPropose(uint32_t dialogueId, const std::string &dest, const ProposeType &proposals, uint32_t msgId, uint32_t target) {
        oefCore_->sendPropose(dialogueId, dest, proposals, msgId, target);
      }
      void sendAccept(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) {
        oefCore_->sendAccept(dialogueId, dest, msgId, target);
      }
      void sendDecline(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) {
        oefCore_->sendDecline(dialogueId, dest, msgId, target);
      }
      void stop() {
        oefCore_->stop();
      }
    };
    SingleDialogue::SingleDialogue(DialogueDispatcher &dispatcher, std::string destination)
      : dispatcher_{dispatcher}, destination_{std::move(destination)}, dialogueId_{Uuid32.uuid().val()},
        buyer_{true}
    {
      dispatcher_.registerDialogue(shared_from_this());
    }

    SingleDialogue::SingleDialogue(DialogueDispatcher &dispatcher, std::string destination, uint32_t dialogueId)
      : dispatcher_{dispatcher}, destination_{std::move(destination)}, dialogueId_{dialogueId},
        buyer_{false}
    {
      dispatcher_.registerDialogue(shared_from_this());
    }
    virtual SingleDialogue::~SingleDialogue() {
      dispatcher_.unregisterDialogue(*this);
    }
  }
}
