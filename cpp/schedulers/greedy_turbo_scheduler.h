#ifndef CPP_SCHEDULERS_GREEDY_TURBO_SCHEDULER_H_
#define CPP_SCHEDULERS_GREEDY_TURBO_SCHEDULER_H_

#include "schedulers/strategy.h"
#include "schedulers/turbo_scheduler.h"
#include "game/turbo.h"
#include "game/race.h"
#include "game/car_tracker.h"


namespace schedulers {

class GreedyTurboScheduler : public TurboScheduler {
 public:
   GreedyTurboScheduler(const game::Race& race,
                        game::CarTracker& car_tracker);

   // Returns if should we use turbo
   bool ShouldFireTurbo();

   // Makes decision on turbo usage
   void Schedule(const game::CarState& state);

   // Prepare for overtake
   void Overtake(const string& color);

   void set_strategy(const Strategy& strategy) { strategy_ = strategy; }

   void NewTurbo(const game::Turbo& turbo);

   void TurboUsed();

  private:
   game::CarTracker& car_tracker_;
   const game::Race& race_;

   bool turbo_available_;
   game::Turbo turbo_;

   Strategy strategy_;
   bool should_fire_now_;
};

}  // namespace schedulers

#endif  // CPP_SCHEDULERS_GREEDY_TURBO_SCHEDULER_H_