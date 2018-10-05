#include <hayai.hpp>
#include "agent.pb.h"
#include <iostream>

std::vector<uint8_t> data;

BENCHMARK(Serialization, ID, 10, 1000)
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_public_key("Agent1");
  std::string data(id.SerializeAsString());
}

BENCHMARK(Serialization, ID_Array, 10, 1000)
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_public_key("Agent1");
  size_t size = id.ByteSize();
  data.resize(size);
  (void)id.SerializeWithCachedSizesToArray(data.data());
}

BENCHMARK(Serialization, Phrase, 10, 1000)
{
  fetch::oef::pb::Server_Phrase phrase;
  phrase.set_phrase("RandomlyGeneratedString");
  std::string data(phrase.SerializeAsString());
}

BENCHMARK(Serialization, Phrase_Array, 10, 1000)
{
  fetch::oef::pb::Server_Phrase phrase;
  phrase.set_phrase("RandomlyGeneratedString");
  size_t size = phrase.ByteSize();
  data.resize(size);
  (void)phrase.SerializeWithCachedSizesToArray(data.data());
}

BENCHMARK(Serialization, Answer, 10, 1000)
{
  fetch::oef::pb::Agent_Server_Answer answer;
  answer.set_answer("gnirtSdetareneGylmodnaR");
  std::string data(answer.SerializeAsString());
}

BENCHMARK(Serialization, Answer_Array, 10, 1000)
{
  fetch::oef::pb::Agent_Server_Answer answer;
  answer.set_answer("gnirtSdetareneGylmodnaR");
  size_t size = answer.ByteSize();
  data.resize(size);
  answer.SerializeWithCachedSizesToArray(data.data());
}

BENCHMARK(Serialization, Connected, 100, 1000)
{
  fetch::oef::pb::Server_Connected good;
  good.set_status(true);
  std::string data(good.SerializeAsString());
}

BENCHMARK(Serialization, Connected_Array, 1, 1)
{
  fetch::oef::pb::Server_Connected good;
  good.set_status(true);
  size_t size = good.ByteSize();
  data.resize(size);
  (void)good.SerializeWithCachedSizesToArray(data.data());
}

BENCHMARK(Serialization, Connected_Array2, 1, 1)
{
  fetch::oef::pb::Server_Connected good;
  good.set_status(true);
  size_t size = good.ByteSize();
  data.resize(size);
  good.SerializeToArray(data.data(), size);
}

BENCHMARK(DeSerialization, ID, 10, 1000)
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_public_key("Agent1");
  fetch::oef::pb::Agent_Server_ID id2;
  id2.ParseFromString(id.SerializeAsString());
}

BENCHMARK(DeSerialization, ID_Array, 10, 1000)
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_public_key("Agent1");
  size_t size = id.ByteSize();
  data.resize(size);
  (void)id.SerializeWithCachedSizesToArray(data.data());
  fetch::oef::pb::Agent_Server_ID id2;
  id2.ParseFromArray(data.data(), size);
}

