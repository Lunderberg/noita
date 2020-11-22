#include <algorithm>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

std::vector<std::string> LIQUIDS{
  "water", "water_ice", "water_swamp",
    "oil", "alcohol", "swamp", "mud", "blood",
    "blood_fungi", "blood_worm", "radioactive_liquid",
    "cement", "acid", "lava", "urine",
    "poison", "magic_liquid_teleportation",
    "magic_liquid_polymorph", "magic_liquid_random_polymorph",
    "magic_liquid_berserk", "magic_liquid_charm",
    "magic_liquid_invisibility"};

std::vector<std::string> ORGANICS{
  "sand", "bone", "soil", "honey",
    "slime", "snow", "rotten_meat", "wax",
    "gold", "silver", "copper", "brass", "diamond",
    "coal", "gunpowder", "gunpowder_explosive",
    "grass", "fungi"};

enum class MaterialType {liquid, organic};

using Material = std::tuple<MaterialType,int>;

struct WorldRecipes {
  std::vector<Material> lively_concoction;
  std::vector<Material> alchemic_precursor;
};

class RNG {
public:
  RNG(int state)
    : state(state) { }

  std::uint32_t gen() {
    auto hi = state / 127773;
    auto lo = state % 127773;
    state = 16807*lo - 2836*hi;
    if(state >= (1U<<31)) {
      state -= (1U<<31);
    }
    return state;
  }

  unsigned int rand_int(int max) {
    double x = double(gen()) / (1U<<31);
    return x*max;
  }

private:
  std::uint32_t state;
};

void shuffle(std::vector<Material>& vec, std::uint32_t seed) {
  RNG rng( (seed>>1) + 0x30f6);
  rng.gen();
  for(int i=vec.size()-1; i >= 0; i--) {
    auto new_index = rng.rand_int(i+1);
    std::swap(vec[i], vec[new_index]);
  }
}

std::vector<Material> random_recipe(RNG& rng, std::uint32_t seed) {
  std::vector<Material> mats;
  while(mats.size() < 3) {
    auto new_mat = Material(
      MaterialType::liquid,
      rng.rand_int(LIQUIDS.size())
    );

    if(std::find(mats.begin(), mats.end(), new_mat) == mats.end()) {
      mats.push_back(new_mat);
    }
  }

  mats.push_back({MaterialType::organic, rng.rand_int(ORGANICS.size())});

  shuffle(mats, seed);
  mats.pop_back();

  return mats;
}

std::ostream& operator<<(std::ostream& stream, const std::vector<Material>& vec) {
  bool is_first = true;

  for(auto [type,index] : vec) {
    if(!is_first) {
      stream << ", ";
    }

    switch(type) {
      case MaterialType::liquid:
        stream << LIQUIDS.at(index);
        break;

      case MaterialType::organic:
        stream << ORGANICS.at(index);
        break;
    }

    is_first = false;
  }
  return stream;
}


WorldRecipes generate_recipes(std::uint32_t seed) {
  RNG rng(seed*0.17127 + 1323.5903);

  for(int i=0; i<6; i++) { rng.gen(); }

  auto lc_recipe = random_recipe(rng, seed);
  rng.gen();
  rng.gen();
  auto ap_recipe = random_recipe(rng, seed);

  return {lc_recipe, ap_recipe};
}

Material get_material(const std::string& name) {
  for(unsigned int i=0; i<LIQUIDS.size(); i++) {
    if(LIQUIDS[i] == name) {
      return {MaterialType::liquid, i};
    }
  }

  for(unsigned int i=0; i<ORGANICS.size(); i++) {
    if(ORGANICS[i] == name) {
      return {MaterialType::organic, i};
    }
  }

  std::stringstream ss;
  ss << "Unknown material: " << name;
  throw std::runtime_error(ss.str());
}


std::map<Material, int> parse_costs(std::string filename) {
  std::ifstream file(filename);

  std::map<Material, int> costs;

  std::string name;
  int cost;
  while(file >> name >> cost) {
    costs[get_material(name)] = cost;
  }

  return costs;
}

void minimize_costs(const std::map<Material, int>& costs) {
  std::vector<int> cost_list;
  for(const auto& pair : costs) {
    cost_list.push_back(pair.second);
  }
  std::sort(cost_list.begin(), cost_list.end());
  int best_possible_cost = cost_list[0] + cost_list[1] + cost_list[2];

  int best_cost = 1e9;
  int num_shown = 0;

  for(unsigned int seed = 1; seed<1e9; seed++) {
    auto recipes = generate_recipes(seed);

    int total_cost = 0;
    for(auto& mat : recipes.lively_concoction) {
      total_cost += costs.at(mat);
    }

    if(total_cost <= best_cost) {
      std::cout << "Seed: " << seed << "\tCost: " << total_cost << "\tLC: " << recipes.lively_concoction << std::endl;
      best_cost = total_cost;

      if(best_cost == best_possible_cost) {
        num_shown++;
        if(num_shown > 100) {
          break;
        }
      }

    }
  }
}


int main(int argc, char** argv) {
  if(argc < 2){
    std::cerr << "Usage: recipes seed" << std::endl;
    std::cerr << "Usage: recipes cost_file" << std::endl;
    return 1;
  }

  int seed = atoi(argv[1]);
  if(seed == 0) {
    auto costs = parse_costs(argv[1]);
    minimize_costs(costs);
  } else {
    auto recipes = generate_recipes(seed);
    std::cout << "Seed: " << seed << std::endl;
    std::cout << "Lively Concoction: " << recipes.lively_concoction << std::endl;
    std::cout << "Alchemic Precursor: " << recipes.alchemic_precursor << std::endl;
  }
}
