#include "bots/raw_bot.h"

#include <ctime>
#include <fstream>
#include <iostream>

#include "game/position.h"
#include "game/race.h"

#include "game/turbo.h"
#include "gflags/gflags.h"
#include "utils/stopwatch.h"

DEFINE_bool(print_track, false, "");
DECLARE_bool(continuous_integration);
DEFINE_bool(log_command, false, "");

using jsoncons::json;

namespace bots {

RawBot::RawBot(BotInterface* bot)
  : bot_(bot), visualizer_(), action_map_ {
      { "join", &RawBot::OnJoin },
      { "joinRace", &RawBot::OnJoin },
      { "yourCar", &RawBot::OnYourCar },
      { "gameInit", &RawBot::OnGameInit },
      { "gameStart", &RawBot::OnGameStart },
      { "carPositions", &RawBot::OnCarPositions },
      { "lapFinished", &RawBot::OnLapFinished },
      { "finish", &RawBot::OnFinish },
      { "gameEnd", &RawBot::OnGameEnd },
      { "tournamentEnd", &RawBot::OnTournamentEnd },
      { "crash", &RawBot::OnCrash },
      { "spawn", &RawBot::OnSpawn },
      { "error", &RawBot::OnError },
      { "dnf", &RawBot::OnDNF },
      { "turboAvailable", &RawBot::OnTurboAvailable },
      { "turboStart", &RawBot::OnTurboEnd },
      { "turboEnd", &RawBot::OnTurboStart }
    }
{
}

RawBot::~RawBot() {
}

RawBot::msg_vector RawBot::CommandToMsg(const game::Command& command, int game_tick) {
  if (command.SwitchSet()) {
    printf("Command: Switch\n");
    return { utils::make_switch(command.get_switch(), game_tick) };
  } else if (command.TurboSet()) {
    printf("Command: Turbo\n");
    return { utils::make_turbo(game_tick) };
  } else if (command.ThrottleSet()) {
    return { utils::make_throttle(command.get_throttle(), game_tick) };
  } else {
    return ping();
  }
}

RawBot::msg_vector RawBot::React(const jsoncons::json& msg) {
  const auto& msg_type = msg.get("msgType", jsoncons::json("")).as_string();
  auto action_it = action_map_.find(msg_type);
  if (action_it != action_map_.end()) {
    return (action_it->second)(this, msg);
  } else {
    std::cout << "Unknown message type: " << msg_type << "\n\n\n\n" << std::endl;
    return ping();
  }
}

RawBot::msg_vector RawBot::OnJoin(const jsoncons::json& msg) {
  std::cout << "Server: Join" << std::endl;
  bot_->JoinedGame();
  return ping();
}

RawBot::msg_vector RawBot::OnYourCar(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));

  const auto& color = data["color"].as_string();
  std::cout << "Server: Your Car - " << ColorPrint(color) << std::endl;

  bot_->YourCar(color);
  return ping();
}

RawBot::msg_vector RawBot::OnGameInit(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));

  std::cout << "Server: Game Init" << std::endl;

  if (FLAGS_print_track) {
    std::cout << pretty_print(msg) << std::endl;
  }

  game::Race race;
  race.ParseFromJson(data["race"]);

  visualizer_.set_race(race);
  bot_->NewRace(race);
  return ping();
}

RawBot::msg_vector RawBot::OnGameStart(const jsoncons::json& msg) {
  std::cout << "Server: Game start" << std::endl;

  visualizer_.GameStart();
  bot_->GameStarted();

  last_game_tick_ = -1;
  if (msg.has_member("gameTick")) {
    last_on_car_positions_["gameTick"] = msg["gameTick"];
  }

  return ProcessOnCarPositions(last_on_car_positions_);
}

RawBot::msg_vector RawBot::ProcessOnCarPositions(const jsoncons::json& msg) {
  clock_t begin_time = clock();
  if (FLAGS_continuous_integration) {
    printf("Time from last received msg: %lf ms\n", stopwatch_.elapsed());
  }
  stopwatch_.reset();

  const auto& data = msg.get("data", jsoncons::json(""));

  const int game_tick = msg.get("gameTick", jsoncons::json(0)).as_int();

  if (last_game_tick_ + 1 != game_tick) {
    std::cerr << "Bad game tick. Received " << game_tick << " expected: " << last_game_tick_;
  }
  last_game_tick_ = game_tick;

  std::map<std::string, game::Position> positions;
  for (auto it = data.begin_elements(); it != data.end_elements(); ++it) {
    game::Position position;
    auto color = position.ParseFromJson(*it);

    if (position.last_tick() != -1 && position.last_tick() != last_game_tick_ - 1) {
      bot_->LastTickIgnored(color, 0);
    } else {
      bot_->LastTickApproved(color, 0);
    }

    positions[color] = position;
  }

  visualizer_.Update(positions);
  auto command = CommandToMsg(bot_->GetMove(positions, game_tick), game_tick);
  if (FLAGS_log_command) {
    std::cout << "command: " << command[0] << std::endl;
  }

  if (FLAGS_continuous_integration) {
    std::cout << "game_tick: " << game_tick << " time: " << double(clock() - begin_time) /  CLOCKS_PER_SEC * 1000 << "ms " 
        << "stopwatch: " << stopwatch_.elapsed() << " ms" << std::endl << std::endl;
  }

  return command;
}

RawBot::msg_vector RawBot::OnCarPositions(const jsoncons::json& msg) {
  if (msg.has_member("gameTick")) {
    return ProcessOnCarPositions(msg);
  } else {
    last_on_car_positions_ = msg;
  }
  return {};
}

RawBot::msg_vector RawBot::OnLapFinished(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));

  game::Result result;
  result.ParseFromJson(data);

  visualizer_.LapFinished(result);

  bot_->CarFinishedLap(result.color(), result);
  return ping();
}

RawBot::msg_vector RawBot::OnFinish(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));
  const auto& color = data["color"].as_string();

  visualizer_.CarFinishedRace(color);

  bot_->CarFinishedRace(color);
  return ping();
}

RawBot::msg_vector RawBot::OnGameEnd(const jsoncons::json& msg) {
  visualizer_.GameEnd();
  std::cout << "Server: Game end" << std::endl;

  bot_->GameEnd(/* results */);
  last_game_tick_ = -1;
  return ping();
}

RawBot::msg_vector RawBot::OnTournamentEnd(const jsoncons::json& msg) {
  std::cout << "Server: Tournament end" << std::endl;
  bot_->TournamentEnd();
  return ping();
}

RawBot::msg_vector RawBot::OnCrash(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));
  const auto& color = data["color"].as_string();

  std::cout << "Crash on tick: " << last_game_tick_ << std::endl;

  visualizer_.CarCrashed(color);
  bot_->CarCrashed(color);
  return ping();
}

RawBot::msg_vector RawBot::OnSpawn(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));
  const auto& color = data["color"].as_string();

  bot_->CarSpawned(color);
  return ping();
}

RawBot::msg_vector RawBot::OnError(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));
  std::cout << "Server: Error - " << data.to_string() << std::endl;
  return ping();
}

RawBot::msg_vector RawBot::OnDNF(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));
  std::cout << "Server: Disqualification - " << data["reason"].as<std::string>() <<  std::endl;
  std::cout << data["car"] << std::endl;
  bot_->CarDNF(data["car"]["color"].as_string());
  return ping();
}

std::string RawBot::ColorPrint(const std::string& color) const {
  std::map<std::string, std::string> colors {
    { "red", "\x1B[31m" },
    { "blue", "\x1B[34m" },
    { "green", "\x1B[32m" },
    { "yellow", "\x1B[33m" },
    { "normal", "\x1B[0m" }
  };

  if (colors.find(color) != colors.end())
    return colors[color] + color + colors["normal"];
  else
    return color;
}

RawBot::msg_vector RawBot::OnTurboAvailable(const jsoncons::json& msg) {
  game::Turbo turbo;
  turbo.ParseFromJson(msg.get("data", jsoncons::json("")));
  // std::cout << "game_tick: " << last_game_tick_ << std::endl;
  // std::cout << turbo.DebugString();
  bot_->OnTurbo(turbo);

  return ping();
}

RawBot::msg_vector RawBot::OnTurboStart(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));
  const auto& color = data["color"].as_string();

  bot_->TurboEnded(color);
  return ping();
}

RawBot::msg_vector RawBot::OnTurboEnd(const jsoncons::json& msg) {
  const auto& data = msg.get("data", jsoncons::json(""));
  const auto& color = data["color"].as_string();

  bot_->TurboStarted(color);
  return ping();
}

}  // namespace bots
