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

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "schema.hpp"
#include <iostream>
#include "servicedirectory.hpp"
#include <google/protobuf/text_format.h>
#include "common.hpp"

using namespace fetch::oef;

namespace Test {

  TEST_CASE("schema", "[creation]") {
    fetch::oef::DataModel d1{"Person", {
        Attribute{"firstName", Type::String, true, "The first name."},
          Attribute{"lastName", Type::String, true},
            Attribute{"age", Type::Int, false, "The age of the person."}}};
    SchemaDirectory sd;
    sd.add("person", d1);
    auto s1 = sd.get("person");
    REQUIRE(s1);
    REQUIRE(s1->version() == 1);
    REQUIRE(s1->schema() == d1);
    DataModel d2{"Person2", {
        Attribute{"firstName", Type::String, true, "The first name."},
          Attribute{"lastName", Type::String, true},
            Attribute{"age", Type::Int, false, "The age of the person."}}};
    sd.add("person", d2);
    auto s2 = sd.get("person");
    REQUIRE(s2->version() == 2);
    REQUIRE(s2->schema() == d2);
    auto s3 = sd.get("person", 1);
    REQUIRE(s3->version() == 1);
    REQUIRE(s3->schema() == d1);
    //    std::cerr << toJsonString<SchemaDirectory>(sd);
  }
  TEST_CASE("schema serialization", "[serialization]") {
    Attribute att1{"ID", Type::Int, true};

    std::string output;
    REQUIRE(google::protobuf::TextFormat::PrintToString(att1.handle(), &output));
    std::cout << output;
    auto buffer = serialize(att1.handle());
    auto a1b = deserialize<fetch::oef::pb::Query_Attribute>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(a1b, &output));
    std::cout << output;
    
    Attribute att2{"firstName", Type::String, true, "The first name."};
    REQUIRE(google::protobuf::TextFormat::PrintToString(att2.handle(), &output));
    std::cout << output;
    buffer = serialize(att2.handle());
    auto a2b = deserialize<fetch::oef::pb::Query_Attribute>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(a2b, &output));
    std::cout << output;

    DataModel datamodel1{"Person", {att2, Attribute{"lastName", Type::String, true},
                                    Attribute{"age", Type::Int, false, "The age of the person."},
                                    Attribute{"weight", Type::Double, false},
                                    Attribute{"married", Type::Bool, false},
                                    Attribute{"birth_place", Type::Location, false}}};
    REQUIRE(google::protobuf::TextFormat::PrintToString(datamodel1.handle(), &output));
    std::cout << output;
    buffer = serialize(datamodel1.handle());
    auto d1b = deserialize<fetch::oef::pb::Query_DataModel>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(d1b, &output));
    std::cout << output;

    Range range5to10{std::make_pair(5,10)};
    REQUIRE(google::protobuf::TextFormat::PrintToString(range5to10.handle(), &output));
    std::cout << output;
    REQUIRE(range5to10.check(VariantType{6}));
    REQUIRE(!range5to10.check(VariantType{12}));
    buffer = serialize(range5to10.handle());
    auto r1b = deserialize<fetch::oef::pb::Query_Range>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(r1b, &output));
    std::cout << output;

    Set set1_3_5{Set::Op::In, std::unordered_set<int>{1,3,5}};
    REQUIRE(google::protobuf::TextFormat::PrintToString(set1_3_5.handle(), &output));
    std::cout << output;
    REQUIRE(set1_3_5.check(VariantType{3}));
    REQUIRE(!set1_3_5.check(VariantType{2}));
    buffer = serialize(set1_3_5.handle());
    auto s1b = deserialize<fetch::oef::pb::Query_Set>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(s1b, &output));
    std::cout << output;

    Relation relLt5{Relation::Op::Lt, 5};
    REQUIRE(google::protobuf::TextFormat::PrintToString(relLt5.handle(), &output));
    std::cout << output;
    REQUIRE(relLt5.check(VariantType{3}));
    REQUIRE(!relLt5.check(VariantType{7}));
    buffer = serialize(relLt5.handle());
    auto rel1b = deserialize<fetch::oef::pb::Query_Relation>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(rel1b, &output));
    std::cout << output;

    Constraint range5to10_cons{att1.name(), range5to10};
    REQUIRE(google::protobuf::TextFormat::PrintToString(range5to10_cons.handle(), &output));
    std::cout << output;
    REQUIRE(range5to10_cons.check(VariantType{7}));
    REQUIRE(!range5to10_cons.check(VariantType{3}));
    buffer = serialize(range5to10_cons.handle());
    auto c1b = deserialize<fetch::oef::pb::Query_ConstraintExpr_Constraint>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(c1b, &output));
    std::cout << output;
    
    std::vector<ConstraintExpr> v{ConstraintExpr{range5to10_cons},ConstraintExpr{Constraint{att1.name(), set1_3_5}}};
    ConstraintExpr c2{Or{v}};
    REQUIRE(google::protobuf::TextFormat::PrintToString(c2.handle(), &output));
    std::cout << output;
    REQUIRE(c2.check(VariantType{3}));
    REQUIRE(!c2.check(VariantType{2}));
    buffer = serialize(c2.handle());
    auto c2b = deserialize<fetch::oef::pb::Query_ConstraintExpr>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(c2b, &output));
    std::cout << output;
    
    ConstraintExpr c3{And{v}};
    REQUIRE(google::protobuf::TextFormat::PrintToString(c3.handle(), &output));
    std::cout << output;
    REQUIRE(c3.check(VariantType{5}));
    REQUIRE(!c3.check(VariantType{3}));
    buffer = serialize(c3.handle());
    auto c3b = deserialize<fetch::oef::pb::Query_ConstraintExpr>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(c3b, &output));
    std::cout << output;

    Set setAlan_Chris{Set::Op::In, std::unordered_set<std::string>{"Alan", "Chris"}};
    ConstraintExpr c4{Constraint{att2.name(), setAlan_Chris}};
    QueryModel q1{{c4}, datamodel1};
    REQUIRE(google::protobuf::TextFormat::PrintToString(q1.handle(), &output));
    std::cout << output;
    buffer = serialize(c3.handle());
    auto q1b = deserialize<fetch::oef::pb::Query_Model>(*buffer);
    REQUIRE(google::protobuf::TextFormat::PrintToString(q1b, &output));
    std::cout << output;
    REQUIRE(q1.check_value<std::string>("Alan"));
    REQUIRE(!q1.check_value<std::string>("Mark"));
  }
  TEST_CASE("meteo", "[omf2]") {
    // infos for the ServiceDirectory
    Attribute wind{"wind_speed", Type::Bool, true};
    Attribute temp{"temperature", Type::Bool, true};
    Attribute air{"air_pressure", Type::Bool, true};
    Attribute humidity{"humidity", Type::Bool, true};
    std::vector<Attribute> attributes{wind,temp,air,humidity};
    DataModel weather{"weather_data", attributes, "All possible weather data."};
    // data that will be actually sent
    std::string output;
    REQUIRE(google::protobuf::TextFormat::PrintToString(weather.handle(), &output));
    std::cout << output;

    Attribute wind_data{"wind_speed", Type::Double, false};
    Attribute temp_data{"temperature", Type::Double, false};
    Attribute air_data{"air_pressure", Type::Double, false};
    Attribute humidity_data{"humidity", Type::Double, false};
    DataModel weather_data{"weather_data", {wind_data,temp_data,air_data,humidity_data}, "Weather data."};
    REQUIRE(google::protobuf::TextFormat::PrintToString(weather_data.handle(), &output));
    std::cout << output;
    // Infos for the AgentDirectory
    Attribute manufacturer{"manufacturer", Type::String, true};
    Attribute model{"model", Type::String, true};
    Attribute wireless{"wireless", Type::Bool, true};
    DataModel station{"weather_station", {manufacturer, model, wireless}, "Weather station"};
    REQUIRE(google::protobuf::TextFormat::PrintToString(station.handle(), &output));
    std::cout << output;
    Instance youshiko{station, {{"manufacturer", VariantType{std::string{"Youshiko"}}},
                                {"model", VariantType{std::string{"YC9315"}}},
                                {"wireless", VariantType{true}}}};
    Instance opes{station, {{"manufacturer", VariantType{std::string{"Opes"}}},
                            {"model", VariantType{std::string{"17500"}}}, {"wireless", VariantType{true}}}};
    REQUIRE(google::protobuf::TextFormat::PrintToString(youshiko.handle(), &output));
    std::cout << output;
    // Instance youshiko2 = fromJsonString<Instance>(toJsonString<Instance>(youshiko));
    // std::cout << toJsonString<Instance>(youshiko2) << "\n";

    ServiceDirectory sd;
    for(size_t i = 0; i < attributes.size(); ++i) {
      std::unordered_map<std::string,VariantType> values;
      for(size_t j = 0; j < attributes.size(); ++j) {
        values.emplace(std::make_pair(attributes[j].name(), VariantType{i != j}));
      }
      std::string name = "Agent"+ std::to_string(i+1);
      sd.registerAgent(Instance{weather, values}, name);
      REQUIRE(sd.size() == (i + 1));
    }

    Relation eqTrue{Relation::Op::Eq, true};
    Constraint temp_c{temp.name(), eqTrue};
    Constraint wind_c{wind.name(), eqTrue};
    Constraint air_c{air.name(), eqTrue};
    Constraint humidity_c{humidity.name(), eqTrue};
    QueryModel q1{{ConstraintExpr{temp_c}}, weather};
    auto agents1 = sd.query(q1);
    REQUIRE(agents1.size() == 3);
    QueryModel q2{{ConstraintExpr{temp_c},ConstraintExpr{wind_c}}, weather};
    auto agents2 = sd.query(q2);
    REQUIRE(agents2.size() == 2);
    QueryModel q3{{ConstraintExpr{temp_c},ConstraintExpr{wind_c},ConstraintExpr{air_c}}, weather};
    auto agents3 = sd.query(q3);
    REQUIRE(agents3.size() == 1);
    QueryModel q4{{ConstraintExpr{temp_c},ConstraintExpr{wind_c},ConstraintExpr{air_c}, ConstraintExpr{humidity_c}}, weather};
    auto agents4 = sd.query(q4);
    REQUIRE(agents4.empty());

    // auto e1 = Envelope::makeDescription(youshiko);
    // std::string s = toJsonString<Envelope>(e1);
    // std::cout << s << "\ndeserialization\n";
    // auto e2 = fromJsonString<Envelope>(s);
    // std::cout << toJsonString<Envelope>(e2);
  }
}
