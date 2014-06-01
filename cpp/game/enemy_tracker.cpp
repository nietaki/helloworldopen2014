#include "game/enemy_tracker.h"

using game::Position;
using game::Race;

namespace game {

EnemyTracker::EnemyTracker(game::CarTracker& car_tracker,
    const game::Race& race,
    const std::string& color,
    const Position& position)
  : car_tracker_(car_tracker), race_(race), color_(color) {
  state_ = CarState(position);
  piece_speed_.resize(race_.track().pieces().size(), 0);
  piece_data_points_.resize(race_.track().pieces().size(), 0);
}

void EnemyTracker::RecordTick(const game::Position& position) {
  if (skip_time_ >= 0) skip_time_--;
  if (skip_time_ == 0) state_ = CarState(position);
  if (skip_time_ < 0) accelerating_ = false;

  if (dead_) time_since_crash_++;
}

bool EnemyTracker::ShouldRecord() const {
  if (accelerating_ || dead_ || finished_ || disabled_) return false;
  return true;
}

void EnemyTracker::RecordPosition(const game::Position& position) {
  RecordTick(position);
  state_ = car_tracker_.CreateCarState(state_, position);

  if (ShouldRecord()) {
    int piece = state_.position().piece();

    piece_speed_[piece] =
      double(piece_speed_[piece] * piece_data_points_[piece] + state_.velocity()) /
      double(piece_data_points_[piece] + 1);

    piece_data_points_[piece]++;

    average_speed_ =
      double(average_speed_ * average_data_points_ + state_.velocity()) /
      double(average_data_points_ + 1);

    average_data_points_++;
  }
}

Position EnemyTracker::PositionAfterTime(int time, int target_lane) const {
  auto state = state_;

  if (target_lane != -1) {
    if (state.position().end_lane() - 1 == target_lane)
      state.set_switch_state(Switch::kSwitchLeft);

    if (state.position().end_lane() + 1 == target_lane)
      state.set_switch_state(Switch::kSwitchRight);
  }

  // Wait until spawn
  if (dead_ == true)
    time -= time_to_spawn();

  // Do rest ticks regularly
  for (int i = 0; i < min(time, 1000); i++) {
    double throttle = car_tracker_.velocity_model().PredictThrottle(
        Velocity(state.position().piece()));

    // Acceleration from too low speed
    if (state.velocity() < 0.9 * Velocity(state.position().piece()))
      throttle = 1;

    state = car_tracker_.Predict(state, Command(throttle));
  }

  return state.position();
}

int EnemyTracker::TimeToPosition(const Position& target) const {
  auto state = state_;

  if (target.end_lane() != state.position().end_lane()) {
    if (state.position().end_lane() - 1 == target.end_lane())
      state.set_switch_state(Switch::kSwitchLeft);

    if (state.position().end_lane() + 1 == target.end_lane())
      state.set_switch_state(Switch::kSwitchRight);
  }

  int time = 0;
  if (dead_) time = time_to_spawn();

  const int kCarLength = race_.cars()[0].length();

  for (int limit = 0; limit < 1000; limit++, time++) {
    double throttle = car_tracker_.velocity_model().PredictThrottle(
        Velocity(state.position().piece()));

    // Acceleration from too low speed
    if (state.velocity() < 0.9 * Velocity(state.position().piece()))
      throttle = 1;

    state = car_tracker_.Predict(state, Command(throttle));

    if (car_tracker_.DistanceBetween(state.position(), target) >
        car_tracker_.DistanceBetween(target, state.position()))
      return time;
  }

  return 100000;
}

// Checks if time difference on that part of track is enough to overtake him
// Based on best laptime right now
bool EnemyTracker::CanOvertake(const EnemyTracker& noobek, int from, int to) {
  if (best_lap_ == -1 || noobek.best_lap() == -1)
    return true;

  // Cant overtake faster
  if (best_lap_ > noobek.best_lap())
    return false;

  // TODO hella inaccurate
  int pieces = from <= to ? to - from + 1 : to + race_.track().pieces().size() - from + 1;

  double percent = double(pieces) / double(race_.track().pieces().size());

  int ticks_difference = double(noobek.best_lap() - best_lap_) * percent;

  const double kCarLength = race_.cars().at(0).length();

  double distance = 0;
  for (int i = 0; i < ticks_difference; i++) {
    distance += piece_speed_[to];
    // TODO
    if (distance > 2.0 * kCarLength)
      return true;
  }

  return false;
}

double EnemyTracker::Velocity(int piece) const {
  double velocity = piece_speed_[state_.position().piece()];
  if (velocity < 1e-9) // Not yet calculated
    velocity = average_speed_;
  return velocity;
}

// Record methods

void EnemyTracker::RecordLapTime(int time) {
  ready_ = true;
  lap_times_.push_back(time);
  if (best_lap_ == -1 || best_lap_ > time)
    best_lap_ = time;
}

void EnemyTracker::RecordCrash() {
  time_since_crash_ = 0;
  dead_ = true;
}

void EnemyTracker::DNF() {
  disabled_ = true;
}

void EnemyTracker::FinishedRace() {
  finished_ = true;
}

void EnemyTracker::Resurrect() {
  if (!disabled_) {
    skip_time_ = kSkipTime;
    accelerating_ = true;
    finished_ = false;
    dead_ = false;
  }
}

void EnemyTracker::Spawned() {
  state_.ResetTurbo();
  dead_ = false;
  accelerating_ = true;
  skip_time_ = kSkipTime;
}

void EnemyTracker::TurboStarted() {
  state_.EnableTurbo();
}

void EnemyTracker::NewTurbo(const Turbo& turbo) {
  state_.AddNewTurbo(turbo);
}

}  // namespace game
