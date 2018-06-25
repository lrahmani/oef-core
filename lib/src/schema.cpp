#include "schema.h"


std::string t_to_string(int i) { return "int"; }
std::string t_to_string(float f) { return "float"; }
std::string t_to_string(bool b) { return "bool"; }
std::string t_to_string(const std::string &s) { return "string"; }

VariantType string_to_value(fetch::oef::pb::Query_Attribute_Type t, const std::string &s) {
  switch(t) {
  case fetch::oef::pb::Query_Attribute_Type_FLOAT:
    return VariantType{float(std::stod(s))};
  case fetch::oef::pb::Query_Attribute_Type_INT:
    return VariantType{int(std::stol(s))};
  case fetch::oef::pb::Query_Attribute_Type_STRING:
    return VariantType{s};
  case fetch::oef::pb::Query_Attribute_Type_BOOL:
    return VariantType{s == "1" || s == "true"};
  }
  // should not reach this line
  return VariantType{std::string{""}};
}

ConstraintType::ConstraintType(const Or &orp) {
  auto *o = _constraint.mutable_or_();
  o->CopyFrom(orp.handle());
}

ConstraintType::ConstraintType(const And &andp) {
  auto *a = _constraint.mutable_and_();
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
    return Set::check(constraint.set(), v);
  case fetch::oef::pb::Query_Constraint_ConstraintType::kRange:
    return Range::check(constraint.range(), v);
  case fetch::oef::pb::Query_Constraint_ConstraintType::kRel:
    return Relation::check(constraint.rel(), v);
  case fetch::oef::pb::Query_Constraint_ConstraintType::CONSTRAINT_NOT_SET:
  default:
    // should not reach this line
    return false;
  }
}

