#include "schedulers/shortest_path_switch_scheduler.h"

DEFINE_bool(overtake, true, "Should car overtake others?");

// TODO refactor whole code here, its copypaste

using game::Position;

namespace schedulers {

ShortestPathSwitchScheduler::ShortestPathSwitchScheduler(
    const game::Race& race,
    game::RaceTracker& race_tracker,
    game::CarTracker& car_tracker)
  : race_(race), car_tracker_(car_tracker), race_tracker_(race_tracker),
    should_switch_now_(false), waiting_for_switch_(false),
    target_switch_(-1) {
  //ComputeShortestPaths();
}

// Prepares for overtake
void ShortestPathSwitchScheduler::Overtake(const string& color) {
  printf("Feature not implemented\n");
}

// Updates the state and calculates next state
// TODO switch time
void ShortestPathSwitchScheduler::Schedule(const game::CarState& state) {
  if (state.position().piece() == target_switch_) {
    waiting_for_switch_ = false;
    target_switch_ = -1;
  }
  if (waiting_for_switch_)
    return;

  // TODO its greedy :D
  const Position& position = state.position();

  // We evaluate next two switches to decide if we should change the lane.
  int from = race_.track().NextSwitch(position.piece());
  int to = race_.track().NextSwitch(from);

  // Distance between current position and the 'to' switch:
  // - if we stay on current lane
  // - if we decide to switch to left lane
  // - if we decide to switch to right lane
  Position end_position;
  end_position.set_piece(to);
  double current = DistanceBetween(position, end_position, position.end_lane());
  double left = DistanceBetween(position, end_position, position.end_lane() - 1);
  double right = DistanceBetween(position, end_position, position.end_lane() + 1);

  // TODO(tomek) Improve:
  // - we should "score" every lane, based on opponents on them
  // - even if someone has very good lap, he can crashed and respawned
  // - consider instead of distance, trying to estimate how much time
  //   it will take us to drive through given lanes

  // Overtaking
  if (FLAGS_overtake) {
    if (!IsLaneSafe(state, from, to, position.end_lane() - 1)) left = kInf;
    if (!IsLaneSafe(state, from, to, position.end_lane() + 1)) right = kInf;
    if (!IsLaneSafe(state, from, to, position.end_lane())) current = kInf;
  }

  // All lanes taken
  if (left > kInf - 1 && right > kInf - 1 && current > kInf - 1) {
    // HACK to avoid lanestanders
    double left = -SlowestCarOnLane(state, from, to, position.end_lane() - 1);
    double right = -SlowestCarOnLane(state, from, to, position.end_lane() + 1);
    double current = -SlowestCarOnLane(state, from, to, position.end_lane());
  }

  if (left < current && left < right) {
    scheduled_switch_ = game::Switch::kSwitchLeft;
    target_switch_ = from;
    should_switch_now_ = true;
  } else if (right < current && right <= left) {
    scheduled_switch_ = game::Switch::kSwitchRight;
    target_switch_ = from;
    should_switch_now_ = true;
  } else {
    should_switch_now_ = false;
    target_switch_ = -1;
  }
}

bool ShortestPathSwitchScheduler::IsLaneSafe(const game::CarState& state,
    int from, int to, int lane) {
  auto cars = race_tracker_.PredictedCarsBetween(from, to, lane);

  std::vector<std::string> cars_to_overtake;

  for (auto c : cars) {
    if (race_tracker_.ShouldTryToOvertake(c->color(), from, to))
      cars_to_overtake.push_back(c->color());
  }

  return (cars_to_overtake.size() == 0);
}

double ShortestPathSwitchScheduler::SlowestCarOnLane(
    const game::CarState& state, int from, int to, int lane) {

  auto cars = race_tracker_.PredictedCarsBetween(from, to, lane);

  std::vector<std::string> cars_to_overtake;

  double min_speed = 10000;

  for (auto c : cars) {
    if (race_tracker_.ShouldTryToOvertake(c->color(), from, to))
      min_speed = fmin(min_speed, c->state().velocity());

    if (c->is_dead() && c->time_to_spawn() < 500)
      min_speed = 0;
  }
  return min_speed;
}

double ShortestPathSwitchScheduler::DistanceBetween(const Position& position1, const Position& position2, int lane) {
  if (!race_.track().IsLaneCorrect(lane)) {
    return kInf;
  }
  Position end_position = position2;
  end_position.set_start_lane(lane);
  end_position.set_end_lane(lane);

  return car_tracker_.DistanceBetween(position1, end_position);
}

void ShortestPathSwitchScheduler::ComputeShortestPaths() {
  // TODO
}

void ShortestPathSwitchScheduler::Switched() {
  should_switch_now_ = false;
  waiting_for_switch_ = true;
}

// Returns scheduled switch
bool ShortestPathSwitchScheduler::ShouldSwitch() {
  if (should_switch_now_ &&
      !waiting_for_switch_) {
    auto s = car_tracker_.current_state();
    s = car_tracker_.Predict(s, game::Command(1));
    s = car_tracker_.Predict(s, game::Command(1));
    if (s.position().piece() != target_switch_)
      return false;

    if (car_tracker_.current_state().velocity() > 0 &&
        car_tracker_.IsSafe(car_tracker_.current_state(), game::Command(scheduled_switch_)))
      return true;
  }
  return false;
}

game::Switch ShortestPathSwitchScheduler::ExpectedSwitch() {
  if (should_switch_now_ && !waiting_for_switch_)
    return scheduled_switch_;
  return game::Switch::kStay;
}

game::Switch ShortestPathSwitchScheduler::SwitchDirection() {
  return scheduled_switch_;
}

}  // namespace schedulers
