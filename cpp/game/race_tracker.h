#ifndef CPP_GAME_RACE_TRACKER_H_
#define CPP_GAME_RACE_TRACKER_H_

#include "game/race.h"
#include "game/enemy_tracker.h"
#include "game/position.h"
#include "game/car_tracker.h"
#include "game/bump_detector.h"
#include "game/lane_scorer.h"
#include "utils/deadline.h"

namespace game {

class RaceTracker {
 public:
  RaceTracker(game::CarTracker& car_tracker,
              const game::Race& race, const std::string& color);

  // Detects if issuing the given command is safe based on cars
  // in front of us. If false, also returns command that is safe in safe_command.
  //
  // It only checks cars that are ahead of us (or will be ahead of us), and
  // makes sure that if we bump them, we will not crash.
  //
  // Use cases that we need to consider.
  // - someone is (or will be) in front of us and we will hit him
  // - someone respawns on top of us - this is hard, because we want to consider it
  //   only if we will not be able to pass him if we go optimally (it doesn't
  //   make sense to slow down if going fast is safe).
  // - one tick after the switch piece -
  //
  // Assumptions:
  // - for a given
  //
  // What is not working:
  // - if someone is much slower, and will take a switch just before us and
  //   we bump into him.
  // - if we ride on switch (or someone is on switch)
  bool IsSafeAhead(const CarState& current_state, const Command& command, Command* safe_command);

  bool IsSafeAttack(const CarState& current_state, const Command& command, Command* safe_command);

  bool IsSafeBehind(const CarState& current_state,
                    const vector<double>& schedule,
                    const Command& command,
                    Command* safe_command);

  // Recording methods

  void Record(const std::map<std::string, game::Position>& positions, int game_tick);
  void RecordCrash(const std::string& color);
  void RecordLapTime(const std::string& color, int time);
  void FinishedRace(const std::string& color);
  void TurboForEveryone(const game::Turbo& turbo);
  void CarSpawned(const std::string& color);
  void TurboStarted(const std::string& color);
  void DNF(const std::string& color);
  void ResurrectCars();

  // Lane scores based on who is there
  std::map<Switch, int> ScoreLanes(const CarState& state, const utils::Deadline& deadline);

  // TODO
  // Move to enemy?
  bool WorthBumping(const std::string& color);
  bool ShouldOvertake(const std::string& color, int from, int to);

  bool IsCompetitive(const std::string& color) { return false; }

  // Getters
  const std::vector<EnemyTracker>& enemies() const { return enemies_; }
  const std::string& my_color() const { return color_; }
  EnemyTracker& enemy(const std::string& color) { return enemies_[indexes_[color]]; }

  // Returns true if there was bump between those two cars in last tick
  bool BumpOccured(const std::string& color, const std::string& color2);

  BumpDetector& bump_detector() { return bump_detector_; }
  void ResetBumpDetector() { bump_detector_.Reset(); }

  bool HasSomeoneMaybeBumpedMe(const map<string, Position>& positions, double* bump_velocity);
  bool HasBumped(
      const Position& position1,
      const Position& position2);

 private:
  // TODO(tomek) deprecated version of IsSafeAhead() method.
  bool IsSafeInFront(const CarState& current_state, const Command& command, Command* safe_command);

  bool IsSafe(const CarState& state, const Command& command,
              Command* safe_command, const Command& our_command);

  void DetectBumps(const std::map<std::string, Position>& positions);

  bool IsBumpInevitable(
      const CarState& my_state_before,
      const CarState& my_state_after,
      const CarState& enemy_state,
      int ticks_after);

  // void RecordEnemy(int index, const game::Position& position);
  // void UpdateSpeedStats(int index, const game::Position& position);

  std::vector<EnemyTracker> enemies_;
  // Maps color to index
  std::map<std::string, int> indexes_;

  const Race& race_;
  CarTracker& car_tracker_;
  std::string color_;

  BumpDetector bump_detector_;
  LaneScorer lane_scorer_;
};

}  // namespace game

#endif  // CPP_GAME_RACE_TRACKER_H_
