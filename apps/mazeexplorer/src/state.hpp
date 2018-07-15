#pragma once

#include "multiclient.h"
#include "mapbox/variant.hpp"

namespace var = mapbox::util; // for the variant

using fetch::oef::MultiClient;
using fetch::oef::Conversation;

enum class OEFState {OEF_WAITING_FOR_MAZE = 1,
                     OEF_WAITING_FOR_NOTHING,
                     OEF_WAITING_FOR_REGISTER,
                     OEF_WAITING_FOR_SELLERS};

enum class ExplorerState {OEF_WAITING_FOR_REGISTER = 1,
                          MAZE_WAITING_FOR_REGISTER,
                          OEF_WAITING_FOR_MOVE_DELIVERED,
                          MAZE_WAITING_FOR_MOVE};

enum class SellerState {OEF_WAITING_FOR_PROPOSE = 1,
                        WAITING_FOR_AGREEMENT,
                        OEF_WAITING_FOR_TRANSACTION,
                        OEF_WAITING_FOR_RESOURCES,
                        DONE};

enum class BuyerState {OEF_WAITING_FOR_CFP = 1,
                       WAITING_FOR_PROPOSE,
                       OEF_WAITING_FOR_ACCEPT,
                       OEF_WAITING_FOR_REFUSE,
                       WAITING_FOR_TRANSACTION,
                       WAITING_FOR_RESOURCES,
                       DONE};
                        
using VariantState = var::variant<OEFState,ExplorerState,SellerState,BuyerState>;

std::string to_string(const VariantState &s);
