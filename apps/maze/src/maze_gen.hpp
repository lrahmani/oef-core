#pragma once
#include "grid.h"
#include <cassert>
#include <set>
#include <vector>
#include <memory>

namespace fetch_maze {

struct Wall;

using coord2D = std::array<std::size_t, 2>;
using sptr_wall = std::shared_ptr<Wall>;
using div_type = std::tuple<sptr_wall,
        std::vector<sptr_wall>,
        std::vector<sptr_wall>>;

enum div_fields {
    div_wall, first_box, second_box
};

struct Wall {
public:

    bool horizontal;
    size_t index, entrance;
    coord2D range;
    static size_t margin;

    Wall(bool orient, size_t ii, coord2D in_range, size_t ent = 0) :
            horizontal(orient), index(ii), entrance(ent), range(in_range) {}

    void print() {
        std::cout << ((horizontal) ? "Horizontal" : "Vertical") << " wall at: " << index
                  << ", Position : (" << range[0] << ", " << range[1] << "), entrance at: "
                  << entrance << std::endl;
    }
};

std::size_t    Wall::margin = 2;

sptr_wall intervening_wall(const Wall &a, const Wall &b) {
    assert(a.horizontal == b.horizontal);
    assert(a.range[0] == b.range[0] and a.range[1] == b.range[1]);
    assert(a.index < b.index);

    if (a.range[1] - a.range[0] < 2 * a.margin + 1)
        return nullptr;

    auto entrances = std::set<size_t>();

    if (a.entrance)
        entrances.insert(a.entrance);

    if (b.entrance)
        entrances.insert(b.entrance);


    if (a.range[1] <= (entrances.size() + (2 * a.margin) + a.range[0]))
        return nullptr;

    size_t denominator = a.range[1] - (entrances.size() + (2 * a.margin) + a.range[0]);

    size_t position = a.range[0] + a.margin + rand() % denominator;

    // shift wall to avoid entrances
    for (const auto &an_entrance : entrances) {
        if (position >= an_entrance)
            ++position;
    }

    return sptr_wall(new Wall(!a.horizontal, position, {{a.index, b.index + 1}},
                              a.index + 1 + rand() % (b.index - (1 + a.index))));
}


std::vector<sptr_wall> initialize_box(size_t n_rows, size_t n_cols) {
    // starting box
    auto box = std::vector<sptr_wall>();
    box.emplace_back(std::make_shared<Wall>(true, 0, coord2D{{0, n_cols}}));
    box.emplace_back(std::make_shared<Wall>(true, n_rows - 1, coord2D{{0, n_cols}}));
    box.emplace_back(std::make_shared<Wall>(false, 0, coord2D{{0, n_rows}}));
    box.emplace_back(std::make_shared<Wall>(false, n_cols - 1, coord2D{{0, n_rows}}));

    // sample entrance
    std::size_t wall = rand() % 4;
    box.at(wall)->entrance = 1 + ((wall < 2) ? rand() % (n_cols - 2) : rand() % (n_rows - 2));
    return box;
}

coord2D box_dimensions(const std::vector<sptr_wall> &box) {
    assert(box.size() == 4);
    return {{1 + box.at(1)->index - box.at(0)->index, 1 + box.at(3)->index - box.at(2)->index}};
}

std::vector<sptr_wall>
derived_walls(const Wall &parent, const Wall &divider) {
    assert(parent.horizontal != divider.horizontal);
    std::vector<sptr_wall> new_walls = std::vector<sptr_wall>();

    // "upper" divided wall
    new_walls.emplace_back(std::make_shared<Wall>(parent.horizontal,
                                                  parent.index,
                                                  coord2D{{parent.range[0], divider.index + 1}},
                                                  (parent.entrance < divider.index) ? parent.entrance : 0));

    // "lower" divided wall
    new_walls.emplace_back(std::make_shared<Wall>(parent.horizontal,
                                                  parent.index,
                                                  coord2D{{divider.index, parent.range[1]}},
                                                  (parent.entrance > divider.index) ? parent.entrance : 0));
    return new_walls;
}

std::tuple<sptr_wall, std::vector<sptr_wall>, std::vector<sptr_wall>>
divide_walls(const std::vector<sptr_wall> &box, bool horizontal) {

    static auto null_tuple = std::make_tuple(nullptr, std::vector<sptr_wall>(), std::vector<sptr_wall>());
    std::vector<sptr_wall> box1, box2, child_walls_a, child_walls_b;
    sptr_wall divide_wall;

    if (horizontal) {
        divide_wall = intervening_wall(*box.at(2), *box.at(3));
        if (!divide_wall)
            return null_tuple;

        child_walls_a = derived_walls(*box.at(2), *divide_wall);
        child_walls_b = derived_walls(*box.at(3), *divide_wall);
        box1 = {box.at(0), divide_wall, child_walls_a[0], child_walls_b[0]};
        box2 = {divide_wall, box.at(1), child_walls_a[1], child_walls_b[1]};
    } else {
        divide_wall = intervening_wall(*box.at(0), *box.at(1));
        if (!divide_wall)
            return null_tuple;

        child_walls_a = derived_walls(*box.at(0), *divide_wall);
        child_walls_b = derived_walls(*box.at(1), *divide_wall);
        box1 = {child_walls_a[0], child_walls_b[0], box.at(2), divide_wall};
        box2 = {child_walls_a[1], child_walls_b[1], divide_wall, box.at(3)};
    }
    return std::make_tuple(divide_wall, box1, box2);
}

template<class T>
void add_wall_to_maze(Grid <T> &maze, const Wall &wall, T value = 1) {
    if (wall.horizontal) {
      for(auto col = wall.range[0]; col < wall.range[1]; ++col)
        maze.set(wall.index, col, value);
      if (wall.entrance > 0)
        maze.set(wall.index, wall.entrance, 0);
    } else {
      for(auto row = wall.range[0]; row < wall.range[1]; ++row)
        maze.set(row, wall.index, value);
      if (wall.entrance > 0)
        maze.set(wall.entrance, wall.index, 0);
    }
}

Grid<bool> maze_from_walls(const std::vector<sptr_wall> &all_walls, bool show_output = false) {
    // first four walls give top, bottom, left, right
    assert(all_walls.size() >= 4);
    size_t n_cols = all_walls.at(0)->range[1];
    size_t n_rows = all_walls.at(2)->range[1];
    Grid<bool> maze = Grid<bool>(n_rows, n_cols);

    for (const auto &wall_ptr : all_walls)
        add_wall_to_maze(maze, *wall_ptr);

    if (show_output) {
        for (const auto &wall_ptr : all_walls)
            wall_ptr->print();
    }

    return maze;
}

void isotropic_divide_walls(const std::vector<sptr_wall> &box,
                            Grid<bool> &maze) {

    if (box.size() != 4)
        return;

    coord2D box_dims = box_dimensions(box);

    // division is along the long axis
    bool horizontal = (box_dims[0] > box_dims[1]);
    if (box_dims[horizontal] < 2 * box.at(0)->margin)  // shortest axis is below threshold
        return;

    div_type div = divide_walls(box, horizontal);

    sptr_wall candidate_wall = std::get<div_wall>(div);

    if (!candidate_wall) // wall is a null pointer
        return;

    add_wall_to_maze<bool>(maze, *candidate_wall);

    isotropic_divide_walls(std::get<first_box>(div), maze);
    isotropic_divide_walls(std::get<second_box>(div), maze);
}

Grid<bool> recursive_maze(size_t n_rows, size_t n_cols)
{
    // display grid
    Grid<bool> maze = Grid<bool>(n_rows, n_cols);

    // initial set of walls and store for final set
    auto box = initialize_box(n_rows, n_cols);

    for (const auto &ptr_wall : box)
        add_wall_to_maze(maze, *ptr_wall);

    isotropic_divide_walls(box, maze);

    return maze;
}

};
