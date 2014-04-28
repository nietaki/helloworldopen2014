#ifndef CPP_GAME_ENEMY_TRACKER_H_
#define CPP_GAME_ENEMY_TRACKER_H_

#include <string>
#include <map>
#include "game/race.h"
#include "game/car_tracker.h"

namespace game {

class EnemyTracker {
 public:
  EnemyTracker(game::CarTracker& car_tracker, const game::Race& race,
      const std::string& color, const game::Position& position);

  // lap time is measured in game ticks
  void RecordLapTime(int time);
  void RecordPosition(const game::Position& position);
  void RecordCrash();

  // time = game_ticks
  Position PositionAfterTime(int time);
  int TimeToPosition(const Position& p);

  /*
  int expected_bump_time() const { return expected_bump_time_; }
  std::pair<int, double> expected_bump_position() const { return expected_bump_position_; }
  */

  const CarState& state() const { return state_; }
  bool is_dead() const { return dead_; }
  int time_to_spawn() const { return time_to_spawn_; }

  const std::string& color() const { return color_; }

 private:
  double Velocity(int piece);

  // double speed_factor_;
  std::vector<int> lap_times_;

  std::vector<double> piece_speed_;
  std::vector<int> piece_data_points_;

  double average_speed_;
  int average_data_points_;

  CarState state_;

  const Race& race_;
  CarTracker& car_tracker_;
  std::string color_;

  int skip_;
  bool dead_;

  int time_to_spawn_;
  static const int kRespawnTime = 300;
  static const int kSkipTime = 20;
};
}  // namespace game

#endif  // CPP_GAME_ENEMY_TRACKER_H_
