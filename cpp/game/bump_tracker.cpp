#include "game/bump_tracker.h"

namespace game {

BumpTracker::BumpTracker(game::CarTracker& car_tracker,
    const game::Race& race)
  : race_(race), car_tracker_(car_tracker) {
  // TODO hardcoded time limit
  throttle_scheduler_.reset(new schedulers::BinaryThrottleScheduler(race_, car_tracker_, 5));
}

bool BumpTracker::CanBump(const CarState& bumping_state, const CarState& bumped_state) {
  return CanBumpOptimalEnemy(bumping_state, bumped_state);
}

bool BumpTracker::CanBumpOptimalEnemy(const CarState& bumping_state, const CarState& bumped_state) {
  throttle_scheduler_->Schedule(bumped_state);
  auto& schedule = throttle_scheduler_->full_schedule();

  auto bumping = bumping_state;
  auto bumped = bumped_state;

  for (double bumped_throttle : schedule) {
    bumping = car_tracker_.Predict(bumping, Command(1));
    bumped = car_tracker_.Predict(bumping, Command(bumped_throttle));

    if (!car_tracker_.crash_model().IsSafe(bumping.position().angle()))
      return false;

    if (race_.track().IsFirstInFront(bumping.position(), bumped.position()))
      return true;
  }
  return false;
}

bool BumpTracker::CanBumpForSure(const CarState& bumping_state, const CarState& bumped_state, int in_ticks) {
  auto bumping = bumping_state;
  auto bumped = bumped_state;

  for (int i = 0; i < in_ticks; i++) {
    bumping = car_tracker_.Predict(bumping, Command(1));
    bumped = car_tracker_.Predict(bumping, Command(1));

    if (!car_tracker_.crash_model().IsSafe(bumping.position().angle()))
      return false;

    if (race_.track().IsFirstInFront(bumping.position(), bumped.position()))
      return true;
  }
  return false;
}

}  // namespace game
