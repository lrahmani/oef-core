#include "dialogue.hpp"

namespace fetch {
  namespace oef {
    fetch::oef::Logger fetch::oef::DialogueAgent::logger = fetch::oef::Logger("dialogue-agent");
    
    SingleDialogue::SingleDialogue(DialogueAgent &agent, std::string destination)
      : agent_{agent}, destination_{std::move(destination)}, dialogueId_{Uuid32::uuid().val()},
        buyer_{true}
    {
      agent_.registerDialogue(shared_from_this());
    }
    
    SingleDialogue::SingleDialogue(DialogueAgent &agent, std::string destination, uint32_t dialogueId)
      : agent_{agent}, destination_{std::move(destination)}, dialogueId_{dialogueId},
        buyer_{false}
    {
      agent_.registerDialogue(shared_from_this());
    }
    SingleDialogue::~SingleDialogue() {
      agent_.unregisterDialogue(*this);
    }
    void SingleDialogue::sendMessage(const std::string &msg) {
      agent_.sendMessage(dialogueId_, destination_, msg);
    }
    void SingleDialogue::sendCFP(const CFPType &constraints, uint32_t msgId, uint32_t target) {
      agent_.sendCFP(dialogueId_, destination_, constraints, msgId, target);
    }
    void SingleDialogue::sendPropose(const ProposeType &proposals, uint32_t msgId, uint32_t target) {
      agent_.sendPropose(dialogueId_, destination_, proposals, msgId, target);
    }
    void SingleDialogue::sendAccept(uint32_t msgId, uint32_t target) {
      agent_.sendAccept(dialogueId_, destination_, msgId, target);
    }
    void SingleDialogue::sendDecline(uint32_t msgId, uint32_t target) {
      agent_.sendDecline(dialogueId_, destination_, msgId, target);
    }
  }
}
