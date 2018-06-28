#pragma once
#include <vector>
#include <sstream>

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
  uint32_t rows() const { return _rows; }
  uint32_t cols() const { return _cols; }
  T get(uint32_t row, uint32_t col) const {
    assert(row < _rows);
    assert(col < _cols);
    return _data[row * _cols + col];
  }
  void set(uint32_t row, uint32_t col, const T &t) {
    assert(row < _rows);
    assert(col < _cols);
    _data[row * _cols + col] = t;
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
