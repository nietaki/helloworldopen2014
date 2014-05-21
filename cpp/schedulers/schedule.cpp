#include "schedulers/schedule.h"

namespace schedulers {

//TODO: Why the compiler asks me for explicit schedulers::?
schedulers::Sched::Sched(game::CarTracker* car_tracker, int horizon)
  : throttles(horizon), car_tracker_(car_tracker) {}

}

game::CarState schedulers::Sched::Predict(const game::CarState& state) {
  game::CarState next = state;
  for (int i = 0; i<size(); ++i) {
    next = car_tracker_->Predict(next, game::Command(throttles[i]));
  }
  return next;
}

double schedulers::Sched::Distance(const game::CarState& state) {
  game::CarState last = Predict(state);
  return car_tracker_->DistanceBetween(state.position(), last.position());
}

// Whether schedule is safe
bool schedulers::Sched::IsSafe(const game::CarState& state) {
  //TODO: Improve performance by adding "from"
  
  game::CarState next = state;
  // Are all scheduled states safe?
  for (int i = 0; i<size(); ++i) {
    next = car_tracker_->Predict(next, game::Command(throttles[i]));
    if (!car_tracker_->crash_model().IsSafe(next.position().angle()))
      return false;
  }
  // Is the last state safe?
  return car_tracker_->IsSafe(next);
}
