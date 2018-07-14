#pragma once
#include <vector>
#include <sstream>
#include <functional>

using Position = std::pair<uint32_t,uint32_t>;

template <typename T>
class Grid {
 private:
  uint32_t _rows;
  uint32_t _cols;
  std::vector<T> _data;
 public:
  explicit Grid(uint32_t rows, uint32_t cols) : _rows{rows}, _cols{cols} {
    _data.resize(_rows * _cols);
  }
  Grid(Grid<T> &&other) : _rows{other._rows}, _cols{other._cols}, _data{std::move(other._data)} {}
  uint32_t rows() const { return _rows; }
  uint32_t cols() const { return _cols; }
  T get(uint32_t row, uint32_t col) const {
    assert(row < _rows);
    assert(col < _cols);
    return _data[row * _cols + col];
  }
  T get(const Position &pos) const {
    return get(pos.first, pos.second);
  }
  void set(uint32_t row, uint32_t col, const T &t) {
    assert(row < _rows);
    assert(col < _cols);
    _data[row * _cols + col] = t;
  }
  void set(const Position &pos, const T &t) {
    set(pos.first, pos.second, t);
  }
  std::vector<Position> filter(std::function<bool(const T&)> pred) {
    std::vector<Position> res;
    for(uint32_t i = 0; i < _rows; ++i)
      for(uint32_t j = 0; j < _cols; ++j)
        if(pred(_data[i * _cols + j]))
          res.emplace_back(std::make_pair(i, j));
    return res;
  }
  std::string to_string() const {
    std::stringstream ss;
    for(uint32_t r = 0; r < _rows; ++r) {
      uint32_t base = r * _cols;
      for(uint32_t c = 0; c < _cols; ++c) {
        ss << static_cast<int>(_data[base + c]);
      }
      ss << "\n";
    }
    return ss.str();
  }
};
