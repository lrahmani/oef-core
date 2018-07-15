#pragma once

#include "state.hpp"

void processOEFStatus(Conversation<VariantState> &conversation) {
  conversation.getState()
    .match(
           [&conversation](ExplorerState s) {
             switch(s) {
             case ExplorerState::OEF_WAITING_FOR_REGISTER:
               assert(conversation.msgId() == 0);
               conversation.setState(ExplorerState::MAZE_WAITING_FOR_REGISTER);
               break;
             case ExplorerState::OEF_WAITING_FOR_MOVE_DELIVERED:
               conversation.setState(ExplorerState::MAZE_WAITING_FOR_MOVE);
               break;
             default:
               std::cerr << "Error processOEFStatus ExplorerState " << static_cast<int>(s) << " msgId "
                         << conversation.msgId() << std::endl;
             }
           },
           [&conversation](OEFState s) {
             switch(s) {
             case OEFState::OEF_WAITING_FOR_REGISTER:
               std::cerr << "Seller registered\n";
               conversation.setState(OEFState::OEF_WAITING_FOR_NOTHING);
               break;
             default:
               std::cerr << "Error processOEFStatus OEFState " << conversation.uuid() << " code "
                         << static_cast<int>(s) << " msgId " << conversation.msgId() << std::endl;
             }
           },
           [](SellerState s) {
           },
           [&conversation](BuyerState s) {
             switch(s) {
             case BuyerState::OEF_WAITING_FOR_CFP:
               assert(conversation.msgId() == 0);
               conversation.setState(BuyerState::WAITING_FOR_PROPOSE);
               break;
             case BuyerState::OEF_WAITING_FOR_ACCEPT:
               conversation.setState(BuyerState::WAITING_FOR_TRANSACTION);
               break;
             case BuyerState::OEF_WAITING_FOR_REFUSE:
               conversation.setState(BuyerState::DONE);
               break;
             default:
               std::cerr << "Error processOEFStatus BuyerState " << static_cast<int>(s) << " msgId "
                         << conversation.msgId() << std::endl;
             }
           });
}
