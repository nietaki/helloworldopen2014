#include "bots/stepping/bot.h"

DEFINE_int32(answer_time, 10, "Time limit for answer in ms");

using std::string;
using std::vector;
using std::map;

using game::CarTracker;
using game::CarState;
using game::Command;
using game::Position;
using game::Race;

namespace bots {
namespace stepping {

Bot::Bot() {
}

void Bot::NewRace(const Race& race) {
  race_ = race;
  car_tracker_.reset(new CarTracker(&race_));
  throttle_scheduler_.reset(
      new schedulers::BinaryThrottleScheduler(race_, *car_tracker_.get(), FLAGS_answer_time));
  turbo_scheduler_.reset(
      new schedulers::GreedyTurboScheduler(race_, *car_tracker_.get()));
  switch_scheduler_.reset(
      new schedulers::ShortestPathSwitchScheduler(race_, *car_tracker_.get()));
}

game::Command Bot::GetMove(const map<string, Position>& positions, int game_tick)  {
  const Position& position = positions.at(color_);
  car_tracker_->Record(position);
  auto& state = car_tracker_->current_state();

  // TODO
  if (crashed_) {
    return Command(0);
  }

  SetStrategy(state);

  throttle_scheduler_->Schedule(state);
  switch_scheduler_->Schedule(state);
  turbo_scheduler_->Schedule(state);

  Command command;

  if (turbo_scheduler_->ShouldFireTurbo()) {
    command = Command(game::TurboToggle::kToggleOn);
    turbo_scheduler_->TurboUsed();
    printf("YABADABADUUUU\n");
  } else if (switch_scheduler_->ShouldSwitch()) {
    command = Command(switch_scheduler_->SwitchDirection());
    switch_scheduler_->Switched();
  } else {
    command = Command(throttle_scheduler_->throttle());
  }

  car_tracker_->RecordCommand(command);
  return command;
}

void Bot::SetStrategy() {
  auto strategy = Strategy::kOptimizeRace;
  int lap = state.position().lap();

  if (lap % 2 == 1)
    strategy = Strategy::kOptimizeNextLap;
  if (lap % 2 == 0 && lap != 0)
    strategy = Strategy::kOptimizeCurrentLap;

  throttle_scheduler_->set_strategy(strategy);
  switch_scheduler_->set_strategy(strategy);
  turbo_scheduler_->set_strategy(strategy);
}

void Bot::OnTurbo(const game::Turbo& turbo) {
  turbo_scheduler_->NewTurbo(turbo);
  printf("Turbo Available\n");
}

void Bot::YourCar(const string& color) {
  color_ = color;
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
  auto& state = car_tracker_->current_state();
  auto next = car_tracker_->Predict(state, Command(car_tracker_->throttle()));
  printf("Crash! %lf %lf %lf\n", next.position().angle(), state.position().angle(), state.previous_angle());

  if (color == color_) {
    crashed_ = true;
    car_tracker_->RecordCarCrash();
  }
}

void Bot::CarSpawned(const string& color)  {
  if (color == color_)
    crashed_ = false;
}


void Bot::TurboStarted(const std::string& color) {
}

void Bot::TurboEnded(const std::string& color) {
}

}  // namespace stepping
}  // namespace bots
