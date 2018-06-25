#pragma once

#include <unordered_map>
#include <mutex>
#include <vector>
#include <limits>
#include <unordered_set>
#include <experimental/optional>
#include <iostream>
#include <typeinfo>
#include "mapbox/variant.hpp"
#include "agent.pb.h"

namespace stde = std::experimental;
namespace var = mapbox::util; // for the variant

enum class Type {
  Float = fetch::oef::pb::Query_Attribute_Type_FLOAT,
    Int = fetch::oef::pb::Query_Attribute_Type_INT,
    Bool = fetch::oef::pb::Query_Attribute_Type_BOOL,
    String = fetch::oef::pb::Query_Attribute_Type_STRING };
using VariantType = var::variant<int,float,std::string,bool>;
VariantType string_to_value(fetch::oef::pb::Query_Attribute_Type t, const std::string &s);

class Attribute {
 private:
  fetch::oef::pb::Query_Attribute _attribute;
  static bool validate(const fetch::oef::pb::Query_Attribute &attribute, const std::string &value) {
    try {
      switch(attribute.type()) {
      case fetch::oef::pb::Query_Attribute_Type_FLOAT:
        (void)std::stod(value);
        break;
      case fetch::oef::pb::Query_Attribute_Type_INT:
        (void)std::stol(value);
        break;
      case fetch::oef::pb::Query_Attribute_Type_BOOL:
        return value == "true" || value == "false" || value == "1" || value == "0";
        break;
      case fetch::oef::pb::Query_Attribute_Type_STRING:
        return true;
        break;
      }
    } catch(std::exception &e) {
      return false;
    }
    return true;
  }
 public:
  explicit Attribute(const std::string &name, Type type, bool required) {
    _attribute.set_name(name);
    _attribute.set_type(static_cast<fetch::oef::pb::Query_Attribute_Type>(type));
    _attribute.set_required(required);
  }
  explicit Attribute(const std::string &name, Type type, bool required,
                     const std::string &description)
    : Attribute{name, type, required} {
    _attribute.set_description(description);
  }
  const fetch::oef::pb::Query_Attribute &handle() const { return _attribute; }
  const std::string &name() const { return _attribute.name(); }
  fetch::oef::pb::Query_Attribute_Type type() const { return _attribute.type(); }
  bool required() const { return _attribute.required(); }
  static std::pair<std::string,std::string> instantiate(const std::unordered_map<std::string,std::string> &values,
                                                        const fetch::oef::pb::Query_Attribute &attribute) {
    auto iter = values.find(attribute.name());
    if(iter == values.end()) {
      if(attribute.required()) {
        throw std::invalid_argument(std::string("Missing value: ") + attribute.name());
      }
      return std::make_pair(attribute.name(),"");
    }
    if(validate(attribute, iter->second)) {
      return std::make_pair(attribute.name(), iter->second);
    }
    throw std::invalid_argument(attribute.name() + std::string(" has a wrong type of value ") + iter->second);
  }
  std::pair<std::string,std::string> instantiate(const std::unordered_map<std::string,std::string> &values) const {
    return instantiate(values, _attribute);
  }
};

class Relation {
 public:
  enum class Op {
    Eq = fetch::oef::pb::Query_Relation_Operator_EQ,
      Lt = fetch::oef::pb::Query_Relation_Operator_LT,
      Gt = fetch::oef::pb::Query_Relation_Operator_GT,
      LtEq = fetch::oef::pb::Query_Relation_Operator_LTEQ,
      GtEq = fetch::oef::pb::Query_Relation_Operator_GTEQ,
      NotEq = fetch::oef::pb::Query_Relation_Operator_NOTEQ};
 private:
  fetch::oef::pb::Query_Relation _relation;
  explicit Relation(Op op) {
    _relation.set_op(static_cast<fetch::oef::pb::Query_Relation_Operator>(op));
  }
 public:
  explicit Relation(Op op, const std::string &s) : Relation(op) {
    auto *val = _relation.mutable_val();
    val->set_s(s);
  }
  explicit Relation(Op op, int i) : Relation(op) {
    auto *val = _relation.mutable_val();
    val->set_i(i);
  }
  explicit Relation(Op op, bool b) : Relation(op) {
    auto *val = _relation.mutable_val();
    val->set_b(b);
  }
  explicit Relation(Op op, float f) : Relation(op) {
    auto *val = _relation.mutable_val();
    val->set_f(f);
  }
  const fetch::oef::pb::Query_Relation &handle() const { return _relation; }
  template <typename T>
  static T get(const fetch::oef::pb::Query_Relation &rel) {
    const auto &val = rel.val();
    if(typeid(T) == typeid(int)) { // hashcode ?
      int i = val.i();
      return *reinterpret_cast<T*>(&(i));
    }
    if(typeid(T) == typeid(float)) { // hashcode ?
      float f = val.f();
      return *reinterpret_cast<T*>(&(f));
    }
    if(typeid(T) == typeid(std::string)) { // hashcode ?
      std::string s = val.s();
      return *reinterpret_cast<T*>(&(s));
    }
    if(typeid(T) == typeid(bool)) { // hashcode ?
      bool b = val.b();
      return *reinterpret_cast<T*>(&(b));
    }
    throw std::invalid_argument("Not is not a valid type.");
  }
  template <typename T>
    static bool check_value(const fetch::oef::pb::Query_Relation &rel, const T &v) {
    const auto &s = get<T>(rel);
    switch(rel.op()) {
    case fetch::oef::pb::Query_Relation_Operator_EQ: return s == v;
    case fetch::oef::pb::Query_Relation_Operator_NOTEQ: return s != v;
    case fetch::oef::pb::Query_Relation_Operator_LT: return v < s;
    case fetch::oef::pb::Query_Relation_Operator_LTEQ: return v <= s;
    case fetch::oef::pb::Query_Relation_Operator_GT: return v > s;
    case fetch::oef::pb::Query_Relation_Operator_GTEQ: return v >= s;
    }
    return false;
  }
  static bool check(const fetch::oef::pb::Query_Relation &rel, const VariantType &v) {
    bool res = false;
    v.match([&rel,&res](int i) {
        res = check_value(rel, i);
      },[&rel,&res](float f) {
        res = check_value(rel, f);
      },[&rel,&res](const std::string &s) {
        res = check_value(rel, s);
      },[&rel,&res](bool b) {
        res = check_value(rel, b);
      });        
    return res;
  }
  bool check(const VariantType &v) const {
    return check(_relation, v);
  }
};

class Set {
 public:
  using ValueType = var::variant<std::unordered_set<int>,std::unordered_set<float>,
    std::unordered_set<std::string>,std::unordered_set<bool>>;
  enum class Op {
    In = fetch::oef::pb::Query_Set_Operator_IN,
      NotIn = fetch::oef::pb::Query_Set_Operator_NOTIN};
 private:
  fetch::oef::pb::Query_Set _set;

 public:
  explicit Set(Op op, const ValueType &values) {
    _set.set_op(static_cast<fetch::oef::pb::Query_Set_Operator>(op));
    fetch::oef::pb::Query_Set_Values *vals = _set.mutable_vals();
    values.match([vals,this](const std::unordered_set<int> &s) {
        fetch::oef::pb::Query_Set_Values_Ints *ints = vals->mutable_i();
        for(auto &v : s)
          ints->add_vals(v);
      },[vals,this](const std::unordered_set<float> &s) {
        fetch::oef::pb::Query_Set_Values_Floats *floats = vals->mutable_f();
        for(auto &v : s)
          floats->add_vals(v);
      },[vals,this](const std::unordered_set<std::string> &s) {
        fetch::oef::pb::Query_Set_Values_Strings *strings = vals->mutable_s();
        for(auto &v : s)
          strings->add_vals(v);
      },[vals,this](const std::unordered_set<bool> &s) {
        fetch::oef::pb::Query_Set_Values_Bools *bools = vals->mutable_b();
        for(auto &v : s)
          bools->add_vals(v);
      });
  }
  const fetch::oef::pb::Query_Set &handle() const { return _set; }
  static bool check(const fetch::oef::pb::Query_Set &set, const VariantType &v) {
    const auto &vals = set.vals();
    bool res = false;
    v.match([&vals,&res](int i) {
        for(auto &v : vals.i().vals()) {
          if(v == i) {
            res = true; 
            return;
          }
        }
      },[&vals,&res](float f) {
        for(auto &v : vals.f().vals()) {
          if(v == f) {
            res = true; 
            return;
          }
        }
      },[&vals,&res](const std::string &st) {
        for(auto &v : vals.s().vals()) {
          if(v == st) {
            res = true; 
            return;
          }
        }
      },[&vals,&res](bool b) {
        for(auto &v : vals.b().vals()) {
          if(v == b) {
            res = true; 
            return;
          }
        }
      });
    return res;
  }  
  bool check(const VariantType &v) const {
    return check(_set, v);
  }
};
class Range {
 public:
  using ValueType = var::variant<std::pair<int,int>,std::pair<float,float>,std::pair<std::string,std::string>>;
 private:
  fetch::oef::pb::Query_Range _range;
 public:
  explicit Range(const std::pair<int,int> &r) {
    fetch::oef::pb::Query_IntPair *p = _range.mutable_i();
    p->set_first(r.first);
    p->set_second(r.second);
  }
  explicit Range(const std::pair<float,float> &r) {
    fetch::oef::pb::Query_FloatPair *p = _range.mutable_f();
    p->set_first(r.first);
    p->set_second(r.second);
  }
  explicit Range(const std::pair<std::string,std::string> &r) {
    fetch::oef::pb::Query_StringPair *p = _range.mutable_s();
    p->set_first(r.first);
    p->set_second(r.second);
  }
  const fetch::oef::pb::Query_Range &handle() const { return _range; }
  static bool check(const fetch::oef::pb::Query_Range &range, const VariantType &v) {
    bool res = false;
    v.match([&res,&range](int i) {
        auto &p = range.i();
        res = i >= p.first() && i <= p.second();
      },[&res,&range](float f) {
        auto &p = range.f();
          res = f >= p.first() && f <= p.second();
      },[&res,&range](const std::string &s) {
        auto &p = range.s();
          res = s >= p.first() && s <= p.second();
      },[&res](bool b) {
        res = false; // doesn't make sense
      });        
    return res;
  }
  bool check(const VariantType &v) const {
    return check(_range, v);
  }
};

class DataModel {
 private:
  fetch::oef::pb::Query_DataModel _model;
 public:
  explicit DataModel(const std::string &name, const std::vector<Attribute> &attributes) {
    _model.set_name(name);
    auto *atts = _model.mutable_attributes();
    for(auto &a : attributes) {
      auto *att = atts->Add();
      att->CopyFrom(a.handle());
    }
  }
  explicit DataModel(const std::string &name, const std::vector<Attribute> &attributes, const std::string &description)
    : DataModel{name, attributes} {
    _model.set_description(description);
  }
  const fetch::oef::pb::Query_DataModel &handle() const { return _model; }
  bool operator==(const DataModel &other) const
  {
    if(_model.name() != other._model.name())
      return false;
    // TODO: should check more.
    return true;
  }
  static stde::optional<fetch::oef::pb::Query_Attribute> attribute(const fetch::oef::pb::Query_DataModel &model,
                                                                   const std::string &name) {
    for(auto &a : model.attributes()) {
      if(a.name() == name)
        return stde::optional<fetch::oef::pb::Query_Attribute>{a};
    }
    return stde::nullopt;
  }
  std::string name() const { return _model.name(); }
  static std::vector<std::pair<std::string,std::string>>
    instantiate(const fetch::oef::pb::Query_DataModel &model, const std::unordered_map<std::string,std::string> &values) {
    std::vector<std::pair<std::string,std::string>> res;
    for(auto &a : model.attributes()) {
      res.emplace_back(Attribute::instantiate(values, a));
    }
    return res;
  }
};

class Instance {
 private:
  fetch::oef::pb::Query_Instance _instance;
  std::unordered_map<std::string,std::string> _values;
 public:
 explicit Instance(const DataModel &model, const std::unordered_map<std::string,std::string> &values) : _values{values} {
    auto *mod = _instance.mutable_model();
    mod->CopyFrom(model.handle());
    auto *vals = _instance.mutable_values();
    for(auto &v : values) {
      auto *val = vals->Add();
      val->set_first(v.first);
      val->set_second(v.second);
    }
  }
  explicit Instance(const fetch::oef::pb::Query_Instance &instance) : _instance{instance}
  {
    const auto &values = _instance.values();
    for(auto &v : values) {
      _values[v.first()] = v.second();
    }
  }
  const fetch::oef::pb::Query_Instance &handle() const { return _instance; }
  bool operator==(const Instance &other) const
  {
    if(!(_instance.model().name() == other._instance.model().name()))
      return false;
    for(const auto &p : _values) {
      const auto &iter = other._values.find(p.first);
      if(iter == other._values.end())
        return false;
      if(iter->second != p.second)
        return false;
    }
    return true;
  }
  std::size_t hash() const {
    std::size_t h = std::hash<std::string>{}(_instance.model().name());
    for(const auto &p : _values) {
      std::size_t hs = std::hash<std::string>{}(p.first);
      h = hs ^ (h << 1);
      hs = std::hash<std::string>{}(p.second);
      h = hs ^ (h << 2);
    }
    return h;
  }
  
  std::vector<std::pair<std::string,std::string>>
    instantiate() const {
    return DataModel::instantiate(_instance.model(), _values);
  }
  const fetch::oef::pb::Query_DataModel &model() const {
    return _instance.model();
  }
  stde::optional<std::string> value(const std::string &name) const {
    auto iter = _values.find(name);
    if(iter == _values.end())
      return stde::nullopt;
    return stde::optional<std::string>{iter->second};
  }
};

namespace std
{  
  template<> struct hash<Instance>  {
    typedef Instance argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept
    {
      return s.hash();
    }
  };
}

class Or;
class And;

class ConstraintType {
 public:
  using ValueType = var::variant<var::recursive_wrapper<Or>,var::recursive_wrapper<And>,Range,Relation,Set>;
 private:
  fetch::oef::pb::Query_Constraint_ConstraintType _constraint;
 public:
  explicit ConstraintType(const Or &or_);
  explicit ConstraintType(const And &and_);
  explicit ConstraintType(const Range &range) {
    auto *r = _constraint.mutable_range();
    r->CopyFrom(range.handle());
  }
  explicit ConstraintType(const Relation &rel) {
    auto *r = _constraint.mutable_rel();
    r->CopyFrom(rel.handle());
  }
  explicit ConstraintType(const Set &set) {
    auto *s = _constraint.mutable_set();
    s->CopyFrom(set.handle());
  }
  const fetch::oef::pb::Query_Constraint_ConstraintType &handle() const { return _constraint; }
  static bool check(const fetch::oef::pb::Query_Constraint_ConstraintType &constraint, const VariantType &v);
  bool check(const VariantType &v) const {
    return check(_constraint, v);
  }
};

class Constraint {
 private:
  fetch::oef::pb::Query_Constraint _constraint;
 public:
  explicit Constraint(const Attribute &attribute, const ConstraintType &constraint) {
    auto *c = _constraint.mutable_attribute();
    c->CopyFrom(attribute.handle());
    auto *ct = _constraint.mutable_constraint();
    ct->CopyFrom(constraint.handle());
  }
  const fetch::oef::pb::Query_Constraint &handle() const { return _constraint; }
  static bool check(const fetch::oef::pb::Query_Constraint &constraint, const VariantType &v) {
    return ConstraintType::check(constraint.constraint(), v);
  }
  static bool check(const fetch::oef::pb::Query_Constraint &constraint, const Instance &i) {
    auto &attribute = constraint.attribute();
    auto attr = DataModel::attribute(i.model(), attribute.name());
    if(attr) {
      if(attr->type() != attribute.type())
        return false;
    }
    auto v = i.value(attribute.name());
    if(!v) {
      if(attribute.required())
        std::cerr << "Should not happen!\n"; // Exception ?
      return false;
    }
    VariantType value{string_to_value(attribute.type(), *v)};
    return check(constraint, value);
  }
  bool check(const VariantType &v) const {
    return check(_constraint, v);
  }
};

class Or {
  fetch::oef::pb::Query_Constraint_ConstraintType_Or _expr;
 public:
  explicit Or(const std::vector<ConstraintType> &expr) {
    auto *cts = _expr.mutable_expr();
    for(auto &e : expr) {
      auto *ct = cts->Add();
      ct->CopyFrom(e.handle());
    }
  }
  const fetch::oef::pb::Query_Constraint_ConstraintType_Or &handle() const { return _expr; }
  static bool check(const fetch::oef::pb::Query_Constraint_ConstraintType_Or &expr, const VariantType &v) {
    for(auto &c : expr.expr()) {
      if(ConstraintType::check(c, v))
        return true;
    }
    return false;
  }
};

class And {
 private:
  fetch::oef::pb::Query_Constraint_ConstraintType_And _expr;
 public:
  explicit And(const std::vector<ConstraintType> &expr) {
    auto *cts = _expr.mutable_expr();
    for(auto &e : expr) {
      auto *ct = cts->Add();
      ct->CopyFrom(e.handle());
    }
  }
  const fetch::oef::pb::Query_Constraint_ConstraintType_And &handle() const { return _expr; }
  static bool check(const fetch::oef::pb::Query_Constraint_ConstraintType_And &expr, const VariantType &v) {
    for(auto &c : expr.expr()) {
      if(!ConstraintType::check(c, v))
        return false;
    }
    return true;
  }
};

class QueryModel {
 private:
  fetch::oef::pb::Query_Model _model;
 public:
  explicit QueryModel(const std::vector<Constraint> &constraints) {
    auto *cts = _model.mutable_constraints();
    for(auto &c : constraints) {
      auto *ct = cts->Add();
      ct->CopyFrom(c.handle());
    }
  }
  explicit QueryModel(const std::vector<Constraint> &constraints, const DataModel &model) : QueryModel{constraints} {
    auto *m = _model.mutable_model();
    m->CopyFrom(model.handle());
  }
  explicit QueryModel(const fetch::oef::pb::Query_Model &model) : _model{model} {}
  const fetch::oef::pb::Query_Model &handle() const { return _model; }
  template <typename T>
    bool check_value(const T &v) const {
    for(auto &c : _model.constraints()) {
      if(!Constraint::check(c, VariantType{v}))
        return false;
    }
    return true;
  }
  bool check(const Instance &i) const {
    if(_model.has_model()) {
      if(_model.model().name() != i.model().name())
        return false;
      // TODO: more to compare ?
    }
    for(auto &c : _model.constraints()) {
      if(!Constraint::check(c, i))
        return false;
    }
    return true;
  }
};

class SchemaRef {
 private:
  std::string _name; // unique
  uint32_t _version;
 public:
  explicit SchemaRef(const std::string &name, uint32_t version) : _name{name}, _version{version} {}
  std::string name() const { return _name; }
  uint32_t version() const { return _version; }
};

class Schema {
 private:
  uint32_t _version;
  DataModel _schema;
 public:
  explicit Schema(uint32_t version, const DataModel &schema) : _version{version}, _schema{schema} {}
  uint32_t version() const { return _version; }
  DataModel schema() const { return _schema; }
};
  
class Schemas {
 private:
  mutable std::mutex _lock;
  std::vector<Schema> _schemas;
 public:
  explicit Schemas() = default;
  uint32_t add(uint32_t version, const DataModel &schema) {
    std::lock_guard<std::mutex> lock(_lock);
    if(version == std::numeric_limits<uint32_t>::max())
      version = _schemas.size() + 1;
    _schemas.emplace_back(Schema(version, schema));
    return version;
  }
  stde::optional<Schema> get(uint32_t version) const {
    std::lock_guard<std::mutex> lock(_lock);
    if(_schemas.size() == 0)
      return stde::nullopt;
    if(version == std::numeric_limits<uint32_t>::max())
      return _schemas.back();
    for(auto &p : _schemas) { // TODO: binary search
      if(p.version() >= version)
        return p;
    }
    return _schemas.back(); 
  }
};

class SchemaDirectory {
 private:
  std::unordered_map<std::string, Schemas> _schemas;
 public:
  explicit SchemaDirectory() = default;
  stde::optional<Schema> get(const std::string &key, uint32_t version = std::numeric_limits<uint32_t>::max()) const {
    const auto &iter = _schemas.find(key);
    if(iter != _schemas.end())
      return iter->second.get(version);
    return stde::nullopt;
  }
  uint32_t add(const std::string &key, const DataModel &schema, uint32_t version = std::numeric_limits<uint32_t>::max()) {
    return _schemas[key].add(version, schema);
  }
};
