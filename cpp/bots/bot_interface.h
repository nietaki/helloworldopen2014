#ifndef CPP_BOTS_BOT_INTERFACE_H_
#define CPP_BOTS_BOT_INTERFACE_H_

#include <string>
#include <map>
#include "game/command.h"
#include "game/position.h"

namespace game {
class Race;
}

namespace bots {

class BotInterface {
 public:
  virtual ~BotInterface() {}
  virtual void JoinedGame() = 0;
  virtual void YourCar(const std::string& color) = 0;
  virtual void NewRace(const game::Race& race) = 0;
  virtual void GameStarted() = 0;

  virtual game::Command GetMove(const std::map<std::string, game::Position>& positions, int game_tick) = 0;
  virtual void CarFinishedLap(const std::string& color) = 0;
  virtual void CarFinishedRace(const std::string& color) = 0;
  virtual void GameEnd() = 0;
  virtual void TournamentEnd() = 0;

  virtual void CarCrashed(const std::string& color) = 0;
  virtual void CarSpawned(const std::string& color) = 0;
};

}  // namespace bots

#endif  // CPP_BOTS_BOT_INTERFACE_H_
