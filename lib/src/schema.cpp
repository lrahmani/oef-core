//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "schema.hpp"

namespace fetch {
  namespace oef {
    ConstraintType::ConstraintType(const Or &orp) {
      auto *o = constraint_.mutable_or_();
      o->CopyFrom(orp.handle());
    }

    ConstraintType::ConstraintType(const And &andp) {
      auto *a = constraint_.mutable_and_();
      a->CopyFrom(andp.handle());
    }

    bool ConstraintType::check(const fetch::oef::pb::Query_Constraint_ConstraintType &constraint, const VariantType &v) {
      auto constraint_case = constraint.constraint_case();
      switch(constraint_case) {
      case fetch::oef::pb::Query_Constraint_ConstraintType::kOr:
        return Or::check(constraint.or_(), v);
      case fetch::oef::pb::Query_Constraint_ConstraintType::kAnd:
        return And::check(constraint.and_(), v);
      case fetch::oef::pb::Query_Constraint_ConstraintType::kSet:
        return Set::check(constraint.set_(), v);
      case fetch::oef::pb::Query_Constraint_ConstraintType::kRange:
        return Range::check(constraint.range_(), v);
      case fetch::oef::pb::Query_Constraint_ConstraintType::kRelation:
        return Relation::check(constraint.relation(), v);
      case fetch::oef::pb::Query_Constraint_ConstraintType::kDistance:
        return Distance::check(constraint.distance(), v);
      case fetch::oef::pb::Query_Constraint_ConstraintType::CONSTRAINT_NOT_SET:
        // should not reach this line
        return false;
      }
      return false;
    }
  } // namespace oef
} // namespace fetch


