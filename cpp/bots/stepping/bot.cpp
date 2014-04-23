#include "bots/stepping/bot.h"
#include <cstring>

using std::string;
using std::vector;
using std::map;

using game::Position;
using game::CarTracker;
using game::Race;
using game::Command;

namespace bots {
namespace stepping {

Bot::Bot() {
  srand(time(0));
}

game::Command Bot::GetMove(const map<string, Position>& positions, int game_tick)  {
  Position position = positions.at(color_);
  Position previous;

  if (car_tracker_->positions().size() == 0) {
    previous = position;
  } else {
    previous = car_tracker_->positions().back();
  }

  car_tracker_->Record(position);

  if (crashed_) {
    return Command(0);
  }

  double throttle = Optimize(previous, position);
  if (game_tick < 10) throttle = 1;

  if (CanUseTurbo(position)) {
    printf("YABADABADUUUUU :D\n");
    turbo_on_ = turbo_.duration() - 1;
    turbo_available_ = false;
    return Command(game::TurboToggle::kToggleOn);
  }

  /*game::Switch s;
  if (ShouldChangeLane(position, &s)) {
    printf("Switch!\n");
    return Command(s);
  }*/

  if (turbo_on_ > 0) {
    // We dont want to spoil model before its done
    turbo_on_--;
    return Command(1);
  }

  if (position.lap() == 2 && race_.track().IsLastStraight(position))
    return Command(1);

  car_tracker_->RecordThrottle(throttle);
  return Command(throttle);
}

// Returns optimal throttlle:
double Bot::Optimize(const Position& previous, const Position& current) {
  // Length of time units in 0/1 search
  vector<int> groups
      { 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
  //    1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19  <-- counter

  // Optimal
  // { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 } 7.43 keimola (just find_mask & 1 )
  // { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3 }; 7.38 keimola / 7.03 with ending turbo
  // { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }; 7.37 keimola
  // { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3 }; germany 8.6 with turbo

  double distance;
  double best_distance = 0;
  double throttle = 0;
  int best_mask = -1;

  //best_mask = FindBestMask(previous, current, groups, &distance);
  //throttle = best_mask & 1;

  // Check no-speed
  Position next = car_tracker_->Predict(current, previous, 0, 0);
  int mask = FindBestMask(current, next, groups, &distance);
  distance += race_.track().Distance(next, current);
  if (mask != -1 && distance > best_distance) {
    throttle = 0;
    best_distance = distance;
    best_mask = mask;
  }


  // Check (0, 1)
  double l = 0, r = 1, m;

  while (r - l > 1e-3) {
    m = (l + r) / 2.0;

    Position next = car_tracker_->Predict(current, previous, m, 0);
    int mask = FindBestMask(current, next, groups, &distance);
    distance += race_.track().Distance(next, current);

    if (mask == -1) {
      r = m;
    } else {
      l = m;
      if (distance > best_distance) {
        best_distance = distance;
        throttle = m;
        best_mask = mask;
      }
    }
  }

  // Check fullspeed
  next = car_tracker_->Predict(current, previous, 1, 0);
  mask = FindBestMask(current, next, groups, &distance);
  distance += race_.track().Distance(next, current);
  if (mask != -1 && distance > best_distance) {
    throttle = 1;
    best_distance = distance;
    best_mask = mask;
  }

  // Log predictions
  std::cout << std::setw(12) << throttle << " ";
  if (best_mask == -1)
    std::cout << "No successful mask";
  else
    for (int i = 0; i < groups.size(); i++)
      std::cout << ((best_mask & (1 << i)) > 0) << " ";
  std::cout << "(" << current.piece() << ")" << std::endl;

  return throttle;
}

// Finds most optimal(by means of distance travelled) mask
// @returns mask or -1 if impossible
// @param distance total distance travelled
int Bot::FindBestMask(const Position& previous, const Position& current, const vector<int>& groups, double* distance) {
  if (previous.angle() >= 60 - 1e-9 || current.angle() >= 60 - 1e-9)
    return -1;

  int best_mask = -1;
  *distance = 0;

  for (int mask = 0; mask < (1 << (groups.size())); mask++) {
    double mask_distance;
    if (CheckMask(mask, previous, current, groups, &mask_distance) && mask_distance > *distance) {
      *distance = mask_distance;
      best_mask = mask;
    }
  }
  return best_mask;
}

// Checks whether given throttle setup wont crash
// @returns false if car crashes
// @param distance total distance travelled
bool Bot::CheckMask(int mask, const Position& previous, const Position& current, const vector<int>& groups, double* distance) {
  vector<Position> positions {previous, current};
  int now = 1;
  *distance = 0;

  for (int g = 0; g < groups.size(); g++) {
    for (int t = 0; t < groups[g]; t++) {
      positions[now ^ 1] = car_tracker_->Predict(positions[now], positions[now ^ 1], ((mask & (1 << g)) > 0), 0);
      now ^= 1;
      (*distance) += race_.track().Distance(positions[now], positions[now ^ 1]);
      if (fabs(positions[now].angle()) >= 60 - 1e-9)
        return false;
    }
  }
  return true;
}

bool Bot::CanUseTurbo(const Position& position) {
  if (turbo_on_ > 0 || turbo_available_ == false)
    return false;

  // TODO hardcode
  //if (race_.track().id() == "keimola" && position.piece() == 36)
  //  return true;
  if (position.lap() == 2 && race_.track().IsLastStraight(position))
    return true;

  return false;
}

void Bot::JoinedGame() {
}

void Bot::YourCar(const string& color) {
  color_ = color;
}

void Bot::NewRace(const Race& race) {
  race_ = race;
  car_tracker_.reset(new CarTracker(&race_));
}

void Bot::GameStarted() {
  started_ = true;
}

void Bot::CarFinishedLap(const string& color /* + results */)  {
}

void Bot::CarFinishedRace(const string& color)  {
}

void Bot::GameEnd(/* results */)  {
}

void Bot::TournamentEnd()  {
}

void Bot::CarCrashed(const string& color)  {
  auto& previous = car_tracker_->positions()[car_tracker_->positions().size() - 2];
  auto& current = car_tracker_->positions().back();
  auto next = car_tracker_->Predict(current, previous, car_tracker_->throttle(), 0);
  printf("Crash! %lf %lf %lf\n", previous.angle(), current.angle(), next.angle());

  if (color == color_) {
    crashed_ = true;
    car_tracker_->RecordCarCrash();
  }
}

void Bot::CarSpawned(const string& color)  {
  if (color == color_)
    crashed_ = false;
}

void Bot::OnTurbo(const game::Turbo& turbo) {
  turbo_available_ = true;
  turbo_ = turbo;
}


// TODO refactor
bool Bot::ShouldChangeLane(const game::Position& position, game::Switch* s) {
  // TODO its just just basic greedy choosing
  if (position.piece() == switched_)
    switched_ = -1;

  if (switched_ != -1)
    return false;

  int from = NextSwitch(position.piece());

  if (car_tracker_->positions().size() <= 1 ||
      car_tracker_->Predict(position, car_tracker_->positions()[car_tracker_->positions().size() - 2], 1, 0).piece() != from)
    return false;

  int to = NextSwitch(from);

  double current = LaneLength(position, position.end_lane(), from, to);
  double left = 1000000000;
  double right = 1000000000;

  if (position.start_lane() > 0)
    left = LaneLength(position, position.end_lane() - 1, from, to);
  if (position.end_lane() < race_.track().lanes().size() - 1)
    right = LaneLength(position, position.end_lane() + 1, from, to);

  if (left < current && left < right) {
    *s = game::Switch::kSwitchLeft;
    switched_ = from;
    return true;
  }
  else if (right < current && right <= left) {
    *s = game::Switch::kSwitchRight;
    switched_ = from;
    return true;
  }
  return false;
}

// From -> To excliding both
double Bot::LaneLength(const game::Position& position, int lane, int from, int to) {
  double distance = 0;
  for (int p = from + 1; p < to; p++)
    distance += race_.track().LaneLength(p, lane);
  return distance;
}

// Next, not including given one
int Bot::NextSwitch(int piece_index) {
  int index = 1;
  auto& pieces = race_.track().pieces();

  auto piece = pieces[(piece_index + 1) % pieces.size()];

  while (index <= pieces.size() && piece.has_switch() == false) {
    index++;
    piece = pieces[(piece_index + index) % pieces.size()];
  }
  return (piece_index + index) % pieces.size();
}

void Bot::TurboStarted(const std::string& color) {
}

void Bot::TurboEnded(const std::string& color) {
}

}  // namespace stepping
}  // namespace bots
