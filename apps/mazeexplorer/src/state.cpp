#include "state.hpp"

std::string to_string(const VariantState &s) {
  std::string res;
  s.match(
          [&res](ExplorerState state) {
            res = "ExplorerState " + std::to_string(static_cast<int>(state));
          },
          [&res](OEFState state) {
            res = "OEFState " + std::to_string(static_cast<int>(state));
          },
          [&res](SellerState state) {
            res = "SellerState " + std::to_string(static_cast<int>(state));
          },
          [&res](BuyerState state) {
            res = "BuyerState " + std::to_string(static_cast<int>(state));
          });
  return res;
}
