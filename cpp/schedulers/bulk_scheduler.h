#ifndef SCHEDULERS_BULK_SCHEDULER_H_
#define SCHEDULERS_BULK_SCHEDULER_H_

#include <string>
#include "game/command.h"
#include "game/car_tracker.h"
#include "schedulers/strategy.h"
#include "schedulers/scheduler.h"

#include "schedulers/greedy_turbo_scheduler.h"
#include "schedulers/shortest_path_switch_scheduler.h"
#include "schedulers/binary_throttle_scheduler.h"

namespace schedulers {

class BulkScheduler : public Scheduler {
 public:
   BulkScheduler(const game::Race& race,
                 game::CarTracker& car_tracker,
                 int time_limit);

   void Schedule(const game::CarState& state) override;

   void Overtake(const string& color) override;

   void set_strategy(const Strategy& strategy) override { strategy_ = strategy; }

   game::Command command() override { return command_; }

   void IssuedCommand(const game::Command& command) override;

  private:
   Strategy strategy_;
   game::Command command_;

   game::CarTracker& car_tracker_;
   const game::Race& race_;

   std::unique_ptr<schedulers::ThrottleScheduler> throttle_scheduler_;
   std::unique_ptr<schedulers::SwitchScheduler> switch_scheduler_;
   std::unique_ptr<schedulers::TurboScheduler> turbo_scheduler_;
};

}  // namespace schedulers

#endif  // SCHEDULERS_BULK_SCHEDULER_H_